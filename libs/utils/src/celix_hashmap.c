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
#include <assert.h>
#include <memory.h>
#include <math.h>

static unsigned int DEFAULT_INITIAL_CAPACITY = 16;
static float DEFAULT_LOAD_FACTOR = 0.75f;
static unsigned int MAXIMUM_CAPACITY = 1 << 30;

typedef union celix_hashmap_key {
    char* strKey;
    long longKey;
} celix_hashmap_key_t;

typedef union celix_hashmap_value {
    void *ptrVal;
    long int longVal;
} celix_hashmap_value_t;

typedef struct celix_hashmap_entry celix_hashmap_entry_t;
struct celix_hashmap_entry {
    celix_hashmap_key_t key;
    celix_hashmap_value_t value;
    celix_hashmap_entry_t* next;
    unsigned int hash;
};

typedef struct celix_hashmap {
    celix_hashmap_entry_t** table;
    size_t size;
    size_t treshold;
    size_t modificationCount;
    size_t tablelength;
    bool stringKey;

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

static unsigned int celix_hashmap_indexFor(unsigned int h, unsigned int length) {
    return h & (length - 1);
}

static celix_hashmap_entry_t* celix_hashmap_getEntry(celix_hashmap_t* map, celix_hashmap_key_t* key) {
    assert(key != NULL);
    unsigned int hash = map->hashKey(key);
    unsigned int index = celix_hashmap_indexFor(hash, map->tablelength);
    for (celix_hashmap_entry_t* entry = map->table[index]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && map->equalsKey(key, &entry->key)) {
            return entry;
        }
    }
    return NULL;
}

static void hashmap_resize(celix_hashmap_t* map, int newCapacity) {
    celix_hashmap_entry_t** newTable;
    unsigned int j;
    if (map->tablelength == MAXIMUM_CAPACITY) {
        return;
    }

    newTable = calloc(newCapacity, sizeof(celix_hashmap_entry_t*));

    for (j = 0; j < map->tablelength; j++) {
        celix_hashmap_entry_t* entry = map->table[j];
        if (entry != NULL) {
            map->table[j] = NULL;
            do {
                celix_hashmap_entry_t* next = entry->next;
                unsigned int i = celix_hashmap_indexFor(entry->hash, newCapacity);
                entry->next = newTable[i];
                newTable[i] = entry;
                entry = next;
            } while (entry != NULL);
        }
    }
    free(map->table);
    map->table = newTable;
    map->tablelength = newCapacity;
    map->treshold = (unsigned int) ceil(newCapacity * DEFAULT_LOAD_FACTOR);
}

static void celix_hashmap_addEntry(celix_hashmap_t* map, unsigned int hash, celix_hashmap_key_t* key, celix_hashmap_value_t* value, int bucketIndex) {
    celix_hashmap_entry_t* entry = map->table[bucketIndex];
    celix_hashmap_entry_t* newEntry = malloc(sizeof(*newEntry));
    newEntry->hash = hash;
    if (map->stringKey) {
        memset(&newEntry->key, 0, sizeof(newEntry->key));
        newEntry->key.strKey = celix_utils_strdup(key->strKey);
    } else {
        memcpy(&newEntry->key, key, sizeof(*key));
    }
    memcpy(&newEntry->value, value, sizeof(*value));
    newEntry->next = entry;
    map->table[bucketIndex] = newEntry;
    if (map->size++ >= map->treshold) {
        hashmap_resize(map, 2 * map->tablelength);
    }
}

static celix_hashmap_value_t celix_hashmap_put(celix_hashmap_t* map, celix_hashmap_key_t* key, celix_hashmap_value_t* value) {
    assert(key != NULL);
    celix_hashmap_value_t prev;
    memset(&prev, 0, sizeof(prev));
    unsigned int hash = map->hashKey(key);
    unsigned int index = celix_hashmap_indexFor(hash, map->tablelength);
    for (celix_hashmap_entry_t* entry = map->table[index]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && map->equalsKey(key, &entry->key)) {
            memcpy(&prev, &entry->value, sizeof(prev));
            memcpy(&entry->value, value, sizeof(*value));
            return prev;
        }
    }
    map->modificationCount++;
    celix_hashmap_addEntry(map, hash, key, value, index);
    return prev;
}

