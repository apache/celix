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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "celixbool.h"
#include "properties.h"
#include "celix_properties.h"
#include "utils.h"
#include "hash_map_private.h"
#include <errno.h>

#define MALLOC_BLOCK_SIZE        5

static void parseLine(const char* line, celix_properties_t *props);

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
    return celix_properties_store(properties, filename, header);
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

static void parseLine(const char* line, celix_properties_t *props) {
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
        //printf("putting 'key'/'value' '%s'/'%s' in properties\n", utils_stringTrim(key), utils_stringTrim(value));
        celix_properties_set(props, utils_stringTrim(key), utils_stringTrim(value));
    }
    if(key) {
        free(key);
    }
    if(value) {
        free(value);
    }

}



/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/



celix_properties_t* celix_properties_create(void) {
    return hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
}

void celix_properties_destroy(celix_properties_t *properties) {
    if (properties != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(properties);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
            hashMapEntry_clear(entry, true, true);
        }
        hashMapIterator_destroy(iter);
        hashMap_destroy(properties, false, false);
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

celix_properties_t* celix_properties_loadWithStream(FILE *file) {
    celix_properties_t *props = NULL;

    if (file != NULL ) {
        char *saveptr;
        char *filebuffer = NULL;
        char *line = NULL;
        size_t file_size = 0;

        props = celix_properties_create();
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size > 0) {
            filebuffer = calloc(file_size + 1, sizeof(char));
            if (filebuffer) {
                size_t rs = fread(filebuffer, sizeof(char), file_size, file);
                if (rs != file_size) {
                    fprintf(stderr,"fread read only %lu bytes out of %lu\n", (long unsigned int) rs, (long unsigned int) file_size);
                }
                filebuffer[file_size]='\0';
                line = strtok_r(filebuffer, "\n", &saveptr);
                while (line != NULL) {
                    parseLine(line, props);
                    line = strtok_r(NULL, "\n", &saveptr);
                }
                free(filebuffer);
            }
        }
    }

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

        parseLine(line, props);
    } while(line != NULL);

    free(in);

    return props;
}

void celix_properties_store(celix_properties_t *properties, const char *filename, const char *header) {
    FILE *file = fopen (filename, "w+" );
    char *str;

    if (file != NULL) {
        if (hashMap_size(properties) > 0) {
            hash_map_iterator_pt iterator = hashMapIterator_create(properties);
            while (hashMapIterator_hasNext(iterator)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
                str = hashMapEntry_getKey(entry);
                for (int i = 0; i < strlen(str); i += 1) {
                    if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
                        fputc('\\', file);
                    }
                    fputc(str[i], file);
                }

                fputc('=', file);

                str = hashMapEntry_getValue(entry);
                for (int i = 0; i < strlen(str); i += 1) {
                    if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
                        fputc('\\', file);
                    }
                    fputc(str[i], file);
                }

                fputc('\n', file);

            }
            hashMapIterator_destroy(iterator);
        }
        fclose(file);
    } else {
        perror("File is null");
    }
}

celix_properties_t* celix_properties_copy(const celix_properties_t *properties) {
    celix_properties_t *copy = celix_properties_create();
    if (properties != NULL) {
        hash_map_iterator_t iter = hashMapIterator_construct((hash_map_t*)properties);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            char *key = hashMapEntry_getKey(entry);
            char *value = hashMapEntry_getValue(entry);
            celix_properties_set(copy, key, value);
        }
    }
    return copy;
}

const char* celix_properties_get(const celix_properties_t *properties, const char *key, const char *defaultValue) {
    const char* value = NULL;
    if (properties != NULL) {
        value = hashMap_get((hash_map_t*)properties, (void*)key);
    }
    return value == NULL ? defaultValue : value;
}

