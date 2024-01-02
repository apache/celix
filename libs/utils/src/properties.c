/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "properties.h"
#include "celix_properties.h"
#include "celix_properties_private.h"
#include "celix_properties_internal.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "celix_build_assert.h"
#include "celix_err.h"
#include "celix_string_hash_map.h"
#include "celix_utils.h"
#include "celix_stdlib_cleanup.h"
#include "celix_convert_utils.h"
#include "celix_utils_private_constants.h"

static const char* const CELIX_PROPERTIES_BOOL_TRUE_STRVAL = "true";
static const char* const CELIX_PROPERTIES_BOOL_FALSE_STRVAL = "false";
static const char* const CELIX_PROPERTIES_EMPTY_STRVAL = "";

struct celix_properties {
    celix_string_hash_map_t* map;

    /**
     * String buffer used to store the first key/value entries,
     * so that in many cases - for usage in service properties - additional memory allocations are not needed.
     *
     * @note based on some small testing most services properties seem to max around 300 bytes.
     * So 128 (next factor 2 based value) seems like a good fit.
     * The size is tunable by changing CMake cache variable CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE or Conan option celix_properties_optimization_string_buffer_size.
     */
    char stringBuffer[CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE];

    /**
     * The current string buffer index.
     */
    int currentStringBufferIndex;

    /**
     * Entries buffer used to store the first entries, so that in many cases additional memory allocation
     * can be prevented.
     *
     * @note based on some small testing most services properties seem to max out at 11 entries.
     * So 16 (next factor 2 based value) seems like a good fit.
     * The size is tunable by changing CMake cache variable CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE or Conan option celix_properties_optimization_entries_buffer_size.
     */
    celix_properties_entry_t entriesBuffer[CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE];

    /**
     * The current string buffer index.
     */
    int currentEntriesBufferIndex;
};

#define MALLOC_BLOCK_SIZE 5

static celix_status_t celix_properties_parseLine(const char* line, celix_properties_t* props);

properties_pt properties_create(void) { return celix_properties_create(); }

void properties_destroy(properties_pt properties) { celix_properties_destroy(properties); }

properties_pt properties_load(const char* filename) { return celix_properties_load(filename); }

properties_pt properties_loadWithStream(FILE* file) { return celix_properties_loadWithStream(file); }

properties_pt properties_loadFromString(const char* input) { return celix_properties_loadFromString(input); }

/**
 * Header is ignored for now, cannot handle comments yet
 */
void properties_store(properties_pt properties, const char* filename, const char* header) {
    celix_properties_store(properties, filename, header);
}

celix_status_t properties_copy(properties_pt properties, properties_pt* out) {
    celix_properties_t* copy = celix_properties_copy(properties);
    *out = copy;
    return copy == NULL ? CELIX_ENOMEM : CELIX_SUCCESS;
}

const char* properties_get(properties_pt properties, const char* key) {
    return celix_properties_get(properties, key, NULL);
}

const char* properties_getWithDefault(properties_pt properties, const char* key, const char* defaultValue) {
    return celix_properties_get(properties, key, defaultValue);
}

void properties_set(properties_pt properties, const char* key, const char* value) {
    celix_properties_set(properties, key, value);
}

void properties_unset(properties_pt properties, const char* key) { celix_properties_unset(properties, key); }

static void updateBuffers(char** key, char** value, char** output, int outputPos, int* key_len, int* value_len) {
    if (*output == *key) {
        if (outputPos == (*key_len) - 1) {
            (*key_len) += MALLOC_BLOCK_SIZE;
            *key = realloc(*key, *key_len);
            *output = *key;
        }
    } else {
        if (outputPos == (*value_len) - 1) {
            (*value_len) += MALLOC_BLOCK_SIZE;
            *value = realloc(*value, *value_len);
            *output = *value;
        }
    }
}

/**
 * Create a new string from the provided str by either using strdup or storing the string the short properties
 * optimization string buffer.
 */
