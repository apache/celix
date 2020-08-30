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
 * hash_map_test.cpp
 *
 *  \date       Sep 15, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "celixbool.h"

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "hash_map.h"
#include "hash_map_private.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
    if(s==NULL){
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc (len + 1,sizeof(char));

    if (d == NULL){
        return NULL;
    }

    strncpy (d,s,len);
    return d;
}

//Callback functions
unsigned int test_hashKeyChar(const void * k) {
    char * str = (char *) k;

    unsigned int hash = 1315423911;
    unsigned int i    = 0;
    unsigned int len = strlen(str);

    for(i = 0; i < len; str++, i++)
    {
        hash ^= ((hash << 5) + (*str) + (hash >> 2));
    }

    return hash;
}

unsigned int test_hashValueChar(const void * v) {
    (void)(v);
    return 0;
}

int test_equalsKeyChar(const void * k, const void * o) {
    return strcmp((char *)k, (char *) o) == 0;
}

int test_equalsValueChar(const void * v, const void * o) {
    return strcmp((char *)v, (char *) o) == 0;
}

//Tests group defines

TEST_GROUP(hash_map){
    hash_map_pt map;

    void setup() {
        map = hashMap_create(NULL, NULL, NULL, NULL);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

TEST_GROUP(hash_map_iterator){
    hash_map_pt map;
    hash_map_iterator_pt it_map;

    void setup() {
        map = hashMap_create(NULL, NULL, NULL, NULL);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

TEST_GROUP(hash_map_values){
    hash_map_pt map;
    hash_map_values_pt values;

    void setup() {
        map = hashMap_create(NULL, NULL, NULL, NULL);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

TEST_GROUP(hash_map_keySet){
    hash_map_pt map;
    hash_map_key_set_pt key_set;

    void setup() {
        map = hashMap_create(NULL, NULL, NULL, NULL);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

TEST_GROUP(hash_map_entrySet){
    hash_map_pt map;
    hash_map_entry_set_pt entry_set;

    void setup() {
        map = hashMap_create(NULL, NULL, NULL, NULL);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

TEST_GROUP(hash_map_hash) {
    hash_map_pt map;

    void setup(void) {
        map = hashMap_create(test_hashKeyChar, test_hashValueChar, test_equalsKeyChar, test_equalsValueChar);
    }
    void teardown() {
        hashMap_destroy(map, false, false);
    }
};

//----------------------HASH MAP DATA MANAGEMENT TEST----------------------

TEST(hash_map, create){
    CHECK(map != NULL);
    LONGS_EQUAL(0, map->size);
    // This fails on windows due to dllimport providing a proxy for exported functions.
    POINTERS_EQUAL(hashMap_equals, map->equalsKey);
    POINTERS_EQUAL(hashMap_equals, map->equalsValue);
    POINTERS_EQUAL(hashMap_hashCode, map->hashKey);
    POINTERS_EQUAL(hashMap_hashCode, map->hashValue);
}

TEST(hash_map, size){
    char* key = my_strdup("key");
    char* value = my_strdup("value");
    char* key2 = my_strdup("key2");
    char* value2 = my_strdup("value2");
    char* key3 = my_strdup("key2");
    char* value3 = my_strdup("value3");

    LONGS_EQUAL(0, map->size);

    // Add one entry
    hashMap_put(map, key, value);
    LONGS_EQUAL(1, map->size);

    // Add second entry
    hashMap_put(map, key2, value2);
    LONGS_EQUAL(2, map->size);

    // Add entry using the same key, this does not overwrite an existing entry
    hashMap_put(map, key3, value3);
    LONGS_EQUAL(3, map->size);

    // Clear map
    hashMap_clear(map, true, true);
    LONGS_EQUAL(0, map->size);
}

TEST(hash_map, isEmpty){
    char * key = my_strdup("key");
    char * value = my_strdup("value");

    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    // Add one entry
    hashMap_put(map, key, value);
    LONGS_EQUAL(1, map->size);
    CHECK(!hashMap_isEmpty(map));

    // Remove entry
    hashMap_remove(map, key);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    free(key);
    free(value);
}

TEST(hash_map, get){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * get;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    get = (char*) (char*) hashMap_get(map, key);
    STRCMP_EQUAL(value, get);

    get = (char*) hashMap_get(map, key2);
    STRCMP_EQUAL(value2, get);

    get = (char*) hashMap_get(map, neKey);
    POINTERS_EQUAL(NULL, get);

    get = (char*) hashMap_get(map, NULL);
    POINTERS_EQUAL(NULL, get);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    get = (char*) hashMap_get(map, NULL);
    STRCMP_EQUAL(value3, get);

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST(hash_map, containsKey){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    CHECK(hashMap_containsKey(map, key));
    CHECK(hashMap_containsKey(map, key2));
    CHECK(!hashMap_containsKey(map, neKey));
    CHECK(!hashMap_containsKey(map, NULL));

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    CHECK(hashMap_containsKey(map, key3));

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST(hash_map, getEntry){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    hash_map_entry_pt entry;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);
    entry = hashMap_getEntry(map, key);
    STRCMP_EQUAL(key, (char*) entry->key);
    STRCMP_EQUAL(value, (char*)entry->value);

    entry = hashMap_getEntry(map, key2);
    STRCMP_EQUAL(key2, (char*) entry->key);
    STRCMP_EQUAL(value2, (char*) entry->value);

    entry = hashMap_getEntry(map, neKey);
    POINTERS_EQUAL(NULL, entry);

    entry = hashMap_getEntry(map, NULL);
    POINTERS_EQUAL(NULL, entry);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key3);
    CHECK_EQUAL(key3, entry->key);
    STRCMP_EQUAL(value3, (char*) entry->value);

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST(hash_map, put){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * nkey2 = my_strdup("key2");
    char * nvalue2 = my_strdup("value3");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = NULL;
    char * old;
    char * get;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    get = (char*) hashMap_get(map, key);
    STRCMP_EQUAL(value, get);

    get = (char*) hashMap_get(map, key2);
    STRCMP_EQUAL(value2, get);

    // Try to add an entry with the same key, since no explicit hash function is used,
    //   this will not overwrite an existing entry.
    old = (char *) hashMap_put(map, nkey2, nvalue2);
    POINTERS_EQUAL(NULL, old);

    // Retrieving the values will return the correct values
    get = (char*) hashMap_get(map, (void*)key2);
    STRCMP_EQUAL(value2, get);
    get = (char*) hashMap_get(map, nkey2);
    STRCMP_EQUAL(nvalue2, get);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    get = (char*) hashMap_get(map, key3);
    STRCMP_EQUAL(value3, get);

    // Add fourth entry with NULL value
    hashMap_put(map, key4, value4);

    get =(char*)hashMap_get(map, key4);
    CHECK_EQUAL(value4, get);

    hashMap_clear(map, true, true);
}

TEST(hash_map, resize){
    int i;
    char key[6];

    LONGS_EQUAL(0, map->size);
    LONGS_EQUAL(16, map->tablelength);
    LONGS_EQUAL(12, map->treshold);
    for (i = 0; i < 12; i++) {
        char key[6];
        sprintf(key, "key%d", i);
        hashMap_put(map, my_strdup(key), my_strdup(key));
    }
    LONGS_EQUAL(12, map->size);
    LONGS_EQUAL(16, map->tablelength);
    LONGS_EQUAL(12, map->treshold);

    sprintf(key, "key%d", i);
    hashMap_put(map, my_strdup(key), my_strdup(key));
    LONGS_EQUAL(13, map->size);
    LONGS_EQUAL(32, map->tablelength);
    LONGS_EQUAL(24, map->treshold);

    hashMap_clear(map, true, true);
}

TEST(hash_map, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = NULL;
    char * value2 = my_strdup("value2");
    char * removeKey;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry with null key
    hashMap_put(map, key2, value2);

    // Remove non-existing entry for map
    removeKey = my_strdup("nonexisting");
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(2, map->size);
    CHECK(!hashMap_isEmpty(map));

    hashMap_remove(map, key);
    LONGS_EQUAL(1, map->size);

    hashMap_remove(map, key2);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    // Remove non-existing entry for empty map
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(removeKey);
}

TEST(hash_map, removeMapping){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = NULL;
    char * value2 = my_strdup("value2");
    hash_map_entry_pt entry1;
    hash_map_entry_pt entry2;
    hash_map_entry_pt removed;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry with null key
    hashMap_put(map, key2, value2);

    entry1 = hashMap_getEntry(map, key);
    entry2 = hashMap_getEntry(map, key2);

    CHECK(entry1 != NULL);
    CHECK(entry2 != NULL);

    removed = hashMap_removeMapping(map, entry1);
    POINTERS_EQUAL(removed, entry1);
    LONGS_EQUAL(1, map->size);

    removed = hashMap_removeMapping(map, entry2);
    POINTERS_EQUAL(removed, entry2);
    LONGS_EQUAL(0, map->size);

    // Remove unexisting entry for empty map
    hashMap_removeMapping(map, NULL);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(entry1);
    free(entry2);
}

TEST(hash_map, clear){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = NULL;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    // Add fourth entry with NULL value
    hashMap_put(map, key4, value4);

    // Clear but leave keys and values intact
    hashMap_clear(map, false, false);
    LONGS_EQUAL(0, map->size);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    // Add fourth entry with NULL value
    hashMap_put(map, key4, value4);
    // Clear and clean up keys and values
    hashMap_clear(map, true, true);
    LONGS_EQUAL(0, map->size);
}

TEST(hash_map, containsValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neValue = my_strdup("notExisting");
    char * key3 = my_strdup("key3");
    char * value3 = NULL;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    CHECK(hashMap_containsValue(map, value));
    CHECK(hashMap_containsValue(map, value2));
    CHECK(!hashMap_containsValue(map, neValue));
    CHECK(!hashMap_containsValue(map, NULL));

    // Add third entry with NULL value
    hashMap_put(map, key3, value3);

    CHECK(hashMap_containsValue(map, value3));

    free(neValue);
    hashMap_clear(map, true, true);
}

TEST(hash_map, ValuestoArray){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char **array;
    unsigned int size;
    hash_map_values_pt values;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    values = hashMapValues_create(map);
    hashMapValues_toArray(values, (void***)&array, &size);
    LONGS_EQUAL(2, size);
    CHECK(hashMapValues_contains(values, array[0]));
    CHECK(hashMapValues_contains(values, array[1]));

    free(array);
    hashMapValues_destroy(values);
    hashMap_clear(map, true, true);
}

TEST(hash_map, entryGetKey){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * get;
    hash_map_entry_pt entry;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key);
    get = (char*) hashMapEntry_getKey(entry);
    STRCMP_EQUAL(key, get);

    entry = hashMap_getEntry(map, key2);
    get = (char*) hashMapEntry_getKey(entry);
    STRCMP_EQUAL(key2, get);

    entry = hashMap_getEntry(map, key3);
    get = (char*) hashMapEntry_getKey(entry);
    POINTERS_EQUAL(key3, get);

    hashMap_clear(map, true, true);
}

TEST(hash_map, entryGetValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * get;
    hash_map_entry_pt entry;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key);
    get = (char*) hashMapEntry_getValue(entry);
    STRCMP_EQUAL(value, get);

    entry = hashMap_getEntry(map, key2);
    get = (char*) hashMapEntry_getValue(entry);
    STRCMP_EQUAL(value2, get);

    entry = hashMap_getEntry(map, key3);
    get = (char*) hashMapEntry_getValue(entry);
    POINTERS_EQUAL(value3, get);

    hashMap_clear(map, true, true);
}

//----------------------HASH MAP ITERATOR TEST----------------------

TEST(hash_map_iterator, create){
    it_map = hashMapIterator_create(map);
    CHECK(it_map != NULL);
    POINTERS_EQUAL(map, it_map->map);
    POINTERS_EQUAL(it_map->current, NULL);
    LONGS_EQUAL(map->modificationCount, it_map->expectedModCount);

    hashMapIterator_destroy(it_map);
}

TEST(hash_map_iterator, hasNext){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = my_strdup("key3");
    char * value3 = my_strdup("value3");

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Add third entry
    hashMap_put(map, key3, value3);

    //create it_map from map
    it_map = hashMapIterator_create(map);

    //check hasNext
    CHECK(hashMapIterator_hasNext(it_map));

    hashMapIterator_nextEntry(it_map);
    CHECK(hashMapIterator_hasNext(it_map));

    hashMapIterator_nextEntry(it_map);
    CHECK(hashMapIterator_hasNext(it_map));

    //third entry next points to NULL
    hashMapIterator_nextEntry(it_map);
    CHECK(!hashMapIterator_hasNext(it_map));

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST(hash_map_iterator, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = my_strdup("key3");
    char * value3 = my_strdup("value3");

    // Add 3 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);
    hashMap_put(map, key3, value3);

    //create it_map from map
    it_map = hashMapIterator_create(map);

    //try to remove current (NULL)
    hashMapIterator_remove(it_map);
    LONGS_EQUAL(3, map->size);

    //delete the first and second entry
    hashMapIterator_nextEntry(it_map);
    hashMapIterator_remove(it_map);
    hashMapIterator_nextEntry(it_map);
    hashMapIterator_remove(it_map);
    LONGS_EQUAL(1, map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(key3);
    free(value3);
    hashMapIterator_destroy(it_map);
}

TEST(hash_map_iterator, nextValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getValue;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create it_map from map
    it_map = hashMapIterator_create(map);

    getValue = (char*) hashMapIterator_nextValue(it_map);
    CHECK(hashMap_containsValue(map, getValue));

    getValue = (char*) hashMapIterator_nextValue(it_map);
    CHECK(hashMap_containsValue(map, getValue));

    getValue = (char*) hashMapIterator_nextValue(it_map);
    POINTERS_EQUAL(NULL, getValue);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST(hash_map_iterator, nextKey){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getKey;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create it_map from map
    it_map = hashMapIterator_create(map);

    getKey = (char*) hashMapIterator_nextKey(it_map);
    CHECK(hashMap_containsKey(map, getKey));

    getKey = (char*) hashMapIterator_nextKey(it_map);
    CHECK(hashMap_containsKey(map, getKey));

    getKey = (char*) hashMapIterator_nextKey(it_map);
    POINTERS_EQUAL(NULL, getKey);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST(hash_map_iterator, nextEntry){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getValue;
    hash_map_entry_pt get;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create it_map from map
    it_map = hashMapIterator_create(map);

    get = (hash_map_entry_pt) hashMapIterator_nextEntry(it_map);
    getValue = (char*) hashMap_get(map, get->key);
    STRCMP_EQUAL((char*)get->value, (char*)getValue);

    get = (hash_map_entry_pt) hashMapIterator_nextEntry(it_map);
    getValue = (char*) hashMap_get(map, get->key);
    STRCMP_EQUAL((char*)get->value, (char*)getValue);

    get = (hash_map_entry_pt) hashMapIterator_nextEntry(it_map);
    POINTERS_EQUAL(NULL, get);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST(hash_map_iterator, valuesIterator){
    hash_map_values_pt values;
    hash_map_iterator_pt it_val;

    values = hashMapValues_create(map);
    it_val = hashMapValues_iterator(values);

    POINTERS_EQUAL(map, it_val->map);

    hashMapIterator_destroy(it_val);
    hashMapValues_destroy(values);
}

//----------------------HASH MAP VALUES TEST----------------------

TEST(hash_map_values, create){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    CHECK(hashMapValues_contains(values, value));
    CHECK(hashMapValues_contains(values, value2));

    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST(hash_map_values, size){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    LONGS_EQUAL(2, hashMapValues_size(values));

    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST(hash_map_values, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);
    CHECK(hashMapValues_contains(values, value));
    CHECK(hashMapValues_contains(values, value2));

    hashMapValues_remove(values, value);
    CHECK(!hashMapValues_contains(values, value));
    CHECK(hashMapValues_contains(values, value2));

    hashMapValues_remove(values, value2);
    CHECK(!hashMapValues_contains(values, value));
    CHECK(!hashMapValues_contains(values, value2));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST(hash_map_values, clear){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    CHECK(hashMapValues_contains(values, value));
    CHECK(hashMapValues_contains(values, value2));
    LONGS_EQUAL(2, values->map->size);

    hashMapValues_clear(values);

    CHECK(!hashMapValues_contains(values, value));
    CHECK(!hashMapValues_contains(values, value2));
    LONGS_EQUAL(0, values->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapValues_destroy(values);
}

TEST(hash_map_values, isEmpty){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    CHECK(!hashMapValues_isEmpty(values));

    hashMapValues_clear(values);

    CHECK(hashMapValues_isEmpty(values));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapValues_destroy(values);
}

//----------------------HASH MAP KEYSET TEST----------------------

TEST(hash_map_keySet, create){
    key_set = hashMapKeySet_create(map);

    POINTERS_EQUAL(map, key_set->map);

    hashMapKeySet_destroy(key_set);
}

TEST(hash_map_keySet, size){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    LONGS_EQUAL(0, hashMapKeySet_size(key_set));

    // Add 2 entries
    hashMap_put(map, key, NULL);
    hashMap_put(map, key2, NULL);


    LONGS_EQUAL(2, hashMapKeySet_size(key_set));

    hashMap_clear(map, true, true);
    hashMapKeySet_destroy(key_set);
}

TEST(hash_map_keySet, contains){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    CHECK(!hashMapKeySet_contains(key_set, key));
    CHECK(!hashMapKeySet_contains(key_set, key2));

    // Add 2 entries
    hashMap_put(map, key, NULL);

    CHECK(hashMapKeySet_contains(key_set, key));
    CHECK(!hashMapKeySet_contains(key_set, key2));

    hashMap_put(map, key2, NULL);

    CHECK(hashMapKeySet_contains(key_set, key));
    CHECK(hashMapKeySet_contains(key_set, key2));

    hashMap_clear(map, true, true);
    hashMapKeySet_destroy(key_set);
}

TEST(hash_map_keySet, remove){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, NULL);
    hashMap_put(map, key2, NULL);

    LONGS_EQUAL(2, key_set->map->size);

    hashMapKeySet_remove(key_set, key);

    LONGS_EQUAL(1, key_set->map->size);

    hashMapKeySet_remove(key_set, key2);

    LONGS_EQUAL(0, key_set->map->size);

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

TEST(hash_map_keySet, clear){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, NULL);
    hashMap_put(map, key2, NULL);

    LONGS_EQUAL(2, key_set->map->size);

    hashMapKeySet_clear(key_set);

    LONGS_EQUAL(0, key_set->map->size);

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

TEST(hash_map_keySet, isEmpty){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    CHECK(hashMapKeySet_isEmpty(key_set));

    // Add 2 entries
    hashMap_put(map, key, NULL);
    hashMap_put(map, key2, NULL);

    CHECK(!hashMapKeySet_isEmpty(key_set));

    hashMapKeySet_clear(key_set);

    CHECK(hashMapKeySet_isEmpty(key_set));

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

//----------------------HASH MAP ENTRYSET TEST----------------------

TEST(hash_map_entrySet, create){
    entry_set = hashMapEntrySet_create(map);

    POINTERS_EQUAL(map, entry_set->map);

    hashMapEntrySet_destroy(entry_set);
}

TEST(hash_map_entrySet, size){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");


    entry_set = hashMapEntrySet_create(map);

    LONGS_EQUAL(0, hashMapEntrySet_size(entry_set));

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);


    LONGS_EQUAL(2, hashMapEntrySet_size(entry_set));

    hashMap_clear(map, true, true);
    hashMapEntrySet_destroy(entry_set);
}

TEST(hash_map_entrySet, contains){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    CHECK(!hashMapEntrySet_contains(entry_set, NULL));
    CHECK(!hashMapEntrySet_contains(entry_set, hashMap_getEntry(map, key)));
    CHECK(!hashMapEntrySet_contains(entry_set, hashMap_getEntry(map, key2)));

    hashMap_clear(map, true, true);
    hashMapEntrySet_destroy(entry_set);
}

TEST(hash_map_entrySet, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    CHECK(!hashMapEntrySet_remove(entry_set, (hash_map_entry_pt) NULL));
    LONGS_EQUAL(2, entry_set->map->size);

    CHECK(hashMapEntrySet_remove(entry_set, hashMap_getEntry(map, key)));
    LONGS_EQUAL(1, entry_set->map->size);

    CHECK(hashMapEntrySet_remove(entry_set, hashMap_getEntry(map, key2)));
    LONGS_EQUAL(0, entry_set->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMap_clear(map, false, false);
    hashMapEntrySet_destroy(entry_set);
}

TEST(hash_map_entrySet, clear){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    LONGS_EQUAL(2, entry_set->map->size);

    hashMapEntrySet_clear(entry_set);

    LONGS_EQUAL(0, entry_set->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapEntrySet_destroy(entry_set);
}

TEST(hash_map_entrySet, isEmpty){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    CHECK(hashMapEntrySet_isEmpty(entry_set));

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    CHECK(!hashMapEntrySet_isEmpty(entry_set));

    hashMapEntrySet_clear(entry_set);

    CHECK(hashMapEntrySet_isEmpty(entry_set));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapEntrySet_destroy(entry_set);
}

//----------------------HASH MAP HASH TEST----------------------

TEST(hash_map_hash, get){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getKey = my_strdup("key");
    char * get;
    char * neKey = my_strdup("notExisting");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Get with new created key
    get = (char*) (char*) hashMap_get(map, getKey);
    CHECK_C(get != NULL);
    STRCMP_EQUAL(value, get);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, getKey);
    CHECK_C(get != NULL);
    STRCMP_EQUAL(value2, get);

    get = (char*) hashMap_get(map, neKey);
    POINTERS_EQUAL(NULL, get);

    get = (char*) hashMap_get(map, NULL);
    POINTERS_EQUAL(NULL,get);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    get = (char*) hashMap_get(map, NULL);
    STRCMP_EQUAL(get, value3);

    free(getKey);
    free(neKey);
    hashMap_clear(map, true, true);
}

TEST(hash_map_hash, containsKey) {
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * containsKey = my_strdup("key");
    char * key3 = NULL;
    char * value3 = my_strdup("value3");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    CHECK(hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = my_strdup("key2");
    CHECK(hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = my_strdup("notExisting");
    CHECK(!hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = NULL;
    CHECK(!hashMap_containsKey(map, containsKey));

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    containsKey = NULL;
    CHECK(hashMap_containsKey(map, containsKey));

    hashMap_clear(map, true, true);
}

TEST(hash_map_hash, getEntry){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getEntryKey;
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    hash_map_entry_pt entry;

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Get with new created key
    getEntryKey = my_strdup("key");
    entry = hashMap_getEntry(map, getEntryKey);
    CHECK(entry != NULL);
    STRCMP_EQUAL(key, (char*) entry->key);
    STRCMP_EQUAL(value, (char*) entry->value);

    free(getEntryKey);
    getEntryKey = my_strdup("key2");
    entry = hashMap_getEntry(map, getEntryKey);
    CHECK(entry != NULL);
    STRCMP_EQUAL(key2, (char*) entry->key);
    STRCMP_EQUAL(value2, (char*) entry->value);

    free(getEntryKey);
    getEntryKey = my_strdup("notExisting");
    entry = hashMap_getEntry(map, getEntryKey);
    POINTERS_EQUAL(NULL, entry);

    free(getEntryKey);
    getEntryKey = NULL;
    entry = hashMap_getEntry(map, getEntryKey);
    POINTERS_EQUAL(NULL, entry);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    getEntryKey = NULL;
    entry = hashMap_getEntry(map, getEntryKey);
    CHECK_EQUAL(key3, entry->key);
    STRCMP_EQUAL(value3, (char*) entry->value);

    hashMap_clear(map, true, true);
}

TEST(hash_map_hash, put){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getKey = my_strdup("key");
    char * get;
    char * nkey2 = my_strdup("key2");
    char * nvalue2 = my_strdup("value3");
    char * old;
    char * key3 = NULL;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = NULL;

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Get with new key
    get = (char*) hashMap_get(map, getKey);
    STRCMP_EQUAL(value, get);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, getKey);
    STRCMP_EQUAL(value2, get);

    // Overwrite existing entry
    old = (char *) hashMap_put(map, nkey2, nvalue2);
    CHECK(old != NULL);
    STRCMP_EQUAL(value2, old);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, key2);
    STRCMP_EQUAL(nvalue2, get);

    // Add third entry with NULL key
    hashMap_put(map, key3, value3);

    free(getKey);
    getKey = NULL;
    get = (char*) hashMap_get(map, key3);
    STRCMP_EQUAL(value3, get);

    // Add fourth entry with NULL value
    hashMap_put(map, key4, value4);

    getKey = my_strdup("key4");
    get = (char*) hashMap_get(map, key4);
    CHECK_EQUAL(value4, get);

    free(getKey);
    free(value2);
    free(nkey2);
    hashMap_clear(map, true, true);
}

TEST(hash_map_hash, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = NULL;
    char * value2 = my_strdup("value2");
    char * removeKey = my_strdup("nonexisting");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry with null key
    hashMap_put(map, key2, value2);

    // Remove non-existing entry for map
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(2, map->size);
    CHECK(!hashMap_isEmpty(map));

    // Remove entry with new key
    free(removeKey);
    removeKey = my_strdup("key");
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(1, map->size);

    free(removeKey);
    removeKey = NULL;
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    // Remove non-existing entry for empty map
    removeKey = my_strdup("nonexisting");
    hashMap_remove(map, removeKey);
    LONGS_EQUAL(0, map->size);
    CHECK(hashMap_isEmpty(map));

    free(removeKey);
    free(key);
    free(value);
    free(value2);
    hashMap_clear(map, true, true);
}

TEST(hash_map_hash, containsValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * containsValue = my_strdup("value");
    char * key3 = my_strdup("key3");
    char * value3 = NULL;

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    CHECK(hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = my_strdup("value2");
    CHECK(hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = my_strdup("notExisting");
    CHECK(!hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = NULL;
    CHECK(!hashMap_containsValue(map, containsValue));

    // Add third entry with NULL value
    hashMap_put(map, key3, value3);

    containsValue = NULL;
    CHECK(hashMap_containsValue(map, containsValue));

    hashMap_clear(map, true, true);
}
