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

#include "celix_string_hash_map.h"
#include "celix_long_hash_map.h"
#include "celix_utils.h"

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

/**
 * Whether to use realloc - instead of calloc - to resize a hash map.
 *
 * With realloc the memory is increased and the
 * map entries are corrected.
 *
 * With alloc a new buckets array is allocated and
 * entries are moved into the new bucket array.
 * And the old buckets array is freed.
 *
 */
#define CELIX_HASH_MAP_RESIZE_WITH_REALLOC

static unsigned int DEFAULT_INITIAL_CAPACITY = 16;
static double DEFAULT_LOAD_FACTOR = 0.75;
static unsigned int MAXIMUM_CAPACITY = INT32_MAX/10;

typedef enum celix_hash_map_key_type {
    CELIX_HASH_MAP_STRING_KEY,
    CELIX_HASH_MAP_LONG_KEY
} celix_hash_map_key_type_e;

typedef union celix_hash_map_key {
    const char* strKey;
    long longKey;
} celix_hash_map_key_t;

typedef struct celix_hash_map_entry celix_hash_map_entry_t;
struct celix_hash_map_entry {
    celix_hash_map_key_t key;
    celix_hash_map_value_t value;
    celix_hash_map_entry_t* next;
    unsigned int hash;
};

typedef struct celix_hash_map {
    celix_hash_map_entry_t** buckets;
    unsigned int bucketsSize; //nr of buckets
    unsigned int size; //nr of total entries
    double loadFactor;
    celix_hash_map_key_type_e keyType;
    celix_hash_map_value_t emptyValue;
    unsigned int (*hashKeyFunction)(const celix_hash_map_key_t* key);
    bool (*equalsKeyFunction)(const celix_hash_map_key_t* key1, const celix_hash_map_key_t* key2);
    void (*simpleRemovedCallback)(void* value);
    void* removedCallbackData;
    void (*removedStringKeyCallback)(void* data, const char* removedKey, celix_hash_map_value_t removedValue);
    void (*removedLongKeyCallback)(void* data, long removedKey, celix_hash_map_value_t removedValue);
    bool storeKeysWeakly;
} celix_hash_map_t;

struct celix_string_hash_map {
    celix_hash_map_t genericMap;
};

struct celix_long_hash_map {
    celix_hash_map_t genericMap;
};

static unsigned int celix_stringHashMap_hash(const celix_hash_map_key_t* key) {
    return celix_utils_stringHash(key->strKey);
}

static unsigned int celix_longHashMap_hash(const celix_hash_map_key_t* key) {
    return key->longKey ^ (key->longKey >> (sizeof(key->longKey)*8/2));
}

static bool celix_stringHashMap_equals(const celix_hash_map_key_t* key1, const celix_hash_map_key_t* key2) {
    return celix_utils_stringEquals(key1->strKey, key2->strKey);
}

static bool celix_longHashMap_equals(const celix_hash_map_key_t* key1, const celix_hash_map_key_t* key2) {
    return key1->longKey == key2->longKey;
}

static unsigned int celix_hashMap_threshold(celix_hash_map_t* map) {
    return (unsigned int)floor((double)map->bucketsSize * map->loadFactor);
}

static unsigned int celix_hashMap_indexFor(unsigned int h, unsigned int length) {
    return h & (length - 1);
}

static celix_hash_map_entry_t* celix_hashMap_getEntry(const celix_hash_map_t* map, const char* strKey, long longKey) {
    celix_hash_map_key_t key;
    if (map->keyType == CELIX_HASH_MAP_STRING_KEY) {
        key.strKey = strKey;
    } else {
        key.longKey = longKey;
    }
    unsigned int hash = map->hashKeyFunction(&key);
    unsigned int index = celix_hashMap_indexFor(hash, map->bucketsSize);
    for (celix_hash_map_entry_t* entry = map->buckets[index]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && map->equalsKeyFunction(&key, &entry->key)) {
            return entry;
        }
    }
    return NULL;
}

static void* celix_hashMap_get(const celix_hash_map_t* map, const char* strKey, long longKey) {
    celix_hash_map_entry_t* entry = celix_hashMap_getEntry(map, strKey, longKey);
    if (entry != NULL) {
        return entry->value.ptrValue;
    }
    return NULL;
}

static long celix_hashMap_getLong(const celix_hash_map_t* map, const char* strKey, long longKey, long fallbackValue) {
    celix_hash_map_entry_t* entry = celix_hashMap_getEntry(map, strKey, longKey);
    if (entry != NULL) {
        return entry->value.longValue;
    }
    return fallbackValue;
}

static double celix_hashMap_getDouble(const celix_hash_map_t* map, const char* strKey, long longKey, double fallbackValue) {
    celix_hash_map_entry_t* entry = celix_hashMap_getEntry(map, strKey, longKey);
    if (entry != NULL) {
        return entry->value.doubleValue;
    }
    return fallbackValue;
}

static bool celix_hashMap_getBool(const celix_hash_map_t* map, const char* strKey, long longKey, bool fallbackValue) {
    celix_hash_map_entry_t* entry = celix_hashMap_getEntry(map, strKey, longKey);
    if (entry != NULL) {
        return entry->value.boolValue;
    }
    return fallbackValue;
}

bool celix_hashMap_hasKey(const celix_hash_map_t* map, const char* strKey, long longKey) {
    return celix_hashMap_getEntry(map, strKey, longKey) != NULL;
}

#ifdef CELIX_HASH_MAP_RESIZE_WITH_REALLOC
static void celix_hashMap_resize(celix_hash_map_t* map, size_t newCapacity) {
    if (map->bucketsSize == MAXIMUM_CAPACITY) {
        return;
    }

    assert(newCapacity > map->bucketsSize); //hash map can only grow
    map->buckets = realloc(map->buckets, newCapacity * sizeof(celix_hash_map_entry_t*));

    //clear newly added mem
    for (unsigned int i = map->bucketsSize; i < newCapacity; ++i) {
        map->buckets[i]= NULL;
    }

    //reinsert old entries
    for (unsigned i = 0; i < map->bucketsSize; i++) {
        celix_hash_map_entry_t* entry = map->buckets[i];
        if (entry != NULL) {
            map->buckets[i] = NULL;
            do {
                celix_hash_map_entry_t* next = entry->next;
                unsigned int bucketIndex = celix_hashMap_indexFor(entry->hash, newCapacity);
                entry->next = map->buckets[bucketIndex];
                map->buckets[bucketIndex] = entry;
                entry = next;
            } while (entry != NULL);
        }
    }

    //update bucketSize to new capacity
    map->bucketsSize = newCapacity;
}
#else
static void celix_hashMap_resize(celix_hash_map_t* map, size_t newCapacity) {
    celix_hash_map_entry_t** newTable;
    unsigned int j;
    if (map->bucketsSize == MAXIMUM_CAPACITY) {
        return;
    }

    newTable = calloc(newCapacity, sizeof(celix_hash_map_entry_t*));

    for (j = 0; j < map->bucketsSize; j++) {
        celix_hash_map_entry_t* entry = map->buckets[j];
        if (entry != NULL) {
            map->buckets[j] = NULL;
            do {
                celix_hash_map_entry_t* next = entry->next;
                unsigned int i = celix_hashMap_indexFor(entry->hash, newCapacity);
                entry->next = newTable[i];
                newTable[i] = entry;
                entry = next;
            } while (entry != NULL);
        }
    }
    free(map->buckets);
    map->buckets = newTable;
    map->bucketsSize = newCapacity;
}
#endif

static void celix_hashMap_addEntry(celix_hash_map_t* map, unsigned int hash, const celix_hash_map_key_t* key, const celix_hash_map_value_t* value, unsigned int bucketIndex) {
    celix_hash_map_entry_t* entry = map->buckets[bucketIndex];
    celix_hash_map_entry_t* newEntry = malloc(sizeof(*newEntry));
    newEntry->hash = hash;
    if (map->keyType == CELIX_HASH_MAP_STRING_KEY) {
        newEntry->key.strKey = map->storeKeysWeakly ? key->strKey : celix_utils_strdup(key->strKey);
    } else {
        newEntry->key.longKey = key->longKey;
    }
    memcpy(&newEntry->value, value, sizeof(*value));
    newEntry->next = entry;
    map->buckets[bucketIndex] = newEntry;
    if (map->size++ >= celix_hashMap_threshold(map)) {
        celix_hashMap_resize(map, 2 * map->bucketsSize);
    }
}

static bool celix_hashMap_putValue(celix_hash_map_t* map, const char* strKey, long longKey, const celix_hash_map_value_t* value, celix_hash_map_value_t* replacedValueOut) {
    celix_hash_map_key_t key;
    if (map->keyType == CELIX_HASH_MAP_STRING_KEY) {
        key.strKey = strKey;
    } else {
        key.longKey = longKey;
    }
    unsigned int hash = map->hashKeyFunction(&key);
    unsigned int index = celix_hashMap_indexFor(hash, map->bucketsSize);
    for (celix_hash_map_entry_t* entry = map->buckets[index]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && map->equalsKeyFunction(&key, &entry->key)) {
            //entry found, replacing entry
            if (replacedValueOut != NULL) {
                *replacedValueOut = entry->value;
            }
            memcpy(&entry->value, value, sizeof(*value));
            return true;
        }
    }
    celix_hashMap_addEntry(map, hash, &key, value, index);
    if (replacedValueOut != NULL) {
        memset(replacedValueOut, 0, sizeof(*replacedValueOut));
    }
    return false;
}

static void* celix_hashMap_put(celix_hash_map_t* map, const char* strKey, long longKey, void* v) {
    celix_hash_map_value_t value;
    value.ptrValue = v;
    celix_hash_map_value_t replaced;
    celix_hashMap_putValue(map, strKey, longKey, &value, &replaced);
    return replaced.ptrValue;
}

static bool celix_hashMap_putLong(celix_hash_map_t* map, const char* strKey, long longKey, long v) {
    celix_hash_map_value_t value;
    value.longValue = v;
    return celix_hashMap_putValue(map, strKey, longKey, &value, NULL);
}

static bool celix_hashMap_putDouble(celix_hash_map_t* map, const char* strKey, long longKey, double v) {
    celix_hash_map_value_t value;
    value.doubleValue = v;
    return celix_hashMap_putValue(map, strKey, longKey, &value, NULL);
}

static bool celix_hashMap_putBool(celix_hash_map_t* map, const char* strKey, long longKey, bool v) {
    celix_hash_map_value_t value;
    value.boolValue = v;
    return celix_hashMap_putValue(map, strKey, longKey, &value, NULL);
}

static void celix_hashMap_destroyRemovedEntry(celix_hash_map_t* map, celix_hash_map_entry_t* removedEntry) {
    if (map->simpleRemovedCallback) {
        map->simpleRemovedCallback(removedEntry->value.ptrValue);
    } else if (map->removedLongKeyCallback) {
        map->removedLongKeyCallback(map->removedCallbackData, removedEntry->key.longKey, removedEntry->value);
    } else if (map->removedStringKeyCallback) {
        map->removedStringKeyCallback(map->removedCallbackData, removedEntry->key.strKey, removedEntry->value);
    }
    if (map->keyType == CELIX_HASH_MAP_STRING_KEY && !map->storeKeysWeakly) {
        free((char*)removedEntry->key.strKey);
    }
    free(removedEntry);
}

static bool celix_hashMap_remove(celix_hash_map_t* map, const char* strKey, long longKey) {
    celix_hash_map_key_t key;
    if (map->keyType == CELIX_HASH_MAP_STRING_KEY) {
        key.strKey = strKey;
    } else {
        key.longKey = longKey;
    }

    unsigned int hash = map->hashKeyFunction(&key);
    unsigned int index = celix_hashMap_indexFor(hash, map->bucketsSize);
    celix_hash_map_entry_t* visit = map->buckets[index];
    celix_hash_map_entry_t* removedEntry = NULL;
    celix_hash_map_entry_t* prev = NULL;

    while (visit != NULL) {
        if (visit->hash == hash && map->equalsKeyFunction(&key, &visit->key)) {
            //hash & equals -> found entry
            map->size--;
            if (map->buckets[index] == visit) {
                //current entry is first entry in bucket, set next to first entry in bucket
                map->buckets[index] = visit->next;
            } else {
                //entry is in between, update link of prev to next entry
                prev->next = visit->next;
            }
            removedEntry = visit;
            break;
        }
        prev = visit;
        visit = visit->next;
    }
    if (removedEntry != NULL) {
        celix_hashMap_destroyRemovedEntry(map, removedEntry);
        return true;
    }
    return false;
}

static void celix_hashMap_init(
        celix_hash_map_t* map,
        celix_hash_map_key_type_e keyType,
        unsigned int initialCapacity,
        double loadFactor,
        unsigned int (*hashKeyFn)(const celix_hash_map_key_t*),
        bool (*equalsKeyFn)(const celix_hash_map_key_t*, const celix_hash_map_key_t*)) {
    map->loadFactor = loadFactor;
    map->buckets = calloc(initialCapacity, sizeof(celix_hash_map_entry_t*));
    map->size = 0;
    map->bucketsSize = initialCapacity;
    map->keyType = keyType;
    memset(&map->emptyValue, 0, sizeof(map->emptyValue));
    map->hashKeyFunction = hashKeyFn;
    map->equalsKeyFunction = equalsKeyFn;
    map->simpleRemovedCallback = NULL;
    map->removedCallbackData = NULL;
    map->removedLongKeyCallback = NULL;
    map->removedStringKeyCallback = NULL;
    map->storeKeysWeakly = false;
}

static void celix_hashMap_clear(celix_hash_map_t* map) {
    for (unsigned int i = 0; i < map->bucketsSize; i++) {
        celix_hash_map_entry_t* entry = map->buckets[i];
        while (entry != NULL) {
            celix_hash_map_entry_t* removedEntry = entry;
            entry = entry->next;
            celix_hashMap_destroyRemovedEntry(map, removedEntry);
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}

static celix_hash_map_entry_t* celix_hashMap_firstEntry(const celix_hash_map_t* map) {
    celix_hash_map_entry_t* entry = NULL;
    for (unsigned int index = 0; index < map->bucketsSize; ++index) {
        entry = map->buckets[index];
        if (entry != NULL) {
            break;
        }
    }
    return entry;
}

static celix_hash_map_entry_t* celix_hashMap_nextEntry(const celix_hash_map_t* map, celix_hash_map_entry_t* entry) {
    if (entry == NULL) {
        //end entry, just return NULL
        return NULL;
    }

    celix_hash_map_entry_t* next = NULL;
    if (entry != NULL) {
        if (entry->next != NULL) {
            next = entry->next;
        } else {
            unsigned int index = celix_hashMap_indexFor(entry->hash, map->bucketsSize) + 1; //next index
            while (index < map->bucketsSize) {
                next = map->buckets[index++];
                if (next != NULL) {
                    break;
                }
            }
        }
    }
    return next;
}


celix_string_hash_map_t* celix_stringHashMap_createWithOptions(const celix_string_hash_map_create_options_t* opts) {
    celix_string_hash_map_t* map = calloc(1, sizeof(*map));
    unsigned int cap = opts->initialCapacity > 0 ? opts->initialCapacity : DEFAULT_INITIAL_CAPACITY;
    double fac = opts->loadFactor > 0 ? opts->loadFactor : DEFAULT_LOAD_FACTOR;
    celix_hashMap_init(&map->genericMap, CELIX_HASH_MAP_STRING_KEY, cap, fac, celix_stringHashMap_hash, celix_stringHashMap_equals);
    map->genericMap.simpleRemovedCallback = opts->simpleRemovedCallback;
    map->genericMap.removedCallbackData = opts->removedCallbackData;
    map->genericMap.removedStringKeyCallback = opts->removedCallback;
    map->genericMap.storeKeysWeakly = opts->storeKeysWeakly;
    return map;
}

celix_string_hash_map_t* celix_stringHashMap_create() {
    celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    return celix_stringHashMap_createWithOptions(&opts);
}

celix_long_hash_map_t* celix_longHashMap_createWithOptions(const celix_long_hash_map_create_options_t* opts) {
    celix_long_hash_map_t* map = calloc(1, sizeof(*map));
    unsigned int cap = opts->initialCapacity > 0 ? opts->initialCapacity : DEFAULT_INITIAL_CAPACITY;
    double fac = opts->loadFactor > 0 ? opts->loadFactor : DEFAULT_LOAD_FACTOR;
    celix_hashMap_init(&map->genericMap, CELIX_HASH_MAP_LONG_KEY, cap, fac, celix_longHashMap_hash, celix_longHashMap_equals);
    map->genericMap.simpleRemovedCallback = opts->simpleRemovedCallback;
    map->genericMap.removedCallbackData = opts->removedCallbackData;
    map->genericMap.removedLongKeyCallback = opts->removedCallback;
    map->genericMap.storeKeysWeakly = false;
    return map;
}

celix_long_hash_map_t* celix_longHashMap_create() {
    celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
    return celix_longHashMap_createWithOptions(&opts);
}

void celix_stringHashMap_destroy(celix_string_hash_map_t* map) {
    if (map != NULL) {
        celix_hashMap_clear(&map->genericMap);
        free(map->genericMap.buckets);
        free(map);
    }
}

void celix_longHashMap_destroy(celix_long_hash_map_t* map) {
    if (map != NULL) {
        celix_hashMap_clear(&map->genericMap);
        free(map->genericMap.buckets);
        free(map);
    }
}

size_t celix_stringHashMap_size(const celix_string_hash_map_t* map) {
    return map->genericMap.size;
}

size_t celix_longHashMap_size(const celix_long_hash_map_t* map) {
    return map->genericMap.size;
}

void* celix_stringHashMap_get(const celix_string_hash_map_t* map, const char* key) {
    return celix_hashMap_get(&map->genericMap, key, 0);
}

void* celix_longHashMap_get(const celix_long_hash_map_t* map, long key) {
    return celix_hashMap_get(&map->genericMap, NULL, key);
}

long celix_stringHashMap_getLong(const celix_string_hash_map_t* map, const char* key, long fallbackValue) {
    return celix_hashMap_getLong(&map->genericMap, key, 0, fallbackValue);
}

long celix_longHashMap_getLong(const celix_long_hash_map_t* map, long key, long fallbackValue) {
    return celix_hashMap_getLong(&map->genericMap, NULL, key, fallbackValue);

}

double celix_stringHashMap_getDouble(const celix_string_hash_map_t* map, const char* key, double fallbackValue) {
    return celix_hashMap_getDouble(&map->genericMap, key, 0, fallbackValue);
}

double celix_longHashMap_getDouble(const celix_long_hash_map_t* map, long key, double fallbackValue) {
    return celix_hashMap_getDouble(&map->genericMap, NULL, key, fallbackValue);
}

bool celix_stringHashMap_getBool(const celix_string_hash_map_t* map, const char* key, bool fallbackValue) {
    return celix_hashMap_getBool(&map->genericMap, key, 0, fallbackValue);
}

bool celix_longHashMap_getBool(const celix_long_hash_map_t* map, long key, bool fallbackValue) {
    return celix_hashMap_getBool(&map->genericMap, NULL, key, fallbackValue);
}

void* celix_stringHashMap_put(celix_string_hash_map_t* map, const char* key, void* value) {
    return celix_hashMap_put(&map->genericMap, key, 0, value);
}

void* celix_longHashMap_put(celix_long_hash_map_t* map, long key, void* value) {
    return celix_hashMap_put(&map->genericMap, NULL, key, value);
}

bool celix_stringHashMap_putLong(celix_string_hash_map_t* map, const char* key, long value) {
    return celix_hashMap_putLong(&map->genericMap, key, 0, value);
}

bool celix_longHashMap_putLong(celix_long_hash_map_t* map, long key, long value) {
    return celix_hashMap_putLong(&map->genericMap, NULL, key, value);
}

bool celix_stringHashMap_putDouble(celix_string_hash_map_t* map, const char* key, double value) {
    return celix_hashMap_putDouble(&map->genericMap, key, 0, value);
}

bool celix_longHashMap_putDouble(celix_long_hash_map_t* map, long key, double value) {
    return celix_hashMap_putDouble(&map->genericMap, NULL, key, value);
}

bool celix_stringHashMap_putBool(celix_string_hash_map_t* map, const char* key, bool value) {
    return celix_hashMap_putBool(&map->genericMap, key, 0, value);
}

bool celix_longHashMap_putBool(celix_long_hash_map_t* map, long key, bool value) {
    return celix_hashMap_putBool(&map->genericMap, NULL, key, value);
}

bool celix_stringHashMap_hasKey(const celix_string_hash_map_t* map, const char* key) {
    return celix_hashMap_hasKey(&map->genericMap, key, 0);
}

bool celix_longHashMap_hasKey(const celix_long_hash_map_t* map, long key) {
    return celix_hashMap_hasKey(&map->genericMap, NULL, key);
}

bool celix_stringHashMap_remove(celix_string_hash_map_t* map, const char* key) {
    return celix_hashMap_remove(&map->genericMap, key, 0);
}

bool celix_longHashMap_remove(celix_long_hash_map_t* map, long key) {
    return celix_hashMap_remove(&map->genericMap, NULL, key);
}

void celix_stringHashMap_clear(celix_string_hash_map_t* map) {
    celix_hashMap_clear(&map->genericMap);
}

void celix_longHashMap_clear(celix_long_hash_map_t* map) {
    celix_hashMap_clear(&map->genericMap);
}

celix_string_hash_map_iterator_t celix_stringHashMap_begin(const celix_string_hash_map_t* map) {
    celix_string_hash_map_iterator_t iter;
    memset(&iter, 0, sizeof(iter));
    iter._internal[0] = (void*)&map->genericMap;
    if (map->genericMap.size > 0) {
        celix_hash_map_entry_t *entry = celix_hashMap_firstEntry(&map->genericMap);
        iter._internal[1] = entry;
        iter.value = entry->value;
        iter.key = entry->key.strKey;
    }
    return iter;
}

celix_long_hash_map_iterator_t celix_longHashMap_begin(const celix_long_hash_map_t* map) {
    celix_long_hash_map_iterator_t iter;
    memset(&iter, 0, sizeof(iter));
    iter._internal[0] = (void*)&map->genericMap;
    if (map->genericMap.size > 0) {
        celix_hash_map_entry_t *entry = celix_hashMap_firstEntry(&map->genericMap);
        iter._internal[1] = entry;
        iter.value = entry->value;
        iter.key = entry->key.longKey;
    }
    return iter;
}

bool celix_stringHashMapIterator_isEnd(const celix_string_hash_map_iterator_t* iter) {
    return iter->_internal[1] == NULL; //check if entry is NULL
}

bool celix_longHashMapIterator_isEnd(const celix_long_hash_map_iterator_t* iter) {
    return iter->_internal[1] == NULL; //check if entry is NULL
}

void celix_stringHashMapIterator_next(celix_string_hash_map_iterator_t* iter) {
    const celix_hash_map_t* map = iter->_internal[0];
    celix_hash_map_entry_t *entry = iter->_internal[1];
    entry = celix_hashMap_nextEntry(map, entry);
    if (entry != NULL) {
        iter->_internal[1] = entry;
        iter->index += 1;
        iter->key = entry->key.strKey;
        iter->value = entry->value;
    } else {
        memset(iter, 0, sizeof(*iter));
        iter->_internal[0] = (void*)map;
    }
}

void celix_longHashMapIterator_next(celix_long_hash_map_iterator_t* iter) {
    const celix_hash_map_t* map = iter->_internal[0];
    celix_hash_map_entry_t *entry = iter->_internal[1];
    entry = celix_hashMap_nextEntry(map, entry);
    if (entry != NULL) {
        iter->_internal[1] = entry;
        iter->index += 1;
        iter->key = entry->key.longKey;
        iter->value = entry->value;
    } else {
        memset(iter, 0, sizeof(*iter));
        iter->_internal[0] = (void*)map;
    }
}

void celix_stringHashMapIterator_remove(celix_string_hash_map_iterator_t* iter) {
    celix_hash_map_t* map = iter->_internal[0];
    celix_hash_map_entry_t *entry = iter->_internal[1];
    const char* key = entry->key.strKey;
    celix_stringHashMapIterator_next(iter);
    celix_hashMap_remove(map, key, 0);
}

void celix_longHashMapIterator_remove(celix_long_hash_map_iterator_t* iter) {
    celix_hash_map_t* map = iter->_internal[0];
    celix_hash_map_entry_t *entry = iter->_internal[1];
    long key = entry->key.longKey;
    celix_longHashMapIterator_next(iter);
    celix_hashMap_remove(map, NULL, key);
}