void celix_properties_set(celix_properties_t *properties, const char *key, const char *value) {
    if (properties != NULL) {
        hash_map_entry_pt entry = hashMap_getEntry(properties, key);
        char *oldVal = NULL;
        char *newVal = value == NULL ? NULL : strndup(value, 1024 * 1024);
        if (entry != NULL) {
            char *oldKey = hashMapEntry_getKey(entry);
            oldVal = hashMapEntry_getValue(entry);
            hashMap_put(properties, oldKey, newVal);
        } else {
            hashMap_put(properties, strndup(key, 1024 * 1024), newVal);
        }
        free(oldVal);
    }
}

void celix_properties_setWithoutCopy(celix_properties_t *properties, char *key, char *value) {
    if (properties != NULL) {
        hash_map_entry_pt entry = hashMap_getEntry(properties, key);
        char *oldVal = NULL;
        if (entry != NULL) {
            char *oldKey = hashMapEntry_getKey(entry);
            oldVal = hashMapEntry_getValue(entry);
            hashMap_put(properties, oldKey, value);
        } else {
            hashMap_put(properties, key, value);
        }
        free(oldVal);
    }
}

void celix_properties_unset(celix_properties_t *properties, const char *key) {
    char* oldValue = hashMap_removeFreeKey(properties, key);
    free(oldValue);
}

long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue) {
    long result = defaultValue;
    const char *val = celix_properties_get(props, key, NULL);
    if (val != NULL) {
        char *enptr = NULL;
        errno = 0;
        long r = strtol(val, &enptr, 10);
        if (enptr != val && errno == 0) {
            result = r;
        }
    }
    return result;
}

void celix_properties_setLong(celix_properties_t *props, const char *key, long value) {
    char buf[32]; //should be enough to store long long int
    int writen = snprintf(buf, 32, "%li", value);
    if (writen <= 31) {
        celix_properties_set(props, key, buf);
    } else {
        fprintf(stderr,"buf to small for value '%li'\n", value);
    }
}

double celix_properties_getAsDouble(const celix_properties_t *props, const char *key, double defaultValue) {
    double result = defaultValue;
    const char *val = celix_properties_get(props, key, NULL);
    if (val != NULL) {
        char *enptr = NULL;
        errno = 0;
        double r = strtod(val, &enptr);
        if (enptr != val && errno == 0) {
            result = r;
        }
    }
    return result;
}

void celix_properties_setDouble(celix_properties_t *props, const char *key, double val) {
    char buf[32]; //should be enough to store long long int
    int writen = snprintf(buf, 32, "%f", val);
    if (writen <= 31) {
        celix_properties_set(props, key, buf);
    } else {
        fprintf(stderr,"buf to small for value '%f'\n", val);
    }
}

bool celix_properties_getAsBool(const celix_properties_t *props, const char *key, bool defaultValue) {
    bool result = defaultValue;
    const char *val = celix_properties_get(props, key, NULL);
    if (val != NULL) {
        char buf[32];
        snprintf(buf, 32, "%s", val);
        char *trimmed = utils_stringTrim(buf);
        if (strncasecmp("true", trimmed, strlen("true")) == 0) {
            result = true;
        } else if (strncasecmp("false", trimmed, strlen("false")) == 0) {
            result = false;
        }
    }
    return result;
}

void celix_properties_setBool(celix_properties_t *props, const char *key, bool val) {
    celix_properties_set(props, key, val ? "true" : "false");
}

int celix_properties_size(const celix_properties_t *properties) {
    return hashMap_size((hash_map_t*)properties);
}

celix_properties_iterator_t celix_propertiesIterator_construct(const celix_properties_t *properties) {
    return hashMapIterator_construct((hash_map_t*)properties);
}
bool celix_propertiesIterator_hasNext(celix_properties_iterator_t *iter) {
    return hashMapIterator_hasNext(iter);
}
const char* celix_propertiesIterator_nextKey(celix_properties_iterator_t *iter) {
    return (const char*)hashMapIterator_nextKey(iter);
}
