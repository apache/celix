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

#include <gtest/gtest.h>

#include "hash_map.h"
#include "hash_map_private.h"


static unsigned int test_hashKeyChar(const void * k);
static unsigned int test_hashValueChar(const void * v);
static int test_equalsKeyChar(const void * k, const void * o);
static int test_equalsValueChar(const void * v, const void * o);

class DeprecatedHashmapTestSuite : public ::testing::Test {
public:
  DeprecatedHashmapTestSuite() {
      map = hashMap_create(test_hashKeyChar, test_hashValueChar, test_equalsKeyChar, test_equalsValueChar);
      defaultMap = hashMap_create(nullptr, nullptr, nullptr, nullptr); //use default hash and equals functions
  }

  ~DeprecatedHashmapTestSuite() override {
      hashMap_destroy(map, false, false);
      hashMap_destroy(defaultMap, false, false);
  }

  DeprecatedHashmapTestSuite(DeprecatedHashmapTestSuite&&) = delete;
  DeprecatedHashmapTestSuite(const DeprecatedHashmapTestSuite&) = delete;
  DeprecatedHashmapTestSuite& operator=(DeprecatedHashmapTestSuite&&) = delete;
  DeprecatedHashmapTestSuite& operator=(const DeprecatedHashmapTestSuite&) = delete;

  hash_map_t* map{};
  hash_map_t* defaultMap{};
  hash_map_iterator_pt it_map{};
  hash_map_values_pt values{};
  hash_map_key_set_pt key_set{};
  hash_map_entry_set_pt entry_set{};
};

