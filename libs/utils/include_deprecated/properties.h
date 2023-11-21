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
 * properties.h
 *
 *  \date       Apr 27, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PROPERTIES_H_
#define PROPERTIES_H_

#include <stdio.h>

#include "celix_properties.h"
#include "hash_map.h"
#include "celix_utils_export.h"
#include "celix_errno.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_properties* properties_pt __attribute__((deprecated("properties is deprecated use celix_properties instead")));
typedef struct celix_properties properties_t __attribute__((deprecated("properties is deprecated use celix_properties instead")));

CELIX_UTILS_DEPRECATED_EXPORT celix_properties_t* properties_create(void);

CELIX_UTILS_DEPRECATED_EXPORT void properties_destroy(celix_properties_t *properties);

CELIX_UTILS_DEPRECATED_EXPORT celix_properties_t* properties_load(const char *filename);

CELIX_UTILS_DEPRECATED_EXPORT celix_properties_t* properties_loadWithStream(FILE *stream);

CELIX_UTILS_DEPRECATED_EXPORT celix_properties_t* properties_loadFromString(const char *input);

CELIX_UTILS_DEPRECATED_EXPORT void properties_store(celix_properties_t *properties, const char *file, const char *header);

CELIX_UTILS_DEPRECATED_EXPORT const char* properties_get(celix_properties_t *properties, const char *key);

CELIX_UTILS_DEPRECATED_EXPORT const char* properties_getWithDefault(celix_properties_t *properties, const char *key, const char *defaultValue);

CELIX_UTILS_DEPRECATED_EXPORT void properties_set(celix_properties_t *properties, const char *key, const char *value);

CELIX_UTILS_DEPRECATED_EXPORT void properties_unset(celix_properties_t *properties, const char *key);

CELIX_UTILS_DEPRECATED_EXPORT celix_status_t properties_copy(celix_properties_t *properties, celix_properties_t **copy);

/** TODO refactor
#define PROPERTIES_FOR_EACH(props, key) \
    for(hash_map_iterator_t iter = hashMapIterator_construct(props); \
        hashMapIterator_hasNext(&iter), (key) = (const char*)hashMapIterator_nextKey(&iter);)
*/

#ifdef __cplusplus
}
#endif

#endif /* PROPERTIES_H_ */
