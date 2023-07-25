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

#ifndef CELIX_PROPERTIES_H_
#define CELIX_PROPERTIES_H_

#include <stdio.h>
#include <stdbool.h>

#include "celix_cleanup.h"
#include "celix_compiler.h"
#include "celix_errno.h"
#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hashMap celix_properties_t; //opaque struct, TODO try to make this a celix_properties struct

typedef struct celix_properties_iterator {
    //private data
    void* _data1;
    void* _data2;
    void* _data3;
    int _data4;
    int _data5;
} celix_properties_iterator_t;


/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

CELIX_UTILS_EXPORT celix_properties_t* celix_properties_create(void);

CELIX_UTILS_EXPORT void celix_properties_destroy(celix_properties_t *properties);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_properties_t, celix_properties_destroy)

CELIX_UTILS_EXPORT celix_properties_t* celix_properties_load(const char *filename);

CELIX_UTILS_EXPORT celix_properties_t* celix_properties_loadWithStream(FILE *stream);

CELIX_UTILS_EXPORT celix_properties_t* celix_properties_loadFromString(const char *input);

CELIX_UTILS_EXPORT void celix_properties_store(celix_properties_t *properties, const char *file, const char *header);

CELIX_UTILS_EXPORT const char* celix_properties_get(const celix_properties_t *properties, const char *key, const char *defaultValue);

CELIX_UTILS_EXPORT void celix_properties_set(celix_properties_t *properties, const char *key, const char *value);

CELIX_UTILS_EXPORT void celix_properties_setWithoutCopy(celix_properties_t *properties, char *key, char *value);

CELIX_UTILS_EXPORT void celix_properties_unset(celix_properties_t *properties, const char *key);

CELIX_UTILS_EXPORT celix_properties_t* celix_properties_copy(const celix_properties_t *properties);

CELIX_UTILS_EXPORT long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue);
CELIX_UTILS_EXPORT void celix_properties_setLong(celix_properties_t *props, const char *key, long value);

CELIX_UTILS_EXPORT bool celix_properties_getAsBool(const celix_properties_t *props, const char *key, bool defaultValue);
CELIX_UTILS_EXPORT void celix_properties_setBool(celix_properties_t *props, const char *key, bool val);


CELIX_UTILS_EXPORT void celix_properties_setDouble(celix_properties_t *props, const char *key, double val);
CELIX_UTILS_EXPORT double celix_properties_getAsDouble(const celix_properties_t *props, const char *key, double defaultValue);

CELIX_UTILS_EXPORT int celix_properties_size(const celix_properties_t *properties);

CELIX_UTILS_EXPORT celix_properties_iterator_t celix_propertiesIterator_construct(const celix_properties_t *properties);
CELIX_UTILS_EXPORT bool celix_propertiesIterator_hasNext(celix_properties_iterator_t *iter);
CELIX_UTILS_EXPORT const char* celix_propertiesIterator_nextKey(celix_properties_iterator_t *iter);
CELIX_UTILS_EXPORT celix_properties_t* celix_propertiesIterator_properties(celix_properties_iterator_t *iter);

#define CELIX_PROPERTIES_FOR_EACH(props, key) _CELIX_PROPERTIES_FOR_EACH(CELIX_UNIQUE_ID(iter), props, key)

#define _CELIX_PROPERTIES_FOR_EACH(iter, props, key) \
    for(celix_properties_iterator_t iter = celix_propertiesIterator_construct(props); \
        celix_propertiesIterator_hasNext(&iter), (key) = celix_propertiesIterator_nextKey(&iter);)


#ifdef __cplusplus
}
#endif

#endif /* CELIX_PROPERTIES_H_ */