char* celix_properties_createString(celix_properties_t* properties, const char* str) {
    if (str == NULL) {
        return (char*)CELIX_PROPERTIES_EMPTY_STRVAL;
    }
    size_t len = strnlen(str, CELIX_UTILS_MAX_STRLEN) + 1;
    size_t left = CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE - properties->currentStringBufferIndex;
    char* result;
    if (len < left) {
        memcpy(&properties->stringBuffer[properties->currentStringBufferIndex], str, len);
        result = &properties->stringBuffer[properties->currentStringBufferIndex];
        properties->currentStringBufferIndex += (int)len;
    } else {
        result = celix_utils_strdup(str);
    }
    return result;
}
/**
 * Free string, but first check if it a static const char* const string or part of the short properties
 * optimization.
 */
static void celix_properties_freeString(celix_properties_t* properties, char* str) {
    if (str == CELIX_PROPERTIES_BOOL_TRUE_STRVAL || str == CELIX_PROPERTIES_BOOL_FALSE_STRVAL ||
        str == CELIX_PROPERTIES_EMPTY_STRVAL) {
        // str is static const char* const -> nop
    } else if (str >= properties->stringBuffer &&
               str < (properties->stringBuffer + CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE)) {
        // str is part of the properties string buffer -> nop
    } else {
        free(str);
    }
}

/**
 * Fill entry and optional use the short properties optimization string buffer.
 */
static celix_status_t celix_properties_fillEntry(celix_properties_t* properties,
                                                 celix_properties_entry_t* entry,
                                                 const celix_properties_entry_t* prototype) {
    char convertedValueBuffer[21] = {0};
    *entry = *prototype;
    if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        bool written = celix_version_fillString(prototype->typed.versionValue, convertedValueBuffer, sizeof(convertedValueBuffer));
        if (written) {
            entry->value = celix_properties_createString(properties, convertedValueBuffer);
        } else {
            entry->value = celix_version_toString(prototype->typed.versionValue);
        }
    } else if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG) {
        // LONG_MAX str is 19 chars, LONG_MIN str is 20 chars
        (void)snprintf(convertedValueBuffer, sizeof(convertedValueBuffer), "%li", entry->typed.longValue);
        entry->value = celix_properties_createString(properties, convertedValueBuffer);
    } else if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
        int written = snprintf(convertedValueBuffer, sizeof(convertedValueBuffer), "%f", entry->typed.doubleValue);
        if (written >= 0 || written < sizeof(convertedValueBuffer)) {
            entry->value = celix_properties_createString(properties, convertedValueBuffer);
        } else {
            char* val = NULL;
            asprintf(&val, "%f", entry->typed.doubleValue);
            entry->value = val;
        }
    } else if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL) {
        entry->value = entry->typed.boolValue ? CELIX_PROPERTIES_BOOL_TRUE_STRVAL : CELIX_PROPERTIES_BOOL_FALSE_STRVAL;
    } else /*string value*/ {
        assert(prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING);
        entry->value = celix_properties_createString(properties, prototype->typed.strValue);
        entry->typed.strValue = entry->value;
    }

    if (entry->value == NULL) {
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

/**
 * Allocate entry and optionally use the short properties optimization entries buffer.
 */
celix_properties_entry_t* celix_properties_allocEntry(celix_properties_t* properties) {
    celix_properties_entry_t* entry;
    if (properties->currentEntriesBufferIndex < CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE) {
        entry = &properties->entriesBuffer[properties->currentEntriesBufferIndex++];
    } else {
        entry = malloc(sizeof(*entry));
    }
    return entry;
}

/**
 * Create entry and optionally use the short properties optimization entries buffer and take ownership of the
 * provided key and value strings.
 */
static celix_properties_entry_t* celix_properties_createEntryWithNoCopy(celix_properties_t* properties,
                                                                        const char* strValue) {
    celix_properties_entry_t* entry = celix_properties_allocEntry(properties);
    if (entry == NULL) {
        return NULL;
    }
    entry->value = strValue;
    entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_STRING;
    entry->typed.strValue = strValue;
    return entry;
}

static void celix_properties_destroyEntry(celix_properties_t* properties, celix_properties_entry_t* entry) {
    celix_properties_freeString(properties, (char*)entry->value);
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        celix_version_destroy((celix_version_t*)entry->typed.versionValue);
    }

    if (entry >= properties->entriesBuffer &&
        entry <= (properties->entriesBuffer + CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE)) {
        if (entry == (properties->entriesBuffer + properties->currentEntriesBufferIndex - 1)) {
            // entry is part of the properties entries buffer -> decrease the currentEntriesBufferIndex
            properties->currentEntriesBufferIndex -= 1;
        } else {
            // entry is part of the properties entries buffer, but not the last entry -> nop
        }
    } else {
        free(entry);
    }
}