static unsigned int test_hashKeyChar(const void * k) {
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

static unsigned int test_hashValueChar(const void * v) {
    (void)(v);
    return 0;
}

static int test_equalsKeyChar(const void * k, const void * o) {
    return strcmp((char *)k, (char *) o) == 0;
}

static int test_equalsValueChar(const void * v, const void * o) {
    return strcmp((char *)v, (char *) o) == 0;
}

static char* my_strdup(const char* s){
    if(s==nullptr){
        return nullptr;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc (len + 1,sizeof(char));

    if (d == nullptr){
        return nullptr;
    }

    strncpy (d,s,len);
    return d;
}

TEST_F(DeprecatedHashmapTestSuite, create){
    EXPECT_TRUE(defaultMap != nullptr);
    EXPECT_EQ(0, defaultMap->size);
    // This fails on windows due to dllimport providing a proxy for exported functions.
    EXPECT_TRUE(hashMap_equals == defaultMap->equalsKey);
    EXPECT_TRUE(hashMap_equals == defaultMap->equalsValue);
    EXPECT_TRUE(hashMap_hashCode == defaultMap->hashKey);
    EXPECT_TRUE(hashMap_hashCode == defaultMap->hashValue);
}

TEST_F(DeprecatedHashmapTestSuite, size){
    char* key = my_strdup("key");
    char* value = my_strdup("value");
    char* key2 = my_strdup("key2");
    char* value2 = my_strdup("value2");
    char* key3 = my_strdup("key2");
    char* value3 = my_strdup("value3");

    EXPECT_EQ(0, defaultMap->size);

    // Add one entry
    hashMap_put(defaultMap, key, value);
    EXPECT_EQ(1, defaultMap->size);

    // Add second entry
    hashMap_put(defaultMap, key2, value2);
    EXPECT_EQ(2, defaultMap->size);

    // Add entry using a different key with the same str value, this does not overwrite an existing entry
    hashMap_put(defaultMap, key3, value3);
    EXPECT_EQ(3, defaultMap->size);

    // Clear map
    hashMap_clear(defaultMap, true, true);
    EXPECT_EQ(0, defaultMap->size);
}

TEST_F(DeprecatedHashmapTestSuite, isEmpty){
    char * key = my_strdup("key");
    char * value = my_strdup("value");

    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    // Add one entry
    hashMap_put(map, key, value);
    EXPECT_EQ(1, map->size);
    EXPECT_TRUE(!hashMap_isEmpty(map));

    // Remove entry
    hashMap_remove(map, key);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    free(key);
    free(value);
}

TEST_F(DeprecatedHashmapTestSuite, get){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * get;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    get = (char*) (char*) hashMap_get(map, key);
    EXPECT_STREQ(value, get);

    get = (char*) hashMap_get(map, key2);
    EXPECT_STREQ(value2, get);

    get = (char*) hashMap_get(map, neKey);
    EXPECT_EQ(nullptr, get);

    get = (char*) hashMap_get(map, nullptr);
    EXPECT_EQ(nullptr, get);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    get = (char*) hashMap_get(map, nullptr);
    EXPECT_STREQ(value3, get);

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, containsKey){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(hashMap_containsKey(map, key));
    EXPECT_TRUE(hashMap_containsKey(map, key2));
    EXPECT_TRUE(!hashMap_containsKey(map, neKey));
    EXPECT_TRUE(!hashMap_containsKey(map, nullptr));

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    EXPECT_TRUE(hashMap_containsKey(map, key3));

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, getEntry){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neKey = my_strdup("notExisting");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    hash_map_entry_pt entry;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);
    entry = hashMap_getEntry(map, key);
    EXPECT_STREQ(key, (char*) entry->key);
    EXPECT_STREQ(value, (char*)entry->value);

    entry = hashMap_getEntry(map, key2);
    EXPECT_STREQ(key2, (char*) entry->key);
    EXPECT_STREQ(value2, (char*) entry->value);

    entry = hashMap_getEntry(map, neKey);
    EXPECT_EQ(nullptr, entry);

    entry = hashMap_getEntry(map, nullptr);
    EXPECT_EQ(nullptr, entry);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key3);
    EXPECT_EQ(key3, entry->key);
    EXPECT_STREQ(value3, (char*) entry->value);

    free(neKey);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, put){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * nkey2 = my_strdup("key2");
    char * nvalue2 = my_strdup("value3");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = nullptr;
    char * old;
    char * get;

    // Add one entry
    hashMap_put(defaultMap, key, value);

    // Add second entry
    hashMap_put(defaultMap, key2, value2);

    get = (char*) hashMap_get(defaultMap, key);
    EXPECT_STREQ(value, get);

    get = (char*) hashMap_get(defaultMap, key2);
    EXPECT_STREQ(value2, get);

    // Try to add an entry with the same key, since no explicit hash function is used,
    //   this will not overwrite an existing entry.
    old = (char *) hashMap_put(defaultMap, nkey2, nvalue2);
    EXPECT_EQ(nullptr, old);

    // Retrieving the values will return the correct values
    get = (char*) hashMap_get(defaultMap, (void*)key2);
    EXPECT_STREQ(value2, get);
    get = (char*) hashMap_get(defaultMap, nkey2);
    EXPECT_STREQ(nvalue2, get);

    // Add third entry with nullptr key
    hashMap_put(defaultMap, key3, value3);

    get = (char*) hashMap_get(defaultMap, key3);
    EXPECT_STREQ(value3, get);

    // Add fourth entry with nullptr value
    hashMap_put(defaultMap, key4, value4);

    get =(char*)hashMap_get(defaultMap, key4);
    EXPECT_EQ(value4, get);

    hashMap_clear(defaultMap, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, resize){
    int i;
    char key[6];

    EXPECT_EQ(0, map->size);
    EXPECT_EQ(16, map->tablelength);
    EXPECT_EQ(12, map->treshold);
    for (i = 0; i < 12; i++) {
        snprintf(key, sizeof(key), "key%d", i);
        hashMap_put(map, my_strdup(key), my_strdup(key));
    }
    EXPECT_EQ(12, map->size);
    EXPECT_EQ(16, map->tablelength);
    EXPECT_EQ(12, map->treshold);

    snprintf(key, sizeof(key), "key%d", i);
    hashMap_put(map, my_strdup(key), my_strdup(key));
    EXPECT_EQ(13, map->size);
    EXPECT_EQ(32, map->tablelength);
    EXPECT_EQ(24, map->treshold);

    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, remove){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = nullptr;
    char * value2 = my_strdup("value2");
    char * removeKey;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry with null key
    hashMap_put(map, key2, value2);

    // Remove non-existing entry for map
    removeKey = my_strdup("nonexisting");
    hashMap_remove(map, removeKey);
    EXPECT_EQ(2, map->size);
    EXPECT_TRUE(!hashMap_isEmpty(map));

    hashMap_remove(map, key);
    EXPECT_EQ(1, map->size);

    hashMap_remove(map, key2);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    // Remove non-existing entry for empty map
    hashMap_remove(map, removeKey);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(removeKey);
}

TEST_F(DeprecatedHashmapTestSuite, removeMapping){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = nullptr;
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

    EXPECT_TRUE(entry1 != nullptr);
    EXPECT_TRUE(entry2 != nullptr);

    removed = hashMap_removeMapping(map, entry1);
    EXPECT_EQ(removed, entry1);
    EXPECT_EQ(1, map->size);

    removed = hashMap_removeMapping(map, entry2);
    EXPECT_EQ(removed, entry2);
    EXPECT_EQ(0, map->size);

    // Remove unexisting entry for empty map
    hashMap_removeMapping(map, nullptr);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(entry1);
    free(entry2);
}

TEST_F(DeprecatedHashmapTestSuite, clear){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = nullptr;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    // Add fourth entry with nullptr value
    hashMap_put(map, key4, value4);

    // Clear but leave keys and values intact
    hashMap_clear(map, false, false);
    EXPECT_EQ(0, map->size);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    // Add fourth entry with nullptr value
    hashMap_put(map, key4, value4);
    // Clear and clean up keys and values
    hashMap_clear(map, true, true);
    EXPECT_EQ(0, map->size);
}

TEST_F(DeprecatedHashmapTestSuite, containsValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * neValue = my_strdup("notExisting");
    char * key3 = my_strdup("key3");
    char * value3 = nullptr;

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(hashMap_containsValue(map, value));
    EXPECT_TRUE(hashMap_containsValue(map, value2));
    EXPECT_TRUE(!hashMap_containsValue(map, neValue));
    EXPECT_TRUE(!hashMap_containsValue(map, nullptr));

    // Add third entry with nullptr value
    hashMap_put(map, key3, value3);

    EXPECT_TRUE(hashMap_containsValue(map, value3));

    free(neValue);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, ValuestoArray){
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
    EXPECT_EQ(2, size);
    EXPECT_TRUE(hashMapValues_contains(values, array[0]));
    EXPECT_TRUE(hashMapValues_contains(values, array[1]));

    free(array);
    hashMapValues_destroy(values);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, entryGetKey){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * get;
    hash_map_entry_pt entry;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key);
    get = (char*) hashMapEntry_getKey(entry);
    EXPECT_STREQ(key, get);

    entry = hashMap_getEntry(map, key2);
    get = (char*) hashMapEntry_getKey(entry);
    EXPECT_STREQ(key2, get);

    entry = hashMap_getEntry(map, key3);
    get = (char*) hashMapEntry_getKey(entry);
    EXPECT_EQ(key3, get);

    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, entryGetValue){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * get;
    hash_map_entry_pt entry;

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);
    hashMap_put(map, key3, value3);

    entry = hashMap_getEntry(map, key);
    get = (char*) hashMapEntry_getValue(entry);
    EXPECT_STREQ(value, get);

    entry = hashMap_getEntry(map, key2);
    get = (char*) hashMapEntry_getValue(entry);
    EXPECT_STREQ(value2, get);

    entry = hashMap_getEntry(map, key3);
    get = (char*) hashMapEntry_getValue(entry);
    EXPECT_EQ(value3, get);

    hashMap_clear(map, true, true);
}

