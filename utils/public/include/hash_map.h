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
 * hash_map.h
 *
 *  \date       Jul 21, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef HASH_MAP_H_
#define HASH_MAP_H_

#include "celixbool.h"
#include "exports.h"

typedef struct hashMapEntry * hash_map_entry_t;
typedef struct hashMap * hash_map_t;

typedef struct hashMapIterator * hash_map_iterator_t;

typedef struct hashMapKeySet * hash_map_key_set_t;
typedef struct hashMapValues * hash_map_values_t;
typedef struct hashMapEntrySet * hash_map_entry_set_t;

UTILS_EXPORT hash_map_t hashMap_create(unsigned int (*keyHash)(void *), unsigned int (*valueHash)(void *),
		int (*keyEquals)(void *, void *), int (*valueEquals)(void *, void *));
UTILS_EXPORT void hashMap_destroy(hash_map_t map, bool freeKeys, bool freeValues);
UTILS_EXPORT int hashMap_size(hash_map_t map);
UTILS_EXPORT bool hashMap_isEmpty(hash_map_t map);
UTILS_EXPORT void * hashMap_get(hash_map_t map, void * key);
UTILS_EXPORT bool hashMap_containsKey(hash_map_t map, void * key);
UTILS_EXPORT hash_map_entry_t hashMap_getEntry(hash_map_t map, void * key);
UTILS_EXPORT void * hashMap_put(hash_map_t map, void * key, void * value);
UTILS_EXPORT void * hashMap_remove(hash_map_t map, void * key);
UTILS_EXPORT void hashMap_clear(hash_map_t map, bool freeKey, bool freeValue);
UTILS_EXPORT bool hashMap_containsValue(hash_map_t map, void * value);

UTILS_EXPORT hash_map_iterator_t hashMapIterator_create(hash_map_t map);
UTILS_EXPORT void hashMapIterator_destroy(hash_map_iterator_t iterator);
UTILS_EXPORT bool hashMapIterator_hasNext(hash_map_iterator_t iterator);
UTILS_EXPORT void hashMapIterator_remove(hash_map_iterator_t iterator);
UTILS_EXPORT void * hashMapIterator_nextValue(hash_map_iterator_t iterator);
UTILS_EXPORT void * hashMapIterator_nextKey(hash_map_iterator_t iterator);
UTILS_EXPORT hash_map_entry_t hashMapIterator_nextEntry(hash_map_iterator_t iterator);

UTILS_EXPORT hash_map_key_set_t hashMapKeySet_create(hash_map_t map);
UTILS_EXPORT int hashMapKeySet_size(hash_map_key_set_t keySet);
UTILS_EXPORT bool hashMapKeySet_contains(hash_map_key_set_t keySet, void * key);
UTILS_EXPORT bool hashMapKeySet_remove(hash_map_key_set_t keySet, void * key);
UTILS_EXPORT void hashMapKeySet_clear(hash_map_key_set_t keySet);
UTILS_EXPORT bool hashMapKeySet_isEmpty(hash_map_key_set_t keySet);

UTILS_EXPORT hash_map_values_t hashMapValues_create(hash_map_t map);
UTILS_EXPORT void hashMapValues_destroy(hash_map_values_t values);
UTILS_EXPORT hash_map_iterator_t hashMapValues_iterator(hash_map_values_t values);
UTILS_EXPORT int hashMapValues_size(hash_map_values_t values);
UTILS_EXPORT bool hashMapValues_contains(hash_map_values_t values, void * o);
UTILS_EXPORT void hashMapValues_toArray(hash_map_values_t values, void* *array[], unsigned int *size);
UTILS_EXPORT bool hashMapValues_remove(hash_map_values_t values, void * o);
UTILS_EXPORT void hashMapValues_clear(hash_map_values_t values);
UTILS_EXPORT bool hashMapValues_isEmpty(hash_map_values_t values);

UTILS_EXPORT hash_map_entry_set_t hashMapEntrySet_create(hash_map_t map);
UTILS_EXPORT int hashMapEntrySet_size(hash_map_entry_set_t entrySet);
UTILS_EXPORT bool hashMapEntrySet_contains(hash_map_entry_set_t entrySet, hash_map_entry_t entry);
UTILS_EXPORT bool hashMapEntrySet_remove(hash_map_values_t entrySet, hash_map_entry_t entry);
UTILS_EXPORT void hashMapEntrySet_clear(hash_map_entry_set_t entrySet);
UTILS_EXPORT bool hashMapEntrySet_isEmpty(hash_map_entry_set_t entrySet);

UTILS_EXPORT void * hashMapEntry_getKey(hash_map_entry_t entry);
UTILS_EXPORT void * hashMapEntry_getValue(hash_map_entry_t entry);

#endif /* HASH_MAP_H_ */
