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
/**
 * manifest.c
 *
 *  \date       Jul 5, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "celix_err.h"
#include "celix_errno.h"
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "manifest.h"
#include "utils.h"

celix_status_t manifest_create(manifest_pt *manifest) {
    celix_status_t status = CELIX_SUCCESS;
    do {
        celix_autofree manifest_pt manifestPtr = NULL;
        manifestPtr = malloc(sizeof(**manifest));
        if (manifestPtr == NULL) {
            status = CELIX_ENOMEM;
            break;
        }
        celix_autoptr(celix_properties_t) mainAttributes = celix_properties_create();
        if (mainAttributes == NULL) {
            status = CELIX_ENOMEM;
            break;
        }
        manifestPtr->attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        if (manifestPtr->attributes == NULL) {
            status = CELIX_ENOMEM;
            break;
        }
        manifestPtr->mainAttributes = celix_steal_ptr(mainAttributes);
        *manifest = celix_steal_ptr(manifestPtr);
    } while(false);

    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Cannot create manifest: %s", celix_strerror(status));
    }
    return status;
}

manifest_pt manifest_clone(manifest_pt manifest) {
    celix_status_t status = CELIX_SUCCESS;

    celix_auto(manifest_pt) clone = NULL;
    status = manifest_create(&clone);
    if (status == CELIX_SUCCESS) {
        CELIX_PROPERTIES_ITERATE(manifest->mainAttributes, visit) {
            celix_properties_set(clone->mainAttributes, visit.key, visit.entry.value);
        }
        hash_map_iterator_t iter = hashMapIterator_construct(manifest->attributes);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            celix_autofree char* attrKey = celix_utils_strdup(hashMapEntry_getKey(entry));
            if (attrKey == NULL) {
                return NULL;
            }
            celix_properties_t* value = hashMapEntry_getValue(entry);
            celix_properties_t* cloneValue = celix_properties_copy(value);
            if (cloneValue == NULL) {
                return NULL;
            }
            hashMap_put(clone->attributes, celix_steal_ptr(attrKey), cloneValue);
        }
    }

    return celix_steal_ptr(clone);
}

celix_status_t manifest_destroy(manifest_pt manifest) {
    if (manifest != NULL) {
        properties_destroy(manifest->mainAttributes);
        hash_map_iterator_t iter = hashMapIterator_construct(manifest->attributes);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            celix_properties_t* value = hashMapEntry_getValue(entry);
            celix_properties_destroy(value);
        }
        hashMap_destroy(manifest->attributes, true, false);
        manifest->mainAttributes = NULL;
        manifest->attributes = NULL;
        free(manifest);
        manifest = NULL;
    }
    return CELIX_SUCCESS;
}

celix_status_t manifest_createFromFile(const char *filename, manifest_pt *manifest) {
    celix_status_t status;

    celix_auto(manifest_pt) manifestNew = NULL;
    status = manifest_create(&manifestNew);

    status = CELIX_DO_IF(status, manifest_read(manifestNew, filename));
    if (status == CELIX_SUCCESS) {
        *manifest = celix_steal_ptr(manifestNew);
    } else {
        celix_err_pushf("Cannot create manifest from file: %s", celix_strerror(status));
    }
    return status;
}

//LCOV_EXCL_START
void manifest_clear(manifest_pt manifest) {

}
//LCOV_EXCL_STOP

properties_pt manifest_getMainAttributes(manifest_pt manifest) {
    return manifest->mainAttributes;
}

celix_status_t manifest_getEntries(manifest_pt manifest, hash_map_pt *map) {
    *map = manifest->attributes;
    return CELIX_SUCCESS;
}

celix_status_t manifest_read(manifest_pt manifest, const char *filename) {
    celix_status_t status = CELIX_SUCCESS;

    celix_autoptr(FILE) file = fopen(filename, "r");
    if (file != NULL) {
        status = manifest_readFromStream(manifest, file);
    } else {
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
    }

    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Cannot read manifest %s: %s", filename, celix_strerror(status));
    }

    return status;
}

celix_status_t manifest_readFromStream(manifest_pt manifest, FILE* stream) {
    char stackBuf[512];
    char *bytes = stackBuf;

    // get file size
    if (fseek(stream, 0L, SEEK_END) == -1) {
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }
    long int size = ftell(stream);
    if (size < 0) {
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
    } else if (size >= INT_MAX) {
        celix_err_pushf("Manifest error: file too large - %ld", size);
        return CELIX_BUNDLE_EXCEPTION;
    }
    rewind(stream);

    celix_autofree char* heapBuf = NULL;
    if (size+1 > sizeof(stackBuf)) {
        heapBuf = bytes =  malloc(size+1);
        if (heapBuf == NULL) {
            celix_err_pushf("Manifest error: failed to allocate %ld bytes", size);
            return CELIX_ENOMEM;
        }
    }

    if(fread(bytes, 1, size, stream) != (size_t)size) {
        return CELIX_FILE_IO_EXCEPTION;
    }
    // Force a new line at the end of the manifest to deal with broken manifest without any line-ending
    bytes[size++] = '\n';

    const char* key = NULL;
    int last = 0;
    int current = 0;
    celix_properties_t *headers = manifest->mainAttributes;
    for (int i = 0; i < size; i++)
    {
        // skip \r and \n if it is followed by another \n
        // (we catch the blank line case in the next iteration)
        if (bytes[i] == '\r')
        {
            if ((i + 1 < size) && (bytes[i + 1] == '\n'))
            {
                continue;
            }
        }
        if (bytes[i] == '\n')
        {
            if ((i + 1 < size) && (bytes[i + 1] == ' '))
            {
                i++;
                continue;
            }
        }
        // If we don't have a key yet and see the first : we parse it as the key
        // and skip the :<blank> that follows it.
        if ((key == NULL) && (bytes[i] == ':'))
        {
            key = &bytes[last];
            // precondition: current <= i
            bytes[current++] = '\0';
            if ((i + 1 < size) && (bytes[i + 1] == ' '))
            {
                last = current + 1;
                continue;
            }
            else
            {
                celix_err_pushf("Manifest error: Missing space separator - %s", key);
                return CELIX_INVALID_SYNTAX;
            }
        }
        // if we are at the end of a line
        if (bytes[i] == '\n')
        {
            // and it is a blank line stop parsing (main attributes are done)
            if ((last == current) && (key == NULL))
            {
                headers = NULL;
                continue;
            }
            // Otherwise, parse the value and add it to the map (we return
            // if we don't have a key or the key already exist).
            const char* value = &bytes[last];
            // precondition: current <= i
            bytes[current++] = '\0';

            if (key == NULL)
            {
                celix_err_pushf("Manifest error: Missing attribute name - %s", value);
                return CELIX_INVALID_SYNTAX;
            }
            else if (headers != NULL && celix_properties_get(headers, key, NULL) != NULL)
            {
                celix_err_pushf("Manifest error: Duplicate attribute name - %s", key);
                return CELIX_INVALID_SYNTAX;
            }
            if (headers == NULL) {
                // beginning of individual-section
                if (strcasecmp(key, "Name")) {
                    celix_err_push("Manifest error: individual-section missing Name attribute");
                    return CELIX_INVALID_SYNTAX;
                }
                headers = hashMap_get(manifest->attributes, value);
                if (headers == NULL) {
                    celix_autofree char* name = celix_utils_strdup(value);
                    if (name == NULL) {
                        return CELIX_ENOMEM;
                    }
                    headers = celix_properties_create();
                    if (headers == NULL) {
                        return CELIX_ENOMEM;
                    }
                    hashMap_put(manifest->attributes, (void*)celix_steal_ptr(name), headers);
                }
            } else if (headers == manifest->mainAttributes && celix_properties_size(headers) == 0) {
                // beginning of main-section
                if (strcasecmp(key, "Manifest-Version")) {
                    celix_err_push("Manifest error: main-section must start with Manifest-Version");
                    return CELIX_INVALID_SYNTAX;
                }
            }
            celix_properties_set(headers, key, value);
            last = current;
            key = NULL;
        }
        else
        {
            // precondition: current <= i
            // write back the byte if it needs to be included in the key or the value.
            bytes[current++] = bytes[i];
        }
    }
    if (celix_properties_size(manifest->mainAttributes) == 0) {
        celix_err_push("Manifest error: Empty");
        return CELIX_INVALID_SYNTAX;
    }
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START
void manifest_write(manifest_pt manifest, const char * filename) {

}
//LCOV_EXCL_STOP

const char* manifest_getValue(manifest_pt manifest, const char* name) {
    const char* val = properties_get(manifest->mainAttributes, name);
    bool isEmpty = utils_isStringEmptyOrNull(val);
    return isEmpty ? NULL : val;
}
