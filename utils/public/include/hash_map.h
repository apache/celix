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
 *  Created on: Jul 21, 2010
 *      Author: alexanderb
 */

#ifndef HASH_MAP_H_
#define HASH_MAP_H_

#include "celixbool.h"
#include "exports.h"

typedef struct hashMapEntry * HASH_MAP_ENTRY;
typedef struct hashMap * HASH_MAP;

typedef struct hashMapIterator * HASH_MAP_ITERATOR;

typedef struct hashMapKeySet * HASH_MAP_KEY_SET;
typedef struct hashMapValues * HASH_MAP_VALUES;
typedef struct hashMapEntrySet * HASH_MAP_ENTRY_SET;

UTILS_EXPORT HASH_MAP hashMap_create(unsigned int (*keyHash)(void *), unsigned int (*valueHash)(void *),
		int (*keyEquals)(void *, void *), int (*valueEquals)(void *, void *));
UTILS_EXPORT void hashMap_destroy(HASH_MAP map, bool freeKeys, bool freeValues);
UTILS_EXPORT int hashMap_size(HASH_MAP map);
UTILS_EXPORT bool hashMap_isEmpty(HASH_MAP map);
UTILS_EXPORT void * hashMap_get(HASH_MAP map, void * key);
UTILS_EXPORT bool hashMap_containsKey(HASH_MAP map, void * key);
UTILS_EXPORT HASH_MAP_ENTRY hashMap_getEntry(HASH_MAP map, void * key);
UTILS_EXPORT void * hashMap_put(HASH_MAP map, void * key, void * value);
UTILS_EXPORT void * hashMap_remove(HASH_MAP map, void * key);
UTILS_EXPORT void hashMap_clear(HASH_MAP map, bool freeKey, bool freeValue);
UTILS_EXPORT bool hashMap_containsValue(HASH_MAP map, void * value);

UTILS_EXPORT HASH_MAP_ITERATOR hashMapIterator_create(HASH_MAP map);
UTILS_EXPORT void hashMapIterator_destroy(HASH_MAP_ITERATOR iterator);
UTILS_EXPORT bool hashMapIterator_hasNext(HASH_MAP_ITERATOR iterator);
UTILS_EXPORT void hashMapIterator_remove(HASH_MAP_ITERATOR iterator);
UTILS_EXPORT void * hashMapIterator_nextValue(HASH_MAP_ITERATOR iterator);
UTILS_EXPORT void * hashMapIterator_nextKey(HASH_MAP_ITERATOR iterator);
UTILS_EXPORT HASH_MAP_ENTRY hashMapIterator_nextEntry(HASH_MAP_ITERATOR iterator);

UTILS_EXPORT HASH_MAP_KEY_SET hashMapKeySet_create(HASH_MAP map);
UTILS_EXPORT int hashMapKeySet_size(HASH_MAP_KEY_SET keySet);
UTILS_EXPORT bool hashMapKeySet_contains(HASH_MAP_KEY_SET keySet, void * key);
UTILS_EXPORT bool hashMapKeySet_remove(HASH_MAP_KEY_SET keySet, void * key);
UTILS_EXPORT void hashMapKeySet_clear(HASH_MAP_KEY_SET keySet);
UTILS_EXPORT bool hashMapKeySet_isEmpty(HASH_MAP_KEY_SET keySet);

UTILS_EXPORT HASH_MAP_VALUES hashMapValues_create(HASH_MAP map);
UTILS_EXPORT void hashMapValues_destroy(HASH_MAP_VALUES values);
UTILS_EXPORT HASH_MAP_ITERATOR hashMapValues_iterator(HASH_MAP_VALUES values);
UTILS_EXPORT int hashMapValues_size(HASH_MAP_VALUES values);
UTILS_EXPORT bool hashMapValues_contains(HASH_MAP_VALUES values, void * o);
UTILS_EXPORT void hashMapValues_toArray(HASH_MAP_VALUES values, void* *array[], unsigned int *size);
UTILS_EXPORT bool hashMapValues_remove(HASH_MAP_VALUES values, void * o);
UTILS_EXPORT void hashMapValues_clear(HASH_MAP_VALUES values);
UTILS_EXPORT bool hashMapValues_isEmpty(HASH_MAP_VALUES values);

UTILS_EXPORT HASH_MAP_ENTRY_SET hashMapEntrySet_create(HASH_MAP map);
UTILS_EXPORT int hashMapEntrySet_size(HASH_MAP_ENTRY_SET entrySet);
UTILS_EXPORT bool hashMapEntrySet_contains(HASH_MAP_ENTRY_SET entrySet, HASH_MAP_ENTRY entry);
UTILS_EXPORT bool hashMapEntrySet_remove(HASH_MAP_VALUES entrySet, HASH_MAP_ENTRY entry);
UTILS_EXPORT void hashMapEntrySet_clear(HASH_MAP_ENTRY_SET entrySet);
UTILS_EXPORT bool hashMapEntrySet_isEmpty(HASH_MAP_ENTRY_SET entrySet);

UTILS_EXPORT void * hashMapEntry_getKey(HASH_MAP_ENTRY entry);
UTILS_EXPORT void * hashMapEntry_getValue(HASH_MAP_ENTRY entry);

#endif /* HASH_MAP_H_ */