/**
 * Create entry and optionally use the short properties optimization buffers.
 * Only 1 of the types values (strValue, LongValue, etc) should be provided.
 */
static celix_properties_entry_t* celix_properties_createEntry(celix_properties_t* properties,
                                                              const celix_properties_entry_t* prototype) {
    celix_properties_entry_t* entry = celix_properties_allocEntry(properties);
    if (entry == NULL) {
        if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
            celix_version_destroy((celix_version_t*)prototype->typed.versionValue);
        }
        celix_err_pushf("Cannot allocate property entry");
        return NULL;
    }

    celix_status_t status = celix_properties_fillEntry(properties, entry, prototype);
    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Cannot fill property entry");
        celix_properties_destroyEntry(properties, entry);
        return NULL;
    }
    return entry;
}

/**
 * Create and add entry and optionally use the short properties optimization buffers.
 * The prototype is used to determine the type of the value.
 */
static celix_status_t celix_properties_createAndSetEntry(celix_properties_t* properties,
                                                         const char* key,
                                                         const celix_properties_entry_t* prototype) {
    if (!properties) {
        if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
            celix_version_destroy((celix_version_t*)prototype->typed.versionValue);
        }
        return CELIX_SUCCESS; // silently ignore
    }

    if (!key) {
        if (prototype->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
            celix_version_destroy((celix_version_t*)prototype->typed.versionValue);
        }
        celix_err_pushf("Cannot set property with NULL key");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_properties_entry_t* entry = celix_properties_createEntry(properties, prototype);
    if (!entry) {
        return CELIX_ENOMEM;
    }

    const char* mapKey = key;
    if (!celix_stringHashMap_hasKey(properties->map, key)) {
        // new entry, needs new allocated key;
        mapKey = celix_properties_createString(properties, key);
        if (!mapKey) {
            celix_properties_destroyEntry(properties, entry);
            return CELIX_ENOMEM;
        }
    }

    celix_status_t status = celix_stringHashMap_put(properties->map, mapKey, entry);
    if (status != CELIX_SUCCESS) {
        celix_properties_destroyEntry(properties, entry);
        if (mapKey != key) {
            celix_properties_freeString(properties, (char*)mapKey);
        }
    }
    return status;
}

static void celix_properties_removeKeyCallback(void* handle, char* key) {
    celix_properties_t* properties = handle;
    celix_properties_freeString(properties, key);
}

static void celix_properties_removeEntryCallback(void* handle,
                                                 const char* key __attribute__((unused)),
                                                 celix_hash_map_value_t val) {
    celix_properties_t* properties = handle;
    celix_properties_entry_t* entry = val.ptrValue;
    celix_properties_destroyEntry(properties, entry);
}

celix_properties_t* celix_properties_create() {
    celix_properties_t* props = malloc(sizeof(*props));
    if (props != NULL) {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.storeKeysWeakly = true;
        opts.initialCapacity = CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE;
        opts.removedCallbackData = props;
        opts.removedCallback = celix_properties_removeEntryCallback;
        opts.removedKeyCallback = celix_properties_removeKeyCallback;
        props->map = celix_stringHashMap_createWithOptions(&opts);
        props->currentStringBufferIndex = 0;
        props->currentEntriesBufferIndex = 0;
        if (props->map == NULL) {
            free(props);
            props = NULL;
        }
    }
    return props;
}

