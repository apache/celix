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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "celix_build_assert.h"
#include "celix_utils.h"
#include "celix_string_hash_map.h"


#define CELIX_SHORT_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE 1024
#define CELIX_SHORT_PROPERTIES_OPTIMIZATION_ENTRIES_SIZE 16

static const char* const CELIX_PROPERTIES_BOOL_TRUE_STRVAL = "true";
static const char* const CELIX_PROPERTIES_BOOL_FALSE_STRVAL = "false";

struct celix_properties {
    celix_string_hash_map_t* map;

    /**
     * String buffer used to store the first key/value entries, so that in many cases additional memory allocation
     * can be prevented.
     */
    char stringBuffer[CELIX_SHORT_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE];

    /**
     * The current string buffer index.
     */
    int currentStringBufferIndex;

    /**
     * Entries buffer used to store the first entries, so that in many cases additional memory allocation
     * can be prevented.
     */
    celix_properties_entry_t entriesBuffer[CELIX_SHORT_PROPERTIES_OPTIMIZATION_ENTRIES_SIZE];

    /**
     * The current string buffer index.
     */
    int currentEntriesBufferIndex;
};

#define MALLOC_BLOCK_SIZE        5

static void celix_properties_parseLine(const char* line, celix_properties_t *props);

properties_pt properties_create(void) {
    return celix_properties_create();
}

void properties_destroy(properties_pt properties) {
    celix_properties_destroy(properties);
}

properties_pt properties_load(const char* filename) {
    return celix_properties_load(filename);
}

properties_pt properties_loadWithStream(FILE *file) {
    return celix_properties_loadWithStream(file);
}

properties_pt properties_loadFromString(const char *input){
    return celix_properties_loadFromString(input);
}


/**
 * Header is ignored for now, cannot handle comments yet
 */
void properties_store(properties_pt properties, const char* filename, const char* header) {
    celix_properties_store(properties, filename, header);
}

