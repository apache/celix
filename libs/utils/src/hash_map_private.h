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
 * hash_map_private.h
 *
 *  \date       Jul 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef HASH_MAP_PRIVATE_H_
#define HASH_MAP_PRIVATE_H_

#include "hash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int hashMap_hashCode(const void* toHash);
int hashMap_equals(const void* toCompare, const void* compare);

void hashMap_resize(hash_map_pt map, int newCapacity);
hash_map_entry_pt hashMap_removeEntryForKey(hash_map_pt map, const void* key);
hash_map_entry_pt hashMap_removeMapping(hash_map_pt map, hash_map_entry_pt entry);
void hashMap_addEntry(hash_map_pt map, int hash, void* key, void* value, int bucketIndex);

struct hashMapEntry {
    void* key;
    void* value;
    hash_map_entry_pt next;
    unsigned int hash;
};

struct hashMap {
    hash_map_entry_pt * table;
    unsigned int size;
    unsigned int treshold;
    unsigned int modificationCount;
    unsigned int tablelength;

    unsigned int (*hashKey)(const void* key);
    unsigned int (*hashValue)(const void* value);
    int (*equalsKey)(const void* key1, const void* key2);
    int (*equalsValue)(const void* value1, const void* value2);
};

struct hashMapKeySet {
    hash_map_pt map;
};

struct hashMapValues {
    hash_map_pt map;
};

struct hashMapEntrySet {
    hash_map_pt map;
};

#ifdef __cplusplus
}
#endif

#endif /* HASH_MAP_PRIVATE_H_ */