void celix_properties_destroy(celix_properties_t* props) {
    if (props != NULL) {
        celix_stringHashMap_destroy(props->map);
        free(props);
    }
}

celix_properties_t* celix_properties_load(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        celix_err_pushf("Cannot open file '%s'", filename);
        return NULL;
    }
    celix_properties_t* props = celix_properties_loadWithStream(file);
    fclose(file);
    return props;
}

static celix_status_t celix_properties_parseLine(const char* line, celix_properties_t* props) {
    int linePos;
    int outputPos;

    bool precedingCharIsBackslash = false;
    char* output = NULL;
    int key_len = MALLOC_BLOCK_SIZE;
    int value_len = MALLOC_BLOCK_SIZE;
    linePos = 0;
    precedingCharIsBackslash = false;
    output = NULL;
    outputPos = 0;

    // Ignore empty lines
    if (line[0] == '\n' && line[1] == '\0') {
        return CELIX_SUCCESS;
    }

    celix_autofree char* key = calloc(1, key_len);
    celix_autofree char* value = calloc(1, value_len);
    if (!key || !value) {
        celix_err_pushf("Cannot allocate memory for key or value. Got error %i", errno);
        return CELIX_ENOMEM;
    }

    key[0] = '\0';
    value[0] = '\0';

    while (line[linePos] != '\0') {
        if (line[linePos] == ' ' || line[linePos] == '\t') {
            if (output == NULL) {
                // ignore
                linePos += 1;
                continue;
            }
        } else {
            if (output == NULL) {
                output = key;
            }
        }
        if (line[linePos] == '=' || line[linePos] == ':' || line[linePos] == '#' || line[linePos] == '!') {
            if (precedingCharIsBackslash) {
                // escaped special character
                output[outputPos++] = line[linePos];
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                precedingCharIsBackslash = false;
            } else {
                if (line[linePos] == '#' || line[linePos] == '!') {
                    if (outputPos == 0) {
                        // comment line, ignore
                        return CELIX_SUCCESS;
                    } else {
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    }
                } else {                   // = or :
                    if (output == value) { // already have a seperator
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    } else {
                        output[outputPos++] = '\0';
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                        output = value;
                        outputPos = 0;
                    }
                }
            }
        } else if (line[linePos] == '\\') {
            if (precedingCharIsBackslash) { // double backslash -> backslash
                output[outputPos++] = '\\';
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
            }
            precedingCharIsBackslash = true;
        } else { // normal character
            precedingCharIsBackslash = false;
            output[outputPos++] = line[linePos];
            updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
        }
        linePos += 1;
    }
    if (output != NULL) {
        output[outputPos] = '\0';
    }

    return celix_properties_set(props, celix_utils_trimInPlace(key), celix_utils_trimInPlace(value));
}

celix_properties_t* celix_properties_loadWithStream(FILE* file) {
    if (file == NULL) {
        return NULL;
    }


    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (!props) {
        celix_err_push("Failed to create properties");
        return NULL;
    }

    int rc = fseek(file, 0, SEEK_END);
    if (rc != 0) {
        celix_err_pushf("Cannot seek to end of file. Got error %i", errno);
        return NULL;
    }

    long fileSize = ftell(file);
    if (fileSize < 0) {
        celix_err_pushf("Cannot get file size. Got error %i", errno);
        return NULL;
    }

    rc = fseek(file, 0, SEEK_SET);
    if (rc != 0) {
        celix_err_pushf("Cannot seek to start of file. Got error %i", errno);
        return NULL;
    }

    celix_autofree char* fileBuffer = malloc(fileSize + 1);
    if (fileBuffer == NULL) {
        celix_err_pushf("Cannot allocate memory for file buffer. Got error %i", errno);
        return NULL;
    }

    size_t rs = fread(fileBuffer, sizeof(char), fileSize, file);
    if (rs < fileSize) {
        celix_err_pushf("fread read only %zu bytes out of %zu\n", rs, fileSize);
        return NULL;
    }
    fileBuffer[fileSize] = '\0'; // ensure a '\0' at the end of the fileBuffer

    char* savePtr = NULL;
    char* line = strtok_r(fileBuffer, "\n", &savePtr);
    while (line != NULL) {
        celix_status_t status = celix_properties_parseLine(line, props);
        if (status != CELIX_SUCCESS) {
            celix_err_pushf("Failed to parse line '%s'", line);
            return NULL;
        }
        line = strtok_r(NULL, "\n", &savePtr);
    }

    return celix_steal_ptr(props);
}

celix_properties_t* celix_properties_loadFromString(const char* input) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_autofree char* in = celix_utils_strdup(input);
    if (!props || !in) {
        celix_err_push("Failed to create properties or duplicate input string");
        return NULL;
    }

    char* line = NULL;
    char* saveLinePointer = NULL;
    line = strtok_r(in, "\n", &saveLinePointer);
    while (line != NULL) {
        celix_status_t status = celix_properties_parseLine(line, props);
        if (status != CELIX_SUCCESS) {
            celix_err_pushf("Failed to parse line '%s'", line);
            return NULL;
        }
        line = strtok_r(NULL, "\n", &saveLinePointer);
    }
    return celix_steal_ptr(props);
}

/**
 * @brief Store properties string to file and escape the characters '#', '!', '=' and ':' if encountered.
 */
static int celix_properties_storeEscapedString(FILE* file, const char* str) {
    int rc = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len && rc != EOF; i += 1) {
        if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
            rc = fputc('\\', file);
            if (rc == EOF) {
                continue;
            }
        }
        rc = fputc(str[i], file);
    }
    return rc;
}

celix_status_t celix_properties_store(celix_properties_t* properties, const char* filename, const char* header) {
    FILE* file = fopen(filename, "w+");

    if (file == NULL) {
        celix_err_pushf("Cannot open file '%s'", filename);
        return CELIX_FILE_IO_EXCEPTION;
    }

    int rc = 0;

    if (header && strstr("\n", header)) {
        celix_err_push("Header cannot contain newlines. Ignoring header.");
    } else if (header) {
        rc = fputc('#', file);
        if (rc != EOF) {
            rc = fputs(header, file);
        }
        if (rc != EOF) {
            rc = fputc('\n', file);
        }
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        const char* val = iter.entry.value;
        if (rc != EOF) {
            rc = celix_properties_storeEscapedString(file, iter.key);
        }
        if (rc != EOF) {
            rc = fputc('=', file);
        }
        if (rc != EOF) {
            rc = celix_properties_storeEscapedString(file, val);
        }
        if (rc != EOF) {
            rc = fputc('\n', file);
        }
    }
    if (rc != EOF) {
        rc = fclose(file);
    } else {
        fclose(file);
    }
    if (rc == EOF) {
        celix_err_push("Failed to write properties to file");
        return CELIX_FILE_IO_EXCEPTION;
    }
    return CELIX_SUCCESS;
}

celix_properties_t* celix_properties_copy(const celix_properties_t* properties) {
    celix_properties_t* copy = celix_properties_create();

    if (!copy) {
        celix_err_push("Failed to create properties copy");
        return NULL;
    }

    if (!properties) {
        return copy;
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        celix_status_t status;
        if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING) {
            status = celix_properties_set(copy, iter.key, iter.entry.value);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG) {
            status = celix_properties_setLong(copy, iter.key, iter.entry.typed.longValue);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
            status = celix_properties_setDouble(copy, iter.key, iter.entry.typed.doubleValue);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL) {
            status = celix_properties_setBool(copy, iter.key, iter.entry.typed.boolValue);
        } else /*version*/ {
            assert(iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION);
            status = celix_properties_setVersion(copy, iter.key, iter.entry.typed.versionValue);
        }
        if (status != CELIX_SUCCESS) {
            celix_err_pushf("Failed to copy property %s", iter.key);
            celix_properties_destroy(copy);
            return NULL;
        }
    }
    return copy;
}

