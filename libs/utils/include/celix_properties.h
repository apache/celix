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

#include "hash_map.h"
#include "exports.h"
#include "celix_errno.h"

#ifndef CELIX_PROPERTIES_H_
#define CELIX_PROPERTIES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef hash_map_t celix_properties_t;
typedef hash_map_iterator_t celix_properties_iterator_t;


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

const char* celix_properties_get(const celix_properties_t *properties, const char *key, const char *defaultValue);

void celix_properties_set(celix_properties_t *properties, const char *key, const char *value);

void celix_properties_setWithoutCopy(celix_properties_t *properties, char *key, char *value);

void celix_properties_unset(celix_properties_t *properties, const char *key);

celix_properties_t* celix_properties_copy(const celix_properties_t *properties);

long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue);
void celix_properties_setLong(celix_properties_t *props, const char *key, long value);

bool celix_properties_getAsBool(const celix_properties_t *props, const char *key, bool defaultValue);
void celix_properties_setBool(celix_properties_t *props, const char *key, bool val);


void celix_properties_setDouble(celix_properties_t *props, const char *key, double val);
double celix_properties_getAsDouble(const celix_properties_t *props, const char *key, double defaultValue);

int celix_properties_size(const celix_properties_t *properties);

celix_properties_iterator_t celix_propertiesIterator_construct(const celix_properties_t *properties);
bool celix_propertiesIterator_hasNext(celix_properties_iterator_t *iter);
const char* celix_propertiesIterator_nextKey(celix_properties_iterator_t *iter);

#define CELIX_PROPERTIES_FOR_EACH(props, key) \
    for(hash_map_iterator_t iter = celix_propertiesIterator_construct(props); \
        celix_propertiesIterator_hasNext(&iter), (key) = celix_propertiesIterator_nextKey(&iter);)



#ifdef __cplusplus
}
#endif

#endif /* CELIX_PROPERTIES_H_ */
