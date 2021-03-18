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

#include "celix_string_hashmap.h"
#include "celix_utils.h"

#include <stdlib.h>

static unsigned int DEFAULT_INITIAL_CAPACITY = 16;
static float DEFAULT_LOAD_FACTOR = 0.75f;
//static unsigned int MAXIMUM_CAPACITY = 1 << 30;

typedef union celix_hashmap_key {
    char* strKey;
    long longKey;
} celix_hashmap_key_t;

typedef union celix_hashmap_value {
    void *ptrVal;
    long int longVal;
} celix_hashmap_value_t;

typedef struct celix_hashmap_entry {
    celix_hashmap_key_t key;
    celix_hashmap_value_t val;
} celix_hashmap_entry_t;

typedef struct celix_hashmap {
    celix_hashmap_entry_t* table;
    size_t size;
    size_t treshold;
    size_t modificationCount;
    size_t tablelength;

    unsigned int (*hashKey)(const celix_hashmap_key_t* key);
    bool (*equalsKey)(const celix_hashmap_key_t* key1, const celix_hashmap_key_t* key2);
} celix_hashmap_t;

struct celix_string_hashmap {
    celix_hashmap_t genericMap;
};

static unsigned int celix_stringHashmap_hash(const celix_hashmap_key_t* key) {
    return celix_utils_stringHash(key->strKey);
}

static bool celix_stringHashmap_equals(const celix_hashmap_key_t* key1, const celix_hashmap_key_t* key2) {
    return celix_utils_stringEquals(key1->strKey, key2->strKey);
}

static void celix_hashmap_init(celix_hashmap_t* map, unsigned int (*hashKey)(const celix_hashmap_key_t*), bool (*equalsKey)(const celix_hashmap_key_t*, const celix_hashmap_key_t*)) {
    map->treshold = DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR;
    map->table = malloc(DEFAULT_INITIAL_CAPACITY * sizeof(celix_hashmap_entry_t));
    map->size = 0;
    map->modificationCount = 0;
    map->tablelength = DEFAULT_INITIAL_CAPACITY;
    map->hashKey = hashKey;
    map->equalsKey = equalsKey;
}
static void celix_hashmap_deinit(celix_hashmap_t* map) {
    free(map->table);
}

celix_string_hashmap_t* celix_stringHashmap_create() {
    celix_string_hashmap_t* map = calloc(1, sizeof(*map));
    celix_hashmap_init(&map->genericMap, celix_stringHashmap_hash, celix_stringHashmap_equals);
    return map;
}

void celix_stringHashmap_destroy(celix_string_hashmap_t* map) {
    if (map != NULL) {
        celix_hashmap_deinit(&map->genericMap);
    }
    free(map);
}

size_t celix_stringHashMap_size(celix_string_hashmap_t* map) {
    return map->genericMap.size;
}