celix_properties_value_type_e celix_properties_getType(const celix_properties_t* properties, const char* key) {
    celix_properties_entry_t* entry = celix_stringHashMap_get(properties->map, key);
    return entry == NULL ? CELIX_PROPERTIES_VALUE_TYPE_UNSET : entry->valueType;
}

const char* celix_properties_get(const celix_properties_t* properties, const char* key, const char* defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(properties, key);
    if (entry != NULL) {
        return entry->value;
    }
    return defaultValue;
}

celix_properties_entry_t* celix_properties_getEntry(const celix_properties_t* properties, const char* key) {
    celix_properties_entry_t* entry = NULL;
    if (properties) {
        entry = celix_stringHashMap_get(properties->map, key);
    }
    return entry;
}

celix_status_t celix_properties_set(celix_properties_t* properties, const char* key, const char* value) {
    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_STRING;
    prototype.typed.strValue = value;
    return celix_properties_createAndSetEntry(properties, key, &prototype);
}

celix_status_t celix_properties_setWithoutCopy(celix_properties_t* properties, char* key, char* value) {
    if (properties) {
        if (!key || !value) {
            celix_err_push("Failed to set (without copy) property. Key or value is NULL.");
            free(key);
            free(value);
            return CELIX_ILLEGAL_ARGUMENT;
        }
        celix_properties_entry_t* entry = celix_properties_createEntryWithNoCopy(properties, value);
        if (!entry) {
            celix_err_push("Failed to create entry for property.");
            free(key);
            free(value);
            return CELIX_ENOMEM;
        }

        bool alreadyExist = celix_stringHashMap_hasKey(properties->map, key);
        celix_status_t status = celix_stringHashMap_put(properties->map, key, entry);
        if (status != CELIX_SUCCESS) {
            celix_err_pushf("Failed to put entry for key %s in map.", key);
            free(key);
            celix_properties_destroyEntry(properties, entry);
        } else if (alreadyExist) {
            free(key);
        }
        return status;
    }
    return CELIX_SUCCESS; // silently ignore NULL properties, key or value
}

celix_status_t
celix_properties_setEntry(celix_properties_t* properties, const char* key, const celix_properties_entry_t* entry) {
    if (entry) {
        switch (entry->valueType) {
        case CELIX_PROPERTIES_VALUE_TYPE_LONG:
            return celix_properties_setLong(properties, key, entry->typed.longValue);
        case CELIX_PROPERTIES_VALUE_TYPE_DOUBLE:
            return celix_properties_setDouble(properties, key, entry->typed.doubleValue);
        case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
            return celix_properties_setBool(properties, key, entry->typed.boolValue);
        case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
            return celix_properties_setVersion(properties, key, entry->typed.versionValue);
        default: // STRING
            return celix_properties_set(properties, key, entry->typed.strValue);
        }
    }
    return CELIX_SUCCESS; // silently ignore NULL entry
}

static bool celix_properties_entryEquals(const celix_properties_entry_t* entry1,
                                         const celix_properties_entry_t* entry2) {
    if (entry1->valueType != entry2->valueType) {
        return false;
    }

    switch (entry1->valueType) {
    case CELIX_PROPERTIES_VALUE_TYPE_LONG:
        return entry1->typed.longValue == entry2->typed.longValue;
    case CELIX_PROPERTIES_VALUE_TYPE_DOUBLE:
        return entry1->typed.doubleValue == entry2->typed.doubleValue;
    case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
        return entry1->typed.boolValue == entry2->typed.boolValue;
    case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
        return celix_version_compareTo(entry1->typed.versionValue, entry2->typed.versionValue) == 0;
    default: // STRING
        return strcmp(entry1->value, entry2->value) == 0;
    }
}