static celix_hashmap_value_t celix_hashmap_remove(celix_hashmap_t* map, celix_hashmap_key_t* key) {
    celix_hashmap_value_t removedValue;
    memset(&removedValue, 0, sizeof(removedValue));

    unsigned int hash = map->hashKey(key);
    unsigned int index = celix_hashmap_indexFor(hash, map->tablelength);
    celix_hashmap_entry_t* entry = map->table[index];
    celix_hashmap_entry_t* prev = entry;

    while (entry != NULL) {
        celix_hashmap_entry_t* next = entry->next;
        if (entry->hash == hash && map->equalsKey(key, &entry->key)) {
            map->modificationCount++;
            map->size--;
            if (prev == entry) {
                map->table[index] = next;
            } else {
                prev->next = next;
            }
            break;
        }
        prev = entry;
        entry = next;
    }
    if (entry != NULL && map->stringKey) {
        free(entry->key.strKey);
        memcpy(&removedValue, &entry->value, sizeof(removedValue));
    }
    return removedValue;
}

static void celix_hashmap_init(celix_hashmap_t* map, bool stringKey, unsigned int (*hashKey)(const celix_hashmap_key_t*), bool (*equalsKey)(const celix_hashmap_key_t*, const celix_hashmap_key_t*)) {
    map->treshold = (size_t)((float)DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR);
    map->table = calloc(DEFAULT_INITIAL_CAPACITY, sizeof(celix_hashmap_entry_t*));
    map->size = 0;
    map->modificationCount = 0;
    map->tablelength = DEFAULT_INITIAL_CAPACITY;
    map->stringKey = true;
    map->hashKey = hashKey;
    map->equalsKey = equalsKey;
}

static void celix_hashmap_clear(celix_hashmap_t* map, bool freeValues) {
    map->modificationCount++;
    for (unsigned int i = 0; i < map->tablelength; i++) {
        celix_hashmap_entry_t* entry = map->table[i];
        while (entry != NULL) {
            celix_hashmap_entry_t* clearEntry = entry;
            entry = entry->next;
            if (map->stringKey) {
                free(clearEntry->key.strKey);
            }
            if (freeValues) {
                free(clearEntry->value.ptrVal);
            }
            free(clearEntry);
        }
        map->table[i] = NULL;
    }
    map->size = 0;
}

static void celix_hashmap_deinit(celix_hashmap_t* map, bool freeValues) {
    celix_hashmap_clear(map, freeValues);
    free(map->table);
}

celix_string_hashmap_t* celix_stringHashmap_create() {
    celix_string_hashmap_t* map = calloc(1, sizeof(*map));
    celix_hashmap_init(&map->genericMap, true, celix_stringHashmap_hash, celix_stringHashmap_equals);
    return map;
}

void celix_stringHashmap_destroy(celix_string_hashmap_t* map, bool freeValues) {
    if (map != NULL) {
        celix_hashmap_deinit(&map->genericMap, freeValues);
    }
    free(map);
}

size_t celix_stringHashMap_size(celix_string_hashmap_t* map) {
    return map->genericMap.size;
}

void* celix_stringHashmap_get(celix_string_hashmap_t* map, const char* key) {
    celix_hashmap_key_t hashKey;
    hashKey.strKey = (char*)key;
    celix_hashmap_entry_t* entry = celix_hashmap_getEntry(&map->genericMap, &hashKey);
    void* val = NULL;
    if (entry != NULL) {
        val = entry->value.ptrVal;
    }
    return val;
}

void* celix_stringHashMap_put(celix_string_hashmap_t* map, const char* key, void* value) {
    celix_hashmap_key_t mapKey;
    mapKey.strKey = (char*)key;
    celix_hashmap_value_t mapValue;
    mapValue.ptrVal = value;
    celix_hashmap_value_t prevValue = celix_hashmap_put(&map->genericMap, &mapKey, &mapValue);
    return prevValue.ptrVal;
}

bool celix_stringHashmap_hasKey(celix_string_hashmap_t* map, const char* key) {
    celix_hashmap_key_t hashKey;
    hashKey.strKey = (char*)key;
    celix_hashmap_entry_t* entry = celix_hashmap_getEntry(&map->genericMap, &hashKey);
    return entry != NULL;
}

void* celix_stringHashMap_remove(celix_string_hashmap_t* map, const char* key) {
    celix_hashmap_key_t hashKey;
    hashKey.strKey = (char*)key;
    celix_hashmap_value_t removedValue = celix_hashmap_remove(&map->genericMap, &hashKey);
    return removedValue.ptrVal;
}