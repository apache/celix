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
 * hash_map.c
 *
 *  \date       Jul 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "hash_map.h"
#include "hash_map_private.h"

static unsigned int DEFAULT_INITIAL_CAPACITY = 16;
static float DEFAULT_LOAD_FACTOR = 0.75f;
static unsigned int MAXIMUM_CAPACITY = 1 << 30;

unsigned int hashMap_hashCode(const void * toHash) {
    intptr_t address = (intptr_t) toHash;
    return address;
}

int hashMap_equals(const void * toCompare, const void * compare) {
    return toCompare == compare;
}

int hashMap_entryEquals(hash_map_pt map, hash_map_entry_pt entry, hash_map_entry_pt compare) {
    if (entry->key == compare->key || map->equalsKey(entry->key, compare->key)) {
        if (entry->value == compare->value || map->equalsValue(entry->value, compare->value)) {
            return true;
        }
    }
    return false;
}

static unsigned int hashMap_hash(unsigned int h) {
    h += ~(h << 9);
    h ^=  ((h >> 14) | (h << 18)); /* >>> */
    h +=  (h << 4);
    h ^=  ((h >> 10) | (h << 22)); /* >>> */
    return h;
}

static unsigned int hashMap_indexFor(unsigned int h, unsigned int length) {
    return h & (length - 1);
}

hash_map_pt hashMap_create(unsigned int (*keyHash)(const void *), unsigned int (*valueHash)(const void *),
        int (*keyEquals)(const void *, const void *), int (*valueEquals)(const void *, const void *)) {
    hash_map_pt map = (hash_map_pt) malloc(sizeof(*map));
    map->treshold = (unsigned int) (DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR);
    map->table = (hash_map_entry_pt *) calloc(DEFAULT_INITIAL_CAPACITY, sizeof(hash_map_entry_pt));
    map->size = 0;
    map->modificationCount = 0;
    map->tablelength = DEFAULT_INITIAL_CAPACITY;
    map->hashKey = hashMap_hashCode;
    map->hashValue = hashMap_hashCode;
    map->equalsKey = hashMap_equals;
    map->equalsValue = hashMap_equals;

    if (keyHash != NULL) {
        map->hashKey = keyHash;
    }
    if (valueHash != NULL) {
        map->hashValue = valueHash;
    }
    if (keyEquals != NULL) {
        map->equalsKey = keyEquals;
    }
    if (valueEquals != NULL) {
        map->equalsValue = valueEquals;
    }

    return map;
}

void hashMap_destroy(hash_map_pt map, bool freeKeys, bool freeValues) {
    hashMap_clear(map, freeKeys, freeValues);
    free(map->table);
    free(map);
}

int hashMap_size(hash_map_t *map) {
    return map->size;
}

bool hashMap_isEmpty(hash_map_pt map) {
    return hashMap_size(map) == 0;
}

void * hashMap_get(hash_map_pt map, const void* key) {
    unsigned int hash;
    if (key == NULL) {
        hash_map_entry_pt entry;
        for (entry = map->table[0]; entry != NULL; entry = entry->next) {
            if (entry->key == NULL) {
                return entry->value;
            }
        }
        return NULL;
    }

    hash = hashMap_hash(map->hashKey(key));
    hash_map_entry_pt entry = NULL;
    for (entry = map->table[hashMap_indexFor(hash, map->tablelength)]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && (entry->key == key || map->equalsKey(key, entry->key))) {
            return entry->value;
        }
    }
    return NULL;
}

bool hashMap_containsKey(hash_map_pt map, const void* key) {
    return hashMap_getEntry(map, key) != NULL;
}

hash_map_entry_pt hashMap_getEntry(hash_map_pt map, const void* key) {
    unsigned int hash = (key == NULL) ? 0 : hashMap_hash(map->hashKey(key));
    hash_map_entry_pt entry;
    int index = hashMap_indexFor(hash, map->tablelength);
    for (entry = map->table[index]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && (entry->key == key || map->equalsKey(key, entry->key))) {
            return entry;
        }
    }
    return NULL;
}

void * hashMap_put(hash_map_pt map, void * key, void * value) {
    unsigned int hash;
    int i;
    if (key == NULL) {
        hash_map_entry_pt entry;
        for (entry = map->table[0]; entry != NULL; entry = entry->next) {
            if (entry->key == NULL) {
                void * oldValue = entry->value;
                entry->value = value;
                return oldValue;
            }
        }
        map->modificationCount++;
        hashMap_addEntry(map, 0, NULL, value, 0);
        return NULL;
    }
    hash = hashMap_hash(map->hashKey(key));
    i = hashMap_indexFor(hash, map->tablelength);

    hash_map_entry_pt entry;
    for (entry = map->table[i]; entry != NULL; entry = entry->next) {
        if (entry->hash == hash && (entry->key == key || map->equalsKey(key, entry->key))) {
            void * oldValue = entry->value;
            entry->value = value;
            return oldValue;
        }
    }
    map->modificationCount++;
    hashMap_addEntry(map, hash, key, value, i);
    return NULL;
}