void celix_properties_unset(celix_properties_t* properties, const char* key) {
    if (properties != NULL) {
        celix_stringHashMap_remove(properties->map, key);
    }
}

long celix_properties_getAsLong(const celix_properties_t* props, const char* key, long defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG) {
        return entry->typed.longValue;
    } else if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
        return (long)entry->typed.doubleValue;
    } else if (entry != NULL) {
        return celix_utils_convertStringToLong(entry->value, defaultValue, NULL);
    }
    return defaultValue;
}

celix_status_t celix_properties_setLong(celix_properties_t* props, const char* key, long value) {
    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_LONG;
    prototype.typed.longValue = value;
    return celix_properties_createAndSetEntry(props, key, &prototype);
}

double celix_properties_getAsDouble(const celix_properties_t* props, const char* key, double defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
        return entry->typed.doubleValue;
    } else if (entry != NULL) {
        return celix_utils_convertStringToDouble(entry->value, defaultValue, NULL);
    }
    return defaultValue;
}

celix_status_t celix_properties_setDouble(celix_properties_t* props, const char* key, double val) {
    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_DOUBLE;
    prototype.typed.doubleValue = val;
    return celix_properties_createAndSetEntry(props, key, &prototype);
}

bool celix_properties_getAsBool(const celix_properties_t* props, const char* key, bool defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL) {
        return entry->typed.boolValue;
    } else if (entry != NULL) {
        return celix_utils_convertStringToBool(entry->value, defaultValue, NULL);
    }
    return defaultValue;
}

celix_status_t celix_properties_setBool(celix_properties_t* props, const char* key, bool val) {
    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_BOOL;
    prototype.typed.boolValue = val;
    return celix_properties_createAndSetEntry(props, key, &prototype);
}

const celix_version_t* celix_properties_peekVersion(const celix_properties_t* properties,
                                                   const char* key,
                                                   const celix_version_t* defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(properties, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        return entry->typed.versionValue;
    }
    return defaultValue;
}

celix_version_t* celix_properties_getAsVersion(const celix_properties_t* properties,
                                               const char* key,
                                               const celix_version_t* defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(properties, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        return celix_version_copy(entry->typed.versionValue);
    }
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING) {
        celix_version_t* createdVersion = celix_version_createVersionFromString(entry->value);
        if (createdVersion != NULL) {
            return createdVersion;
        }
    }
    return defaultValue == NULL ? NULL : celix_version_copy(defaultValue);
}

celix_status_t
celix_properties_setVersion(celix_properties_t* props, const char* key, const celix_version_t* version) {
    celix_version_t* copy = celix_version_copy(version);
    if (copy == NULL) {
        celix_err_push("Failed to copy version");
        return CELIX_ENOMEM;
    }

    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_VERSION;
    prototype.typed.versionValue = copy;
    return celix_properties_createAndSetEntry(props, key, &prototype);
}

celix_status_t
celix_properties_assignVersion(celix_properties_t* properties, const char* key, celix_version_t* version) {
    celix_properties_entry_t prototype = {0};
    prototype.valueType = CELIX_PROPERTIES_VALUE_TYPE_VERSION;
    prototype.typed.versionValue = version;
    return celix_properties_createAndSetEntry(properties, key, &prototype);
}

size_t celix_properties_size(const celix_properties_t* properties) {
    return celix_stringHashMap_size(properties->map);
}

bool celix_properties_equals(const celix_properties_t* props1, const celix_properties_t* props2) {
    if (props1 == props2) {
        return true;
    }
    if (props1 == NULL && props2 == NULL) {
        return true;
    }
    if (props1 == NULL || props2 == NULL) {
        return false;
    }
    if (celix_properties_size(props1) != celix_properties_size(props2)) {
        return false;
    }
    CELIX_PROPERTIES_ITERATE(props1, iter) {
        celix_properties_entry_t* entry2 = celix_properties_getEntry(props2, iter.key);
        if (entry2 == NULL || !celix_properties_entryEquals(&iter.entry, entry2)) {
            return false;
        }
    }
    return true;
}

