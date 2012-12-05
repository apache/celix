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
 * hash_map_private.h
 *
 *  \date       Jul 21, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef HASH_MAP_PRIVATE_H_
#define HASH_MAP_PRIVATE_H_

#include "exports.h"
#include "hash_map.h"

UTILS_EXPORT unsigned int hashMap_hashCode(void * toHash);
UTILS_EXPORT int hashMap_equals(void * toCompare, void * compare);

void hashMap_resize(hash_map_t map, int newCapacity);
void * hashMap_removeEntryForKey(hash_map_t map, void * key);
UTILS_EXPORT hash_map_entry_t hashMap_removeMapping(hash_map_t map, hash_map_entry_t entry);
void hashMap_addEntry(hash_map_t map, int hash, void * key, void * value, int bucketIndex);

struct hashMapEntry {
	void * key;
	void * value;
	hash_map_entry_t next;
	unsigned int hash;
};

struct hashMap {
	hash_map_entry_t * table;
	unsigned int size;
	unsigned int treshold;
	unsigned int modificationCount;
	unsigned int tablelength;

	unsigned int (*hashKey)(void * key);
	unsigned int (*hashValue)(void * value);
	int (*equalsKey)(void * key1, void * key2);
	int (*equalsValue)(void * value1, void * value2);
};

struct hashMapIterator {
	hash_map_t map;
	hash_map_entry_t next;
	hash_map_entry_t current;
	int expectedModCount;
	int index;
};

struct hashMapKeySet {
	hash_map_t map;
};

struct hashMapValues {
	hash_map_t map;
};

struct hashMapEntrySet {
	hash_map_t map;
};


#endif /* HASH_MAP_PRIVATE_H_ */