//----------------------HASH MAP ITERATOR TEST----------------------

TEST_F(DeprecatedHashmapTestSuite, createIteratorTest) {
    it_map = hashMapIterator_create(map);
    EXPECT_TRUE(it_map != nullptr);
    EXPECT_EQ(map, it_map->map);
    EXPECT_EQ(it_map->current, nullptr);
    EXPECT_EQ(map->modificationCount, it_map->expectedModCount);

    hashMapIterator_destroy(it_map);
}

TEST_F(DeprecatedHashmapTestSuite, hasNext){
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
    EXPECT_TRUE(hashMapIterator_hasNext(it_map));

    hashMapIterator_nextEntry(it_map);
    EXPECT_TRUE(hashMapIterator_hasNext(it_map));

    hashMapIterator_nextEntry(it_map);
    EXPECT_TRUE(hashMapIterator_hasNext(it_map));

    //third entry next points to nullptr
    hashMapIterator_nextEntry(it_map);
    EXPECT_TRUE(!hashMapIterator_hasNext(it_map));

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, removeIteratorTest) {
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

    //try to remove current (nullptr)
    hashMapIterator_remove(it_map);
    EXPECT_EQ(3, map->size);

    //delete the first and second entry
    hashMapIterator_nextEntry(it_map);
    hashMapIterator_remove(it_map);
    hashMapIterator_nextEntry(it_map);
    hashMapIterator_remove(it_map);
    EXPECT_EQ(1, map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    free(key3);
    free(value3);
    hashMapIterator_destroy(it_map);
}

TEST_F(DeprecatedHashmapTestSuite, nextValue){
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
    EXPECT_TRUE(hashMap_containsValue(map, getValue));

    getValue = (char*) hashMapIterator_nextValue(it_map);
    EXPECT_TRUE(hashMap_containsValue(map, getValue));

    getValue = (char*) hashMapIterator_nextValue(it_map);
    EXPECT_EQ(nullptr, getValue);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, nextKey){
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
    EXPECT_TRUE(hashMap_containsKey(map, getKey));

    getKey = (char*) hashMapIterator_nextKey(it_map);
    EXPECT_TRUE(hashMap_containsKey(map, getKey));

    getKey = (char*) hashMapIterator_nextKey(it_map);
    EXPECT_EQ(nullptr, getKey);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, nextEntry){
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
    EXPECT_STREQ((char*)get->value, (char*)getValue);

    get = (hash_map_entry_pt) hashMapIterator_nextEntry(it_map);
    getValue = (char*) hashMap_get(map, get->key);
    EXPECT_STREQ((char*)get->value, (char*)getValue);

    get = (hash_map_entry_pt) hashMapIterator_nextEntry(it_map);
    EXPECT_EQ(nullptr, get);

    hashMapIterator_destroy(it_map);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, valuesIterator){
    hash_map_values_pt values;
    hash_map_iterator_pt it_val;

    values = hashMapValues_create(map);
    it_val = hashMapValues_iterator(values);

    EXPECT_EQ(map, it_val->map);

    hashMapIterator_destroy(it_val);
    hashMapValues_destroy(values);
}

//----------------------HASH MAP VALUES TEST----------------------

TEST_F(DeprecatedHashmapTestSuite, createValues){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    EXPECT_TRUE(hashMapValues_contains(values, value));
    EXPECT_TRUE(hashMapValues_contains(values, value2));

    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST_F(DeprecatedHashmapTestSuite, sizeValues){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    EXPECT_EQ(2, hashMapValues_size(values));

    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST_F(DeprecatedHashmapTestSuite, removeValues){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);
    EXPECT_TRUE(hashMapValues_contains(values, value));
    EXPECT_TRUE(hashMapValues_contains(values, value2));

    hashMapValues_remove(values, value);
    EXPECT_TRUE(!hashMapValues_contains(values, value));
    EXPECT_TRUE(hashMapValues_contains(values, value2));

    hashMapValues_remove(values, value2);
    EXPECT_TRUE(!hashMapValues_contains(values, value));
    EXPECT_TRUE(!hashMapValues_contains(values, value2));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMap_clear(map, true, true);
    hashMapValues_destroy(values);
}

TEST_F(DeprecatedHashmapTestSuite, clearValues){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    EXPECT_TRUE(hashMapValues_contains(values, value));
    EXPECT_TRUE(hashMapValues_contains(values, value2));
    EXPECT_EQ(2, values->map->size);

    hashMapValues_clear(values);

    EXPECT_TRUE(!hashMapValues_contains(values, value));
    EXPECT_TRUE(!hashMapValues_contains(values, value2));
    EXPECT_EQ(0, values->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapValues_destroy(values);
}

TEST_F(DeprecatedHashmapTestSuite, isEmptyValues){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    //create Values from map
    values = hashMapValues_create(map);

    EXPECT_TRUE(!hashMapValues_isEmpty(values));

    hashMapValues_clear(values);

    EXPECT_TRUE(hashMapValues_isEmpty(values));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapValues_destroy(values);
}

//----------------------HASH MAP KEYSET TEST----------------------

TEST_F(DeprecatedHashmapTestSuite, createSet){
    key_set = hashMapKeySet_create(map);

    EXPECT_EQ(map, key_set->map);

    hashMapKeySet_destroy(key_set);
}

TEST_F(DeprecatedHashmapTestSuite, sizeSet){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    EXPECT_EQ(0, hashMapKeySet_size(key_set));

    // Add 2 entries
    hashMap_put(map, key, nullptr);
    hashMap_put(map, key2, nullptr);


    EXPECT_EQ(2, hashMapKeySet_size(key_set));

    hashMap_clear(map, true, true);
    hashMapKeySet_destroy(key_set);
}

TEST_F(DeprecatedHashmapTestSuite, containsSet){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    EXPECT_TRUE(!hashMapKeySet_contains(key_set, key));
    EXPECT_TRUE(!hashMapKeySet_contains(key_set, key2));

    // Add 2 entries
    hashMap_put(map, key, nullptr);

    EXPECT_TRUE(hashMapKeySet_contains(key_set, key));
    EXPECT_TRUE(!hashMapKeySet_contains(key_set, key2));

    hashMap_put(map, key2, nullptr);

    EXPECT_TRUE(hashMapKeySet_contains(key_set, key));
    EXPECT_TRUE(hashMapKeySet_contains(key_set, key2));

    hashMap_clear(map, true, true);
    hashMapKeySet_destroy(key_set);
}

TEST_F(DeprecatedHashmapTestSuite, removeSet){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, nullptr);
    hashMap_put(map, key2, nullptr);

    EXPECT_EQ(2, key_set->map->size);

    hashMapKeySet_remove(key_set, key);

    EXPECT_EQ(1, key_set->map->size);

    hashMapKeySet_remove(key_set, key2);

    EXPECT_EQ(0, key_set->map->size);

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

TEST_F(DeprecatedHashmapTestSuite, clearSet){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, nullptr);
    hashMap_put(map, key2, nullptr);

    EXPECT_EQ(2, key_set->map->size);

    hashMapKeySet_clear(key_set);

    EXPECT_EQ(0, key_set->map->size);

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

TEST_F(DeprecatedHashmapTestSuite, isEmptySet){
    char * key = my_strdup("key");
    char * key2 = my_strdup("key2");

    key_set = hashMapKeySet_create(map);

    EXPECT_TRUE(hashMapKeySet_isEmpty(key_set));

    // Add 2 entries
    hashMap_put(map, key, nullptr);
    hashMap_put(map, key2, nullptr);

    EXPECT_TRUE(!hashMapKeySet_isEmpty(key_set));

    hashMapKeySet_clear(key_set);

    EXPECT_TRUE(hashMapKeySet_isEmpty(key_set));

    free(key);
    free(key2);
    hashMapKeySet_destroy(key_set);
}

//----------------------HASH MAP ENTRYSET TEST----------------------

TEST_F(DeprecatedHashmapTestSuite, createEntrySet){
    entry_set = hashMapEntrySet_create(map);

    EXPECT_EQ(map, entry_set->map);

    hashMapEntrySet_destroy(entry_set);
}

TEST_F(DeprecatedHashmapTestSuite, sizeEntrySet){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");


    entry_set = hashMapEntrySet_create(map);

    EXPECT_EQ(0, hashMapEntrySet_size(entry_set));

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);


    EXPECT_EQ(2, hashMapEntrySet_size(entry_set));

    hashMap_clear(map, true, true);
    hashMapEntrySet_destroy(entry_set);
}

TEST_F(DeprecatedHashmapTestSuite, containsEntrySet){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(!hashMapEntrySet_contains(entry_set, nullptr));
    EXPECT_TRUE(!hashMapEntrySet_contains(entry_set, hashMap_getEntry(map, key)));
    EXPECT_TRUE(!hashMapEntrySet_contains(entry_set, hashMap_getEntry(map, key2)));

    hashMap_clear(map, true, true);
    hashMapEntrySet_destroy(entry_set);
}

TEST_F(DeprecatedHashmapTestSuite, removeEntrySet){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(!hashMapEntrySet_remove(entry_set, (hash_map_entry_pt) nullptr));
    EXPECT_EQ(2, entry_set->map->size);

    EXPECT_TRUE(hashMapEntrySet_remove(entry_set, hashMap_getEntry(map, key)));
    EXPECT_EQ(1, entry_set->map->size);

    EXPECT_TRUE(hashMapEntrySet_remove(entry_set, hashMap_getEntry(map, key2)));
    EXPECT_EQ(0, entry_set->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMap_clear(map, false, false);
    hashMapEntrySet_destroy(entry_set);
}

TEST_F(DeprecatedHashmapTestSuite, clearEntrySet){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    EXPECT_EQ(2, entry_set->map->size);

    hashMapEntrySet_clear(entry_set);

    EXPECT_EQ(0, entry_set->map->size);

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapEntrySet_destroy(entry_set);
}

TEST_F(DeprecatedHashmapTestSuite, isEmptyEntrySet){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");

    entry_set = hashMapEntrySet_create(map);

    EXPECT_TRUE(hashMapEntrySet_isEmpty(entry_set));

    // Add 2 entries
    hashMap_put(map, key, value);
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(!hashMapEntrySet_isEmpty(entry_set));

    hashMapEntrySet_clear(entry_set);

    EXPECT_TRUE(hashMapEntrySet_isEmpty(entry_set));

    free(key);
    free(value);
    free(key2);
    free(value2);
    hashMapEntrySet_destroy(entry_set);
}

//----------------------HASH MAP HASH TEST----------------------

TEST_F(DeprecatedHashmapTestSuite, getTest){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getKey = my_strdup("key");
    char * get;
    char * neKey = my_strdup("notExisting");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Get with new created key
    get = (char*) (char*) hashMap_get(map, getKey);
    EXPECT_TRUE(get != nullptr);
    EXPECT_STREQ(value, get);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, getKey);
    EXPECT_TRUE(get != nullptr);
    EXPECT_STREQ(value2, get);

    get = (char*) hashMap_get(map, neKey);
    EXPECT_EQ(nullptr, get);

    get = (char*) hashMap_get(map, nullptr);
    EXPECT_EQ(nullptr,get);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    get = (char*) hashMap_get(map, nullptr);
    EXPECT_STREQ(get, value3);

    free(getKey);
    free(neKey);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, containsKeyTest) {
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * containsKey = my_strdup("key");
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = my_strdup("key2");
    EXPECT_TRUE(hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = my_strdup("notExisting");
    EXPECT_TRUE(!hashMap_containsKey(map, containsKey));
    free(containsKey);
    containsKey = nullptr;
    EXPECT_TRUE(!hashMap_containsKey(map, containsKey));

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    containsKey = nullptr;
    EXPECT_TRUE(hashMap_containsKey(map, containsKey));

    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, getEntryTest){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getEntryKey;
    char * key3 = nullptr;
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
    EXPECT_TRUE(entry != nullptr);
    EXPECT_STREQ(key, (char*) entry->key);
    EXPECT_STREQ(value, (char*) entry->value);

    free(getEntryKey);
    getEntryKey = my_strdup("key2");
    entry = hashMap_getEntry(map, getEntryKey);
    EXPECT_TRUE(entry != nullptr);
    EXPECT_STREQ(key2, (char*) entry->key);
    EXPECT_STREQ(value2, (char*) entry->value);

    free(getEntryKey);
    getEntryKey = my_strdup("notExisting");
    entry = hashMap_getEntry(map, getEntryKey);
    EXPECT_EQ(nullptr, entry);

    free(getEntryKey);
    getEntryKey = nullptr;
    entry = hashMap_getEntry(map, getEntryKey);
    EXPECT_EQ(nullptr, entry);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    getEntryKey = nullptr;
    entry = hashMap_getEntry(map, getEntryKey);
    EXPECT_EQ(key3, entry->key);
    EXPECT_STREQ(value3, (char*) entry->value);

    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, putTest){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * getKey = my_strdup("key");
    char * get;
    char * nkey2 = my_strdup("key2");
    char * nvalue2 = my_strdup("value3");
    char * old;
    char * key3 = nullptr;
    char * value3 = my_strdup("value3");
    char * key4 = my_strdup("key4");
    char * value4 = nullptr;

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    // Get with new key
    get = (char*) hashMap_get(map, getKey);
    EXPECT_STREQ(value, get);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, getKey);
    EXPECT_STREQ(value2, get);

    // Overwrite existing entry
    old = (char *) hashMap_put(map, nkey2, nvalue2);
    EXPECT_TRUE(old != nullptr);
    EXPECT_STREQ(value2, old);

    free(getKey);
    getKey = my_strdup("key2");
    get = (char*) hashMap_get(map, key2);
    EXPECT_STREQ(nvalue2, get);

    // Add third entry with nullptr key
    hashMap_put(map, key3, value3);

    free(getKey);
    getKey = nullptr;
    get = (char*) hashMap_get(map, key3);
    EXPECT_STREQ(value3, get);

    // Add fourth entry with nullptr value
    hashMap_put(map, key4, value4);

    getKey = my_strdup("key4");
    get = (char*) hashMap_get(map, key4);
    EXPECT_EQ(value4, get);

    free(getKey);
    free(value2);
    free(nkey2);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, removeTest){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = nullptr;
    char * value2 = my_strdup("value2");
    char * removeKey = my_strdup("nonexisting");

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry with null key
    hashMap_put(map, key2, value2);

    // Remove non-existing entry for map
    hashMap_remove(map, removeKey);
    EXPECT_EQ(2, map->size);
    EXPECT_TRUE(!hashMap_isEmpty(map));

    // Remove entry with new key
    free(removeKey);
    removeKey = my_strdup("key");
    hashMap_remove(map, removeKey);
    EXPECT_EQ(1, map->size);

    free(removeKey);
    removeKey = nullptr;
    hashMap_remove(map, removeKey);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    // Remove non-existing entry for empty map
    removeKey = my_strdup("nonexisting");
    hashMap_remove(map, removeKey);
    EXPECT_EQ(0, map->size);
    EXPECT_TRUE(hashMap_isEmpty(map));

    free(removeKey);
    free(key);
    free(value);
    free(value2);
    hashMap_clear(map, true, true);
}

TEST_F(DeprecatedHashmapTestSuite, containsValueTest){
    char * key = my_strdup("key");
    char * value = my_strdup("value");
    char * key2 = my_strdup("key2");
    char * value2 = my_strdup("value2");
    char * containsValue = my_strdup("value");
    char * key3 = my_strdup("key3");
    char * value3 = nullptr;

    hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    EXPECT_TRUE(hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = my_strdup("value2");
    EXPECT_TRUE(hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = my_strdup("notExisting");
    EXPECT_TRUE(!hashMap_containsValue(map, containsValue));
    free(containsValue);
    containsValue = nullptr;
    EXPECT_TRUE(!hashMap_containsValue(map, containsValue));

    // Add third entry with nullptr value
    hashMap_put(map, key3, value3);

    containsValue = nullptr;
    EXPECT_TRUE(hashMap_containsValue(map, containsValue));

    hashMap_clear(map, true, true);
}