typedef struct {
    celix_string_hash_map_iterator_t mapIter;
    const celix_properties_t* props;
} celix_properties_iterator_internal_t;

celix_properties_iterator_t celix_properties_begin(const celix_properties_t* properties) {
    celix_properties_iterator_t iter;
    celix_properties_iterator_internal_t internalIter;

    CELIX_BUILD_ASSERT(sizeof(celix_properties_iterator_internal_t) <= sizeof(iter._data));

    internalIter.mapIter = celix_stringHashMap_begin(properties->map);
    internalIter.props = properties;

    if (celix_stringHashMapIterator_isEnd(&internalIter.mapIter)) {
        iter.key = NULL;
        memset(&iter.entry, 0, sizeof(iter.entry));
    } else {
        iter.key = internalIter.mapIter.key;
        memcpy(&iter.entry, internalIter.mapIter.value.ptrValue, sizeof(iter.entry));
    }

    memset(&iter._data, 0, sizeof(iter._data));
    memcpy(iter._data, &internalIter, sizeof(internalIter));
    return iter;
}

celix_properties_iterator_t celix_properties_end(const celix_properties_t* properties) {
    celix_properties_iterator_internal_t internalIter;
    internalIter.mapIter = celix_stringHashMap_end(properties->map);
    internalIter.props = properties;

    celix_properties_iterator_t iter;
    memset(&iter, 0, sizeof(iter));
    memcpy(iter._data, &internalIter, sizeof(internalIter));
    return iter;
}

void celix_propertiesIterator_next(celix_properties_iterator_t* iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter->_data, sizeof(internalIter));
    celix_stringHashMapIterator_next(&internalIter.mapIter);
    memcpy(iter->_data, &internalIter, sizeof(internalIter));
    if (celix_stringHashMapIterator_isEnd(&internalIter.mapIter)) {
        iter->key = NULL;
        memset(&iter->entry, 0, sizeof(iter->entry));
    } else {
        iter->key = internalIter.mapIter.key;
        memcpy(&iter->entry, internalIter.mapIter.value.ptrValue, sizeof(iter->entry));
    }
}

bool celix_propertiesIterator_isEnd(const celix_properties_iterator_t* iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter->_data, sizeof(internalIter));
    return celix_stringHashMapIterator_isEnd(&internalIter.mapIter);
}

bool celix_propertiesIterator_equals(const celix_properties_iterator_t* a, const celix_properties_iterator_t* b) {
    celix_properties_iterator_internal_t internalIterA;
    memcpy(&internalIterA, a->_data, sizeof(internalIterA));
    celix_properties_iterator_internal_t internalIterB;
    memcpy(&internalIterB, b->_data, sizeof(internalIterB));
    return celix_stringHashMapIterator_equals(&internalIterA.mapIter, &internalIterB.mapIter);
}

celix_properties_statistics_t celix_properties_getStatistics(const celix_properties_t* properties) {
    size_t sizeOfKeysAndStringValues = 0;
    CELIX_PROPERTIES_ITERATE(properties, iter) {
        sizeOfKeysAndStringValues += celix_utils_strlen(iter.key) + 1;
        sizeOfKeysAndStringValues += celix_utils_strlen(iter.entry.value) + 1;
    }

    celix_properties_statistics_t stats;
    stats.sizeOfKeysAndStringValues = sizeOfKeysAndStringValues;
    stats.averageSizeOfKeysAndStringValues = (double)sizeOfKeysAndStringValues / (double)celix_properties_size(properties) * 2;
    stats.fillStringOptimizationBufferPercentage = (double)properties->currentStringBufferIndex / CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE;
    stats.fillEntriesOptimizationBufferPercentage = (double)properties->currentEntriesBufferIndex / CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE;
    stats.mapStatistics = celix_stringHashMap_getStatistics(properties->map);
    return stats;
}
