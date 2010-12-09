/*
 * hash_map_private.h
 *
 *  Created on: Jul 21, 2010
 *      Author: alexanderb
 */

#ifndef HASH_MAP_PRIVATE_H_
#define HASH_MAP_PRIVATE_H_

#include "hash_map.h"

unsigned int hashMap_hashCode(void * toHash);
int hashMap_equals(void * toCompare, void * compare);

void hashMap_resize(HASH_MAP map, int newCapacity);
void * hashMap_removeEntryForKey(HASH_MAP map, void * key);
HASH_MAP_ENTRY hashMap_removeMapping(HASH_MAP map, HASH_MAP_ENTRY entry);
void hashMap_addEntry(HASH_MAP map, int hash, void * key, void * value, int bucketIndex);

struct hashMapEntry {
	void * key;
	void * value;
	HASH_MAP_ENTRY next;
	unsigned int hash;
};

struct hashMap {
	HASH_MAP_ENTRY * table;
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
	HASH_MAP map;
	HASH_MAP_ENTRY next;
	HASH_MAP_ENTRY current;
	int expectedModCount;
	int index;
};

struct hashMapKeySet {
	HASH_MAP map;
};

struct hashMapValues {
	HASH_MAP map;
};

struct hashMapEntrySet {
	HASH_MAP map;
};


#endif /* HASH_MAP_PRIVATE_H_ */