celix_status_t properties_copy(properties_pt properties, properties_pt *out) {
    celix_properties_t *copy = celix_properties_copy(properties);
    *out = copy;
    return copy == NULL ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
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

void properties_unset(properties_pt properties, const char* key) {
    celix_properties_unset(properties, key);
}

static void updateBuffers(char **key, char ** value, char **output, int outputPos, int *key_len, int *value_len) {
    if (*output == *key) {
        if (outputPos == (*key_len) - 1) {
            (*key_len) += MALLOC_BLOCK_SIZE;
            *key = realloc(*key, *key_len);
            *output = *key;
        }
    }
    else {
        if (outputPos == (*value_len) - 1) {
            (*value_len) += MALLOC_BLOCK_SIZE;
            *value = realloc(*value, *value_len);
            *output = *value;
        }
    }
}

/**
 * Create a new string from the provided str by either using strup or storing the string the short properties
 * optimization string buffer.
 */
static char* celix_properties_createString(celix_properties_t* properties, const char* str) {
    size_t len = str == NULL ? 0 : strnlen(str, CELIX_UTILS_MAX_STRLEN) + 1;
    size_t left = CELIX_SHORT_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE - properties->currentStringBufferIndex;
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
 * Fill entry and optional use the short properties optimization string buffer.
 */
static celix_status_t celix_properties_fillEntry(
        celix_properties_t *properties,
        celix_properties_entry_t* entry,
        const char *strValue,
        const long* longValue,
        const double* doubleValue,
        const bool* boolValue,
        celix_version_t* versionValue) {
    char convertedValueBuffer[32];
    if (strValue != NULL) {
        entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_STRING;
        entry->value = celix_properties_createString(properties, strValue);
        entry->typed.strValue = entry->value;
    } else if (longValue != NULL) {
        entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_LONG;
        entry->typed.longValue = *longValue;
        int written = snprintf(convertedValueBuffer, sizeof(convertedValueBuffer), "%li", entry->typed.longValue);
        if (written < 0 || written >= sizeof(convertedValueBuffer)) {
            entry->value = celix_properties_createString(properties, convertedValueBuffer);
        } else {
            char* val = NULL;
            asprintf(&val, "%li", entry->typed.longValue);
            entry->value = val;
        }
    } else if (doubleValue != NULL) {
        entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_DOUBLE;
        entry->typed.doubleValue = *doubleValue;
        int written = snprintf(convertedValueBuffer, sizeof(convertedValueBuffer), "%f", entry->typed.doubleValue);
        if (written < 0 || written >= sizeof(convertedValueBuffer)) {
            entry->value = celix_properties_createString(properties, convertedValueBuffer);
        } else {
            char* val = NULL;
            asprintf(&val, "%f", entry->typed.doubleValue);
            entry->value = val;
        }
    } else if (boolValue != NULL) {
        entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_BOOL;
        entry->typed.boolValue = *boolValue;
        entry->value = entry->typed.boolValue ? CELIX_PROPERTIES_BOOL_TRUE_STRVAL : CELIX_PROPERTIES_BOOL_FALSE_STRVAL;
    } else /*versionValue*/ {
        assert(versionValue != NULL);
        entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_VERSION;
        entry->typed.versionValue = versionValue;
        bool written = celix_version_fillString(versionValue, convertedValueBuffer, sizeof(convertedValueBuffer));
        if (written) {
            entry->value = celix_properties_createString(properties, convertedValueBuffer);
        } else {
            entry->value = celix_version_toString(versionValue);
        }
    }
    if (entry->value == NULL) {
        return CELIX_ENOMEM;
    }
    return CELIX_SUCCESS;
}

/**
 * Allocate entry and optionally use the short properties optimization entries buffer.
 */
static celix_properties_entry_t* celix_properties_allocEntry(celix_properties_t* properties) {
    celix_properties_entry_t* entry;
    if (properties->currentEntriesBufferIndex < CELIX_SHORT_PROPERTIES_OPTIMIZATION_ENTRIES_SIZE) {
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
static celix_properties_entry_t* celix_properties_createEntryWithNoCopy(celix_properties_t *properties,
                                                                        char *strValue) {
    celix_properties_entry_t* entry = celix_properties_allocEntry(properties);
    if (entry == NULL) {
        return NULL;
    }
    entry->value = strValue;
    entry->valueType = CELIX_PROPERTIES_VALUE_TYPE_STRING;
    entry->typed.strValue = strValue;
    return entry;
}


/**
 * Create entry and optionally use the short properties optimization buffers.
 * Only 1 of the types values (strValue, LongValue, etc) should be provided.
 */
static celix_properties_entry_t* celix_properties_createEntry(
        celix_properties_t *properties,
        const char *key,
        const char *strValue,
        const long* longValue,
        const double* doubleValue,
        const bool* boolValue,
        celix_version_t* versionValue) {
    celix_properties_entry_t* entry = celix_properties_allocEntry(properties);
    if (entry == NULL) {
        return NULL;
    }

    celix_status_t status = celix_properties_fillEntry(properties, entry, strValue, longValue, doubleValue,
                                                       boolValue, versionValue);
    if (status != CELIX_SUCCESS) {
        free(entry);
        entry = NULL;
    }
    return entry;
}

/**
 * Create and add entry and optionally use the short properties optimization buffers.
 * Only 1 of the types values (strValue, LongValue, etc) should be provided.
 */
static void celix_properties_createAndSetEntry(
        celix_properties_t *properties,
        const char *key,
        const char *strValue,
        const long* longValue,
        const double* doubleValue,
        const bool* boolValue,
        celix_version_t* versionValue) {
    if (properties == NULL) {
        return;
    }
    celix_properties_entry_t* entry = celix_properties_createEntry(properties, key, strValue, longValue, doubleValue,
                                                                   boolValue, versionValue);
    const char* mapKey = key;
    if (!celix_stringHashMap_hasKey(properties->map, key)) {
        //new entry, needs new allocated key;
        mapKey = celix_properties_createString(properties, key);
    }
    if (entry != NULL) {
        celix_stringHashMap_put(properties->map, mapKey, entry);
    }
}



static void celix_properties_freeString(celix_properties_t* properties, char* str) {
    if (str == CELIX_PROPERTIES_BOOL_TRUE_STRVAL || str == CELIX_PROPERTIES_BOOL_FALSE_STRVAL) {
        //str is static const char* const -> nop
    } else if (str >= properties->stringBuffer &&
               str < (properties->stringBuffer + CELIX_SHORT_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE))   {
        //str is part of the properties string buffer -> nop
    } else {
        free(str);
    }
}

static void celix_properties_removeKeyCallback(void* handle, char* key) {
    celix_properties_t* properties = handle;
    celix_properties_freeString(properties, key);
}

static void celix_properties_removeEntryCallback(void* handle, const char* key __attribute__((unused)), celix_hash_map_value_t val) {
    celix_properties_t* properties = handle;
    celix_properties_entry_t* entry = val.ptrValue;
    celix_properties_freeString(properties, (char*)entry->value);
    if (entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        celix_version_destroy(entry->typed.versionValue);
    }
    if (entry >= properties->entriesBuffer &&
            entry <= (properties->entriesBuffer + CELIX_SHORT_PROPERTIES_OPTIMIZATION_ENTRIES_SIZE)) {
        //entry is part of the properties entries buffer -> nop.
    } else {
        free(entry);
    }
}

celix_properties_t* celix_properties_create(void) {
    celix_properties_t* props = malloc(sizeof(*props));
    if (props != NULL) {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.storeKeysWeakly = true;
        opts.initialCapacity = (unsigned int)ceil(CELIX_SHORT_PROPERTIES_OPTIMIZATION_ENTRIES_SIZE / 0.75);
        opts.removedCallbackData = props;
        opts.removedCallback = celix_properties_removeEntryCallback;
        opts.removedKeyCallback = celix_properties_removeKeyCallback;
        props->map = celix_stringHashMap_createWithOptions(&opts);
        props->currentStringBufferIndex = 0;
        props->currentEntriesBufferIndex = 0;
    }
    return props;
}

void celix_properties_destroy(celix_properties_t *props) {
    if (props != NULL) {
        //TODO measure print nr of entries and total size of the string keys and values
//        fprintf(stdout, "Properties size; %d", celix_properties_size(props));
//        size_t size = 0;
//        CELIX_PROPERTIES_ITERATE(props, iter) {
//            size += strlen(iter.entry.key) + 1;
//            size += strlen(iter.entry.value) + 1;
//        }
//        fprintf(stdout, "Properties string size: %zu", size);

        celix_stringHashMap_destroy(props->map);
        free(props);
    }
}

celix_properties_t* celix_properties_load(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    celix_properties_t *props = celix_properties_loadWithStream(file);
    fclose(file);
    return props;
}

static void celix_properties_parseLine(const char* line, celix_properties_t *props) {
    int linePos = 0;
    bool precedingCharIsBackslash = false;
    bool isComment = false;
    int outputPos = 0;
    char *output = NULL;
    int key_len = MALLOC_BLOCK_SIZE;
    int value_len = MALLOC_BLOCK_SIZE;
    linePos = 0;
    precedingCharIsBackslash = false;
    isComment = false;
    output = NULL;
    outputPos = 0;

    //Ignore empty lines
    if (line[0] == '\n' && line[1] == '\0') {
        return;
    }

    char *key = calloc(1, key_len);
    char *value = calloc(1, value_len);
    key[0] = '\0';
    value[0] = '\0';

    while (line[linePos] != '\0') {
        if (line[linePos] == ' ' || line[linePos] == '\t') {
            if (output == NULL) {
                //ignore
                linePos += 1;
                continue;
            }
        }
        else {
            if (output == NULL) {
                output = key;
            }
        }
        if (line[linePos] == '=' || line[linePos] == ':' || line[linePos] == '#' || line[linePos] == '!') {
            if (precedingCharIsBackslash) {
                //escaped special character
                output[outputPos++] = line[linePos];
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                precedingCharIsBackslash = false;
            }
            else {
                if (line[linePos] == '#' || line[linePos] == '!') {
                    if (outputPos == 0) {
                        isComment = true;
                        break;
                    }
                    else {
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    }
                }
                else { // = or :
                    if (output == value) { //already have a seperator
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    }
                    else {
                        output[outputPos++] = '\0';
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                        output = value;
                        outputPos = 0;
                    }
                }
            }
        }
        else if (line[linePos] == '\\') {
            if (precedingCharIsBackslash) { //double backslash -> backslash
                output[outputPos++] = '\\';
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
            }
            precedingCharIsBackslash = true;
        }
        else { //normal character
            precedingCharIsBackslash = false;
            output[outputPos++] = line[linePos];
            updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
        }
        linePos += 1;
    }
    if (output != NULL) {
        output[outputPos] = '\0';
    }

    if (!isComment) {
        celix_utils_trimInPlace(key);
        celix_utils_trimInPlace(value);
        celix_properties_setWithoutCopy(props, key, value);
    } else {
        free(key);
        free(value);
    }
}

celix_properties_t* celix_properties_loadWithStream(FILE *file) {
    if (file == NULL) {
        return NULL;
    }

    //TODO create properties with no internal short properties buffer, so celix_properties_createWithOptions()
    celix_properties_t *props = celix_properties_create();
    if (props == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fileSize == 0) {
        return props;
    }

    char* fileBuffer = malloc(fileSize + 1);
    if (fileBuffer == NULL) {
        celix_properties_destroy(props);
        return NULL;
    }

    size_t rs = fread(fileBuffer, sizeof(char), fileSize, file);
    if (rs < fileSize) {
        fprintf(stderr,"fread read only %zu bytes out of %zu\n", rs, fileSize);
    }
    fileBuffer[fileSize]='\0'; //ensure a '\0' at the end of the fileBuffer

    char* savePtr = NULL;
    char* line = strtok_r(fileBuffer, "\n", &savePtr);
    while (line != NULL) {
        celix_properties_parseLine(line, props);
        line = strtok_r(NULL, "\n", &savePtr);
    }
    free(fileBuffer);

    return props;
}

celix_properties_t* celix_properties_loadFromString(const char *input) {
    celix_properties_t *props = celix_properties_create();

    char *in = strdup(input);
    char *line = NULL;
    char *saveLinePointer = NULL;

    bool firstTime = true;
    do {
        if (firstTime){
            line = strtok_r(in, "\n", &saveLinePointer);
            firstTime = false;
        }else {
            line = strtok_r(NULL, "\n", &saveLinePointer);
        }

        if (line == NULL){
            break;
        }

        celix_properties_parseLine(line, props);
    } while(line != NULL);

    free(in);

    return props;
}

/**
 * @brief Store properties string to file and escape the characters '#', '!', '=' and ':' if encountered.
 */
static int celix_properties_storeEscapedString(FILE* file, const char* str) {
    int rc = 0;
    for (int i = 0; i < strlen(str); i += 1) {
        if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
            rc = fputc('\\', file);
            if (rc == EOF) {
                break;
            }
        }
        rc = fputc(str[i], file);
    }
    return rc;
}

celix_status_t celix_properties_store(celix_properties_t *properties, const char *filename, const char *header) {
    FILE *file = fopen (filename, "w+" );

    if (file == NULL) {
        return CELIX_FILE_IO_EXCEPTION;
    }

    int rc = 0;
    CELIX_STRING_HASH_MAP_ITERATE(properties->map, iter) {
        const char* val = iter.value.ptrValue;
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
    }
    return rc != EOF ? CELIX_SUCCESS : CELIX_FILE_IO_EXCEPTION;
}

celix_properties_t* celix_properties_copy(const celix_properties_t *properties) {
    celix_properties_t *copy = celix_properties_create();
    if (properties == NULL) {
        return copy;
    }

    CELIX_PROPERTIES_ITERATE(properties, iter) {
        if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING) {
            celix_properties_set(copy, iter.key, iter.entry.value);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG) {
            celix_properties_setLong(copy, iter.key, iter.entry.typed.longValue);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
            celix_properties_setDouble(copy, iter.key, iter.entry.typed.doubleValue);
        } else if (iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL) {
            celix_properties_setBool(copy, iter.key, iter.entry.typed.boolValue);
        } else /*version*/ {
            assert(iter.entry.valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION);
            celix_properties_setVersion(copy, iter.key, iter.entry.typed.versionValue);
        }
    }
    return copy;
}

celix_properties_value_type_e celix_properties_getType(const celix_properties_t* properties, const char* key) {
    celix_properties_entry_t* entry = celix_stringHashMap_get(properties->map, key);
    return entry == NULL ? CELIX_PROPERTIES_VALUE_TYPE_UNSET : entry->valueType;
}

const char* celix_properties_get(const celix_properties_t *properties, const char *key, const char *defaultValue) {
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

void celix_properties_set(celix_properties_t *properties, const char *key, const char *value) {
    if (properties != NULL && key != NULL && value != NULL) {
        celix_properties_createAndSetEntry(properties, key, value, NULL, NULL, NULL, NULL);
    }
}

void celix_properties_setWithoutCopy(celix_properties_t *properties, char *key, char *value) {
    if (properties != NULL && key != NULL && value != NULL) {
        celix_properties_entry_t* entry = celix_properties_createEntryWithNoCopy(properties, value);
        if (entry != NULL) {
            bool replaced = celix_stringHashMap_put(properties->map, key, entry);
            if (replaced) {
                free(key);
            }
        }
    }
}

void celix_properties_unset(celix_properties_t *properties, const char *key) {
    if (properties != NULL) {
        celix_stringHashMap_remove(properties->map, key);
    }
}

long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue) {
    long result = defaultValue;
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_LONG) {
        return entry->typed.longValue;
    } else if (entry != NULL) {
        char *enptr = NULL;
        errno = 0;
        long r = strtol(entry->value, &enptr, 10);
        if (enptr != entry->value && errno == 0) {
            result = r;
        }
    }
    return result;
}

void celix_properties_setLong(celix_properties_t *props, const char *key, long value) {
    celix_properties_createAndSetEntry(props, key, NULL, &value, NULL, NULL, NULL);
}

double celix_properties_getAsDouble(const celix_properties_t *props, const char *key, double defaultValue) {
    double result = defaultValue;
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_DOUBLE) {
        return entry->typed.doubleValue;
    } else if (entry != NULL) {
        char *enptr = NULL;
        errno = 0;
        double r = strtod(entry->value, &enptr);
        if (enptr != entry->value && errno == 0) {
            result = r;
        }
    }
    return result;
}