void hashMap_resize(hash_map_pt map, int newCapacity) {
    hash_map_entry_pt * newTable;
    unsigned int j;
    if (map->tablelength == MAXIMUM_CAPACITY) {
        return;
    }

    newTable = (hash_map_entry_pt *) calloc(newCapacity, sizeof(hash_map_entry_pt));

    for (j = 0; j < map->tablelength; j++) {
        hash_map_entry_pt entry = map->table[j];
        if (entry != NULL) {
            map->table[j] = NULL;
            do {
                hash_map_entry_pt next = entry->next;
                int i = hashMap_indexFor(entry->hash, newCapacity);
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

void * hashMap_remove(hash_map_pt map, const void* key) {
    hash_map_entry_pt entry = hashMap_removeEntryForKey(map, key);
    void * value = (entry == NULL ? NULL : entry->value);
    if (entry != NULL) {
        entry->key = NULL;
        entry->value = NULL;
        free(entry);
    }
    return value;
}

void * hashMap_removeFreeKey(hash_map_pt map, const void* key) {
    hash_map_entry_pt entry = hashMap_removeEntryForKey(map, key);
    void * value = (entry == NULL ? NULL : entry->value);
    if (entry != NULL) {
        free(entry->key);
        entry->key = NULL;
        entry->value = NULL;
        free(entry);
    }
    return value;
}

hash_map_entry_pt hashMap_removeEntryForKey(hash_map_pt map, const void* key) {
    unsigned int hash = (key == NULL) ? 0 : hashMap_hash(map->hashKey(key));
    int i = hashMap_indexFor(hash, map->tablelength);
    hash_map_entry_pt prev = map->table[i];
    hash_map_entry_pt entry = prev;

    while (entry != NULL) {
        hash_map_entry_pt next = entry->next;
        if (entry->hash == hash && (entry->key == key || (key != NULL && map->equalsKey(key, entry->key)))) {
            map->modificationCount++;
            map->size--;
            if (prev == entry) {
                map->table[i] = next;
            } else {
                prev->next = next;
            }
            return entry;
        }
        prev = entry;
        entry = next;
    }

    return entry;
}

hash_map_entry_pt hashMap_removeMapping(hash_map_pt map, hash_map_entry_pt entry) {
    unsigned int hash;
    hash_map_entry_pt prev;
    hash_map_entry_pt e;
    int i;
    if (entry == NULL) {
        return NULL;
    }
    hash = (entry->key == NULL) ? 0 : hashMap_hash(map->hashKey(entry->key));
    i = hashMap_indexFor(hash, map->tablelength);
    prev = map->table[i];
    e = prev;

    while (e != NULL) {
        hash_map_entry_pt next = e->next;
        if (e->hash == hash && hashMap_entryEquals(map, e, entry)) {
            map->modificationCount++;
            map->size--;
            if (prev == e) {
                map->table[i] = next;
            } else {
                prev->next = next;
            }
            return e;
        }
        prev = e;
        e = next;
    }

    return e;
}

void hashMap_clear(hash_map_pt map, bool freeKey, bool freeValue) {
    unsigned int i;
    hash_map_entry_pt * table;
    map->modificationCount++;
    table = map->table;

    for (i = 0; i < map->tablelength; i++) {
        hash_map_entry_pt entry = table[i];
        while (entry != NULL) {
            hash_map_entry_pt f = entry;
            entry = entry->next;
            hashMapEntry_clear(f, freeKey, freeValue);
            free(f);
        }
        table[i] = NULL;
    }
    map->size = 0;
}

bool hashMap_containsValue(hash_map_pt map, const void* value) {
    unsigned int i;
    if (value == NULL) {
        for (i = 0; i < map->tablelength; i++) {
            hash_map_entry_pt entry;
            for (entry = map->table[i]; entry != NULL; entry = entry->next) {
                if (entry->value == NULL) {
                    return true;
                }
            }
        }
        return false;
    }
    for (i = 0; i < map->tablelength; i++) {
        hash_map_entry_pt entry;
        for (entry = map->table[i]; entry != NULL; entry = entry->next) {
            if (entry->value == value || map->equalsValue(entry->value, value)) {
                return true;
            }
        }
    }
    return false;
}

void hashMap_addEntry(hash_map_pt map, int hash, void* key, void* value, int bucketIndex) {
    hash_map_entry_pt entry = map->table[bucketIndex];
    hash_map_entry_pt new = (hash_map_entry_pt) malloc(sizeof(*new));
    new->hash = hash;
    new->key = key;
    new->value = value;
    new->next = entry;
    map->table[bucketIndex] = new;
    if (map->size++ >= map->treshold) {
        hashMap_resize(map, 2 * map->tablelength);
    }
}

hash_map_iterator_pt hashMapIterator_alloc(void) {
    return calloc(1, sizeof(hash_map_iterator_t));
}

void hashMapIterator_dealloc(hash_map_iterator_pt iterator) {
    free(iterator);
}

hash_map_iterator_pt hashMapIterator_create(hash_map_pt map) {
    hash_map_iterator_pt iterator = hashMapIterator_alloc();
    hashMapIterator_init(map, iterator);
    return iterator;
}

hash_map_iterator_t hashMapIterator_construct(hash_map_pt map) {
    hash_map_iterator_t iter;
    memset(&iter, 0, sizeof(iter));
    hashMapIterator_init(map, &iter);
    return iter;
}

void hashMapIterator_init(hash_map_pt map, hash_map_iterator_pt iterator) {
    iterator->map = map;
    iterator->expectedModCount = map->modificationCount;
    iterator->index = 0;
    iterator->next = NULL;
    iterator->current = NULL;
    if (map->size > 0) {
        while (iterator->index < map->tablelength && (iterator->next = map->table[iterator->index++]) == NULL) {
        }
    }
}

void hashMapIterator_deinit(hash_map_iterator_pt iterator) {
    iterator->current = NULL;
    iterator->expectedModCount = 0;
    iterator->index = 0;
    iterator->map = NULL;
    iterator->next = NULL;
}

void hashMapIterator_destroy(hash_map_iterator_pt iterator) {
    hashMapIterator_deinit(iterator);
    hashMapIterator_dealloc(iterator);
}

bool hashMapIterator_hasNext(hash_map_iterator_pt iterator) {
    return iterator->next != NULL;
}

void hashMapIterator_remove(hash_map_iterator_pt iterator) {
    void * key;
    hash_map_entry_pt entry;
    if (iterator->current == NULL) {
        return;
    }
    if (iterator->expectedModCount != iterator->map->modificationCount) {
        return;
    }
    key = iterator->current->key;
    iterator->current = NULL;
    entry = hashMap_removeEntryForKey(iterator->map, key);
    free(entry);
    iterator->expectedModCount = iterator->map->modificationCount;
}

void * hashMapIterator_nextValue(hash_map_iterator_pt iterator) {
    hash_map_entry_pt entry;
    if (iterator->expectedModCount != iterator->map->modificationCount) {
        return NULL;
    }
    entry = iterator->next;
    if (entry == NULL) {
        return NULL;
    }
    if ((iterator->next = entry->next) == NULL) {
        while (iterator->index < iterator->map->tablelength && (iterator->next = iterator->map->table[iterator->index++]) == NULL) {
        }
    }
    iterator->current = entry;
    return entry->value;
}

void * hashMapIterator_nextKey(hash_map_iterator_pt iterator) {
    hash_map_entry_pt entry;
    if (iterator->expectedModCount != iterator->map->modificationCount) {
        return NULL;
    }
    entry = iterator->next;
    if (entry == NULL) {
        return NULL;
    }
    if ((iterator->next = entry->next) == NULL) {
        while (iterator->index < iterator->map->tablelength && (iterator->next = iterator->map->table[iterator->index++]) == NULL) {
        }
    }
    iterator->current = entry;
    return entry->key;
}

hash_map_entry_pt hashMapIterator_nextEntry(hash_map_iterator_pt iterator) {
    hash_map_entry_pt entry;
    if (iterator->expectedModCount != iterator->map->modificationCount) {
        return NULL;
    }
    entry = iterator->next;
    if (entry == NULL) {
        return NULL;
    }
    if ((iterator->next = entry->next) == NULL) {
        while (iterator->index < iterator->map->tablelength && (iterator->next = iterator->map->table[iterator->index++]) == NULL) {
        }
    }
    iterator->current = entry;
    return entry;
}

hash_map_key_set_pt hashMapKeySet_create(hash_map_pt map) {
    hash_map_key_set_pt keySet = (hash_map_key_set_pt) malloc(sizeof(*keySet));
    keySet->map = map;

    return keySet;
}

void hashMapKeySet_destroy(hash_map_key_set_pt keySet){
    keySet->map = NULL;
    free(keySet);
}

int hashMapKeySet_size(hash_map_key_set_pt keySet) {
    return keySet->map->size;
}

bool hashMapKeySet_contains(hash_map_key_set_pt keySet, const void* key) {
    return hashMap_containsKey(keySet->map, key);
}

bool hashMapKeySet_remove(hash_map_key_set_pt keySet, const void* key) {
    hash_map_entry_pt entry = hashMap_removeEntryForKey(keySet->map, key);
    bool removed = entry != NULL;
    free(entry);
    return removed;
}

void hashMapKeySet_clear(hash_map_key_set_pt keySet) {
    hashMap_clear(keySet->map, false, false);
}

bool hashMapKeySet_isEmpty(hash_map_key_set_pt keySet) {
    return hashMapKeySet_size(keySet) == 0;
}

hash_map_values_pt hashMapValues_create(hash_map_pt map) {
    hash_map_values_pt values = (hash_map_values_pt) malloc(sizeof(*values));
    values->map = map;

    return values;
}

void hashMapValues_destroy(hash_map_values_pt values) {
    values->map = NULL;
    free(values);
}

hash_map_iterator_pt hashMapValues_iterator(hash_map_values_pt values) {
    return hashMapIterator_create(values->map);
}

int hashMapValues_size(hash_map_values_pt values) {
    return values->map->size;
}

bool hashMapValues_contains(hash_map_values_pt values, const void* value) {
    return hashMap_containsValue(values->map, value);
}

void hashMapValues_toArray(hash_map_values_pt values, void* *array[], unsigned int *size) {
    hash_map_iterator_pt it;
    int i = 0;
    int vsize = hashMapValues_size(values);
    *size = vsize;
    *array = malloc(vsize * sizeof(**array));
    it = hashMapValues_iterator(values);
    while(hashMapIterator_hasNext(it) && i<vsize){
        (*array)[i++] = hashMapIterator_nextValue(it);
    }
    hashMapIterator_destroy(it);
}

bool hashMapValues_remove(hash_map_values_pt values, const void* value) {
    hash_map_iterator_pt iterator = hashMapValues_iterator(values);
    if (value == NULL) {
        while (hashMapIterator_hasNext(iterator)) {
            if (hashMapIterator_nextValue(iterator) == NULL) {
                hashMapIterator_remove(iterator);
                hashMapIterator_destroy(iterator);
                return true;
            }
        }
    } else {
        while (hashMapIterator_hasNext(iterator)) {
            if (values->map->equalsValue(value, hashMapIterator_nextValue(iterator))) {
                hashMapIterator_remove(iterator);
                hashMapIterator_destroy(iterator);
                return true;
            }
        }
    }
    hashMapIterator_destroy(iterator);
    return false;
}

void hashMapValues_clear(hash_map_values_pt values) {
    hashMap_clear(values->map, false, false);
}

bool hashMapValues_isEmpty(hash_map_values_pt values) {
    return hashMapValues_size(values) == 0;
}

hash_map_entry_set_pt hashMapEntrySet_create(hash_map_pt map) {
    hash_map_entry_set_pt entrySet = (hash_map_entry_set_pt) malloc(sizeof(*entrySet));
    entrySet->map = map;

    return entrySet;
}

void hashMapEntrySet_destroy(hash_map_entry_set_pt entrySet){
    entrySet->map = NULL;
    free(entrySet);
}

int hashMapEntrySet_size(hash_map_entry_set_pt entrySet) {
    return entrySet->map->size;
}

bool hashMapEntrySet_contains(hash_map_entry_set_pt entrySet, hash_map_entry_pt entry) {
    return hashMap_containsValue(entrySet->map, entry);
}

bool hashMapEntrySet_remove(hash_map_entry_set_pt entrySet, hash_map_entry_pt entry) {
    hash_map_entry_pt temp = hashMap_removeMapping(entrySet->map, entry);
    if (temp != NULL) {
        free(temp);
        return true;
    } else {
        return false;
    }
}

void hashMapEntrySet_clear(hash_map_entry_set_pt entrySet) {
    hashMap_clear(entrySet->map, false, false);
}

bool hashMapEntrySet_isEmpty(hash_map_entry_set_pt entrySet) {
    return hashMapEntrySet_size(entrySet) == 0;
}

void * hashMapEntry_getKey(hash_map_entry_pt entry) {
    return entry->key;
}

void * hashMapEntry_getValue(hash_map_entry_pt entry) {
    return entry->value;
}

void  hashMapEntry_clear(hash_map_entry_pt entry, bool freeKey, bool freeValue) {
    if (freeKey && entry->key != NULL) {
        free(entry->key);
        entry->key = NULL;
    }
    if (freeValue && entry->value != NULL) {
        free(entry->value);
        entry->value = NULL;
    }
}




