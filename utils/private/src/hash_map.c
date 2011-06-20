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
 * hash_map.c
 *
 *  Created on: Jul 21, 2010
 *      Author: alexanderb
 */
#include "celixbool.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "hash_map.h"
#include "hash_map_private.h"

static unsigned int DEFAULT_INITIAL_CAPACITY = 16;
static float DEFAULT_LOAD_FACTOR = 0.75f;
static unsigned int MAXIMUM_CAPACITY = 1 << 30;

unsigned int hashMap_hashCode(void * toHash) {
	intptr_t address = (intptr_t) toHash;
	return address;
}

int hashMap_equals(void * toCompare, void * compare) {
	return toCompare == compare;
}

int hashMap_entryEquals(HASH_MAP map, HASH_MAP_ENTRY entry, HASH_MAP_ENTRY compare) {
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

HASH_MAP hashMap_create(unsigned int (*keyHash)(void *), unsigned int (*valueHash)(void *),
		int (*keyEquals)(void *, void *), int (*valueEquals)(void *, void *)) {
	HASH_MAP map = (HASH_MAP) malloc(sizeof(*map));
	map->treshold = (int) (DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR);
	map->table = (HASH_MAP_ENTRY *) calloc(DEFAULT_INITIAL_CAPACITY, sizeof(HASH_MAP_ENTRY));
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

void hashMap_destroy(HASH_MAP map, bool freeKeys, bool freeValues) {
	hashMap_clear(map, freeKeys, freeValues);
	free(map->table);
	free(map);
	map = NULL;
}

int hashMap_size(HASH_MAP map) {
	return map->size;
}

bool hashMap_isEmpty(HASH_MAP map) {
	return hashMap_size(map) == 0;
}

void * hashMap_get(HASH_MAP map, void * key) {
	unsigned int hash;
	HASH_MAP_ENTRY entry;
	if (key == NULL) {
		HASH_MAP_ENTRY entry = map->table[0];
		for (entry = map->table[0]; entry != NULL; entry = entry->next) {
			if (entry->key == NULL) {
				return entry->value;
			}
		}
		return NULL;
	}

	hash = hashMap_hash(map->hashKey(key));
	entry = NULL;
	for (entry = map->table[hashMap_indexFor(hash, map->tablelength)]; entry != NULL; entry = entry->next) {
		if (entry->hash == hash && (entry->key == key || map->equalsKey(key, entry->key))) {
			return entry->value;
		}
	}
	return NULL;
}

bool hashMap_containsKey(HASH_MAP map, void * key) {
	return hashMap_getEntry(map, key) != NULL;
}

HASH_MAP_ENTRY hashMap_getEntry(HASH_MAP map, void * key) {
	unsigned int hash = (key == NULL) ? 0 : hashMap_hash(map->hashKey(key));
	HASH_MAP_ENTRY entry;
	int index = hashMap_indexFor(hash, map->tablelength);
	for (entry = map->table[index]; entry != NULL; entry = entry->next) {
		if (entry->hash == hash && (entry->key == key || map->equalsKey(key, entry->key))) {
			return entry;
		}
	}
	return NULL;
}

void * hashMap_put(HASH_MAP map, void * key, void * value) {
	unsigned int hash;
	int i;
	HASH_MAP_ENTRY entry;
	if (key == NULL) {
		HASH_MAP_ENTRY entry;
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

void hashMap_resize(HASH_MAP map, int newCapacity) {
	HASH_MAP_ENTRY * newTable;
	int j;
	if (map->tablelength == MAXIMUM_CAPACITY) {
		return;
	}

	newTable = (HASH_MAP_ENTRY *) calloc(newCapacity, sizeof(HASH_MAP_ENTRY));

	for (j = 0; j < map->tablelength; j++) {
		HASH_MAP_ENTRY entry = map->table[j];
		if (entry != NULL) {
			map->table[j] = NULL;
			do {
				HASH_MAP_ENTRY next = entry->next;
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

void * hashMap_remove(HASH_MAP map, void * key) {
	HASH_MAP_ENTRY entry = hashMap_removeEntryForKey(map, key);
	void * value = (entry == NULL ? NULL : entry->value);
	if (entry != NULL) {
		entry->key = NULL;
		entry->value = NULL;
		free(entry);
		entry = NULL;
	}
	return value;
}

void * hashMap_removeEntryForKey(HASH_MAP map, void * key) {
	unsigned int hash = (key == NULL) ? 0 : hashMap_hash(map->hashKey(key));
	int i = hashMap_indexFor(hash, map->tablelength);
	HASH_MAP_ENTRY prev = map->table[i];
	HASH_MAP_ENTRY entry = prev;

	while (entry != NULL) {
		HASH_MAP_ENTRY next = entry->next;
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

HASH_MAP_ENTRY hashMap_removeMapping(HASH_MAP map, HASH_MAP_ENTRY entry) {
	unsigned int hash;
	HASH_MAP_ENTRY prev;
	HASH_MAP_ENTRY e;
    int i;
	if (entry == NULL) {
		return NULL;
	}
	hash = (entry->key == NULL) ? 0 : hashMap_hash(map->hashKey(entry->key));
    i = hashMap_indexFor(hash, map->tablelength);
	prev = map->table[i];
	e = prev;

	while (e != NULL) {
		HASH_MAP_ENTRY next = e->next;
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

void hashMap_clear(HASH_MAP map, bool freeKey, bool freeValue) {
	int i;
	HASH_MAP_ENTRY * table;
	map->modificationCount++;
	table = map->table;

	for (i = 0; i < map->tablelength; i++) {
		HASH_MAP_ENTRY entry = table[i];
		while (entry != NULL) {
			HASH_MAP_ENTRY f = entry;
			entry = entry->next;
			if (freeKey && f->key != NULL)
				free(f->key);
			if (freeValue && f->value != NULL)
				free(f->value);
			free(f);
		}
		table[i] = NULL;
	}
	map->size = 0;
}

bool hashMap_containsValue(HASH_MAP map, void * value) {
	int i;
	if (value == NULL) {
		for (i = 0; i < map->tablelength; i++) {
			HASH_MAP_ENTRY entry;
			for (entry = map->table[i]; entry != NULL; entry = entry->next) {
				if (entry->value == NULL) {
					return true;
				}
			}
		}
		return false;
	}
	for (i = 0; i < map->tablelength; i++) {
		HASH_MAP_ENTRY entry;
		for (entry = map->table[i]; entry != NULL; entry = entry->next) {
			if (entry->value == value || map->equalsValue(entry->value, value)) {
				return true;
			}
		}
	}
	return false;
}

void hashMap_addEntry(HASH_MAP map, int hash, void * key, void * value, int bucketIndex) {
	HASH_MAP_ENTRY entry = map->table[bucketIndex];
	HASH_MAP_ENTRY new = (HASH_MAP_ENTRY) malloc(sizeof(*new));
	new->hash = hash;
	new->key = key;
	new->value = value;
	new->next = entry;
	map->table[bucketIndex] = new;
	if (map->size++ >= map->treshold) {
		hashMap_resize(map, 2 * map->tablelength);
	}
}

HASH_MAP_ITERATOR hashMapIterator_create(HASH_MAP map) {
	HASH_MAP_ITERATOR iterator = (HASH_MAP_ITERATOR) malloc(sizeof(*iterator));
	iterator->map = map;
	iterator->expectedModCount = map->modificationCount;
	iterator->index = 0;
	iterator->next = NULL;
	iterator->current = NULL;
	if (map->size > 0) {
		while (iterator->index < map->tablelength && (iterator->next = map->table[iterator->index++]) == NULL) {
		}
	}
	return iterator;
}

void hashMapIterator_destroy(HASH_MAP_ITERATOR iterator) {
	iterator->current = NULL;
	iterator->expectedModCount = 0;
	iterator->index = 0;
	iterator->map = NULL;
	iterator->next = NULL;
	free(iterator);
	iterator = NULL;
}

bool hashMapIterator_hasNext(HASH_MAP_ITERATOR iterator) {
	return iterator->next != NULL;
}

void hashMapIterator_remove(HASH_MAP_ITERATOR iterator) {
	void * key;
	HASH_MAP_ENTRY entry;
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

void * hashMapIterator_nextValue(HASH_MAP_ITERATOR iterator) {
	HASH_MAP_ENTRY entry;
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

void * hashMapIterator_nextKey(HASH_MAP_ITERATOR iterator) {
	HASH_MAP_ENTRY entry;
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

HASH_MAP_ENTRY hashMapIterator_nextEntry(HASH_MAP_ITERATOR iterator) {
	HASH_MAP_ENTRY entry;
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

HASH_MAP_KEY_SET hashMapKeySet_create(HASH_MAP map) {
	HASH_MAP_KEY_SET keySet = (HASH_MAP_KEY_SET) malloc(sizeof(*keySet));
	keySet->map = map;

	return keySet;
}

int hashMapKeySet_size(HASH_MAP_KEY_SET keySet) {
	return keySet->map->size;
}

bool hashMapKeySet_contains(HASH_MAP_KEY_SET keySet, void * key) {
	return hashMap_containsKey(keySet->map, key);
}

bool hashMapKeySet_remove(HASH_MAP_KEY_SET keySet, void * key) {
	HASH_MAP_ENTRY entry = hashMap_removeEntryForKey(keySet->map, key);
	bool removed = entry != NULL;
	free(entry);
	return removed;
}

void hashMapKeySet_clear(HASH_MAP_KEY_SET keySet) {
	hashMap_clear(keySet->map, false, false);
}

bool hashMapKeySet_isEmpty(HASH_MAP_KEY_SET keySet) {
	return hashMapKeySet_size(keySet) == 0;
}

HASH_MAP_VALUES hashMapValues_create(HASH_MAP map) {
	HASH_MAP_VALUES values = (HASH_MAP_VALUES) malloc(sizeof(*values));
	values->map = map;

	return values;
}

void hashMapValues_destroy(HASH_MAP_VALUES values) {
	values->map = NULL;
	free(values);
	values = NULL;
}

HASH_MAP_ITERATOR hashMapValues_iterator(HASH_MAP_VALUES values) {
	return hashMapIterator_create(values->map);
}

int hashMapValues_size(HASH_MAP_VALUES values) {
	return values->map->size;
}

bool hashMapValues_contains(HASH_MAP_VALUES values, void * value) {
	return hashMap_containsValue(values->map, value);
}

void hashMapValues_toArray(HASH_MAP_VALUES values, void* *array[], unsigned int *size) {
	HASH_MAP_ITERATOR it;
	int i;
    int vsize = hashMapValues_size(values);
    *size = vsize;
    *array = malloc(vsize * sizeof(*array));
    it = hashMapValues_iterator(values);
    i = 0;
    for (i = 0; i < vsize; i++) {
        if (!hashMapIterator_hasNext(it)) {
            return;
        }
        (*array)[i] = hashMapIterator_nextValue(it);
    }
}

bool hashMapValues_remove(HASH_MAP_VALUES values, void * value) {
	HASH_MAP_ITERATOR iterator = hashMapValues_iterator(values);
	if (value == NULL) {
		while (hashMapIterator_hasNext(iterator)) {
			if (hashMapIterator_nextValue(iterator) == NULL) {
				hashMapIterator_remove(iterator);
				return true;
			}
		}
	} else {
		while (hashMapIterator_hasNext(iterator)) {
			if (values->map->equalsValue(value, hashMapIterator_nextValue(iterator))) {
				hashMapIterator_remove(iterator);
				return true;
			}
		}
	}
	return false;
}

void hashMapValues_clear(HASH_MAP_VALUES values) {
	hashMap_clear(values->map, false, false);
}

bool hashMapValues_isEmpty(HASH_MAP_VALUES values) {
	return hashMapValues_size(values) == 0;
}

HASH_MAP_ENTRY_SET hashMapEntrySet_create(HASH_MAP map) {
	HASH_MAP_ENTRY_SET entrySet = (HASH_MAP_ENTRY_SET) malloc(sizeof(*entrySet));
	entrySet->map = map;

	return entrySet;
}

int hashMapEntrySet_size(HASH_MAP_ENTRY_SET entrySet) {
	return entrySet->map->size;
}

bool hashMapEntrySet_contains(HASH_MAP_ENTRY_SET entrySet, HASH_MAP_ENTRY entry) {
	return hashMap_containsValue(entrySet->map, entry);
}

bool hashMapEntrySet_remove(HASH_MAP_VALUES entrySet, HASH_MAP_ENTRY entry) {
	return hashMap_removeMapping(entrySet->map, entry) != NULL;
}

void hashMapEntrySet_clear(HASH_MAP_ENTRY_SET entrySet) {
	hashMap_clear(entrySet->map, false, false);
}

bool hashMapEntrySet_isEmpty(HASH_MAP_ENTRY_SET entrySet) {
	return hashMapEntrySet_size(entrySet) == 0;
}

void * hashMapEntry_getKey(HASH_MAP_ENTRY entry) {
	return entry->key;
}

void * hashMapEntry_getValue(HASH_MAP_ENTRY entry) {
	return entry->value;
}