void celix_properties_setDouble(celix_properties_t *props, const char *key, double val) {
    celix_properties_createAndSetEntry(props, key, NULL, NULL, &val, NULL, NULL);
}

bool celix_properties_getAsBool(const celix_properties_t *props, const char *key, bool defaultValue) {
    bool result = defaultValue;
    celix_properties_entry_t* entry = celix_properties_getEntry(props, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_BOOL) {
        return entry->typed.boolValue;
    } else if (entry != NULL) {
        char buf[32];
        snprintf(buf, 32, "%s", entry->value);
        celix_utils_trimInPlace(buf);
        if (strncasecmp("true", buf, strlen("true")) == 0) {
            result = true;
        } else if (strncasecmp("false", buf, strlen("false")) == 0) {
            result = false;
        }
    }
    return result;
}

void celix_properties_setBool(celix_properties_t *props, const char *key, bool val) {
    celix_properties_createAndSetEntry(props, key, NULL, NULL, NULL, &val, NULL);
}

const celix_version_t* celix_properties_getVersion(
        const celix_properties_t* properties,
        const char* key,
        const celix_version_t* defaultValue) {
    celix_properties_entry_t* entry = celix_properties_getEntry(properties, key);
    if (entry != NULL && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_VERSION) {
        return entry->typed.versionValue;
    }
    return defaultValue;
}

