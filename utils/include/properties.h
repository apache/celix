/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * properties.h
 *
 *  \date       Apr 27, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PROPERTIES_H_
#define PROPERTIES_H_

#include <stdio.h>

#include "hash_map.h"
#include "exports.h"
#include "celix_errno.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef hash_map_pt properties_pt;
typedef hash_map_t properties_t;

typedef hash_map_t celix_properties_t;

UTILS_EXPORT properties_pt properties_create(void);

UTILS_EXPORT void properties_destroy(properties_pt properties);

UTILS_EXPORT properties_pt properties_load(const char *filename);

UTILS_EXPORT properties_pt properties_loadWithStream(FILE *stream);

UTILS_EXPORT properties_pt properties_loadFromString(const char *input);

UTILS_EXPORT void properties_store(properties_pt properties, const char *file, const char *header);

UTILS_EXPORT const char *properties_get(properties_pt properties, const char *key);

UTILS_EXPORT const char *properties_getWithDefault(properties_pt properties, const char *key, const char *defaultValue);

UTILS_EXPORT void properties_set(properties_pt properties, const char *key, const char *value);

UTILS_EXPORT void properties_unset(properties_pt properties, const char *key);

UTILS_EXPORT celix_status_t properties_copy(properties_pt properties, properties_pt *copy);

#define PROPERTIES_FOR_EACH(props, key) \
    for(hash_map_iterator_t iter = hashMapIterator_construct(props); \
        hashMapIterator_hasNext(&iter), (key) = (const char*)hashMapIterator_nextKey(&iter);)


/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/
 
celix_properties_t* celix_properties_create(void);

void celix_properties_destroy(celix_properties_t *properties);

celix_properties_t* celix_properties_load(const char *filename);

celix_properties_t* celix_properties_loadWithStream(FILE *stream);

celix_properties_t* celix_properties_loadFromString(const char *input);

void celix_properties_store(celix_properties_t *properties, const char *file, const char *header);

const char* celix_properties_get(const celix_properties_t *properties, const char *key);

const char* celix_properties_getWithDefault(const celix_properties_t *properties, const char *key, const char *defaultValue);

void celix_properties_set(celix_properties_t *properties, const char *key, const char *value);

void celix_properties_unset(celix_properties_t *properties, const char *key);

celix_properties_t* celix_properties_copy(celix_properties_t *properties);

long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue);

void celix_properties_setLong(celix_properties_t *props, const char *key, long value);

#define CELIX_PROPERTIES_FOR_EACH(props, key) \
    for(hash_map_iterator_t iter = hashMapIterator_construct(props); \
        hashMapIterator_hasNext(&iter), (key) = (const char*)hashMapIterator_nextKey(&iter);)



#ifdef __cplusplus
}
#endif

#endif /* PROPERTIES_H_ */