celix_version_t* celix_properties_getAsVersion(
        const celix_properties_t* properties,
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

void celix_properties_setVersion(celix_properties_t *properties, const char *key, const celix_version_t* version) {
    celix_properties_createAndSetEntry(properties, key, NULL, NULL, NULL, NULL, celix_version_copy(version));
}

void celix_properties_setVersionWithoutCopy(celix_properties_t* properties, const char* key, celix_version_t* version) {
    celix_properties_createAndSetEntry(properties, key, NULL, NULL, NULL, NULL, version);
}

int celix_properties_size(const celix_properties_t *properties) {
    return (int)celix_stringHashMap_size(properties->map);
}

typedef struct {
    celix_string_hash_map_iterator_t mapIter;
    const celix_properties_t* props;
}  celix_properties_iterator_internal_t;

celix_properties_iterator_t celix_propertiesIterator_construct(const celix_properties_t *properties) {
    celix_properties_iterator_internal_t internalIter;
    internalIter.mapIter = celix_stringHashMap_begin(properties->map);
    internalIter.props = properties;

    celix_properties_iterator_t iter;
    iter.index = 0;
    iter.key = NULL;
    memset(&iter.entry, 0, sizeof(iter.entry));

    CELIX_BUILD_ASSERT(sizeof(celix_properties_iterator_internal_t) <= sizeof(iter._data));
    memset(&iter._data, 0, sizeof(iter._data));
    memcpy(iter._data, &internalIter, sizeof(internalIter));
    return iter;
}

bool celix_propertiesIterator_hasNext(celix_properties_iterator_t *iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter->_data, sizeof(internalIter));
    return !celix_stringHashMapIterator_isEnd(&internalIter.mapIter);
}

const char* celix_propertiesIterator_nextKey(celix_properties_iterator_t *iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter->_data, sizeof(internalIter));

    //note assigning key first and then move the next, because celix string hash map iter start at the beginning
    const char* key = internalIter.mapIter.key;
    iter->index = (int)internalIter.mapIter.index;
    celix_properties_entry_t* entry = internalIter.mapIter.value.ptrValue;
    if (entry != NULL) {
        iter->key = internalIter.mapIter.key;
        memcpy(&iter->entry, entry, sizeof(iter->entry));
    } else {
        iter->key = NULL;
        memset(&iter->entry, 0, sizeof(iter->entry));
    }
    celix_stringHashMapIterator_next(&internalIter.mapIter);

    memcpy(iter->_data, &internalIter, sizeof(internalIter));
    return key;
}

celix_properties_iterator_t celix_properties_begin(const celix_properties_t* properties) {
    CELIX_BUILD_ASSERT(sizeof(celix_properties_iterator_internal_t) <= sizeof(celix_properties_iterator_t));
    celix_properties_iterator_internal_t internalIter;
    internalIter.mapIter = celix_stringHashMap_begin(properties->map);
    internalIter.props = properties;

    celix_properties_iterator_t iter;
    iter.index = 0;
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
    iter.index = internalIter.mapIter.index;
    memcpy(iter._data, &internalIter, sizeof(internalIter));
    return iter;
}

void celix_propertiesIterator_next(celix_properties_iterator_t *iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter->_data, sizeof(internalIter));
    celix_stringHashMapIterator_next(&internalIter.mapIter);
    memcpy(iter->_data, &internalIter, sizeof(internalIter));
    iter->index = internalIter.mapIter.index;
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

celix_properties_t* celix_propertiesIterator_properties(const celix_properties_iterator_t *iter) {
    celix_properties_iterator_internal_t internalIter;
    memcpy(&internalIter, iter, sizeof(internalIter));
    return (celix_properties_t*)internalIter.props;
}
