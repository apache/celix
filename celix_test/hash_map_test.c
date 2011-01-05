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
 * hash_map_test.c
 *
 *  Created on: Jul 25, 2010
 *      Author: alexanderb
 */
#include <stdio.h>

#include "CUnit/Basic.h"

#include "hash_map.h"
#include "hash_map_private.h"

HASH_MAP map;

int setup(void) {
	printf("test\n");
	map = hashMap_create(NULL, NULL, NULL, NULL);
	if(map == NULL) {
		return 1;
	}
	return 0;
}

void test_hashMap_create(void) {
	CU_ASSERT_PTR_NOT_NULL_FATAL(map);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_EQUAL(map->equalsKey, hashMap_equals);
	CU_ASSERT_EQUAL(map->equalsValue, hashMap_equals);
	CU_ASSERT_EQUAL(map->hashKey, hashMap_hashCode);
	CU_ASSERT_EQUAL(map->hashValue, hashMap_hashCode);
}

void test_hashMap_size(void) {
	CU_ASSERT_EQUAL(map->size, 0);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);
	CU_ASSERT_EQUAL(map->size, 1);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);
	CU_ASSERT_EQUAL(map->size, 2);

	// Overwrite existing entry
	char * key3 = "key2";
	char * value3 = "value3";
	hashMap_put(map, key3, value3);
	CU_ASSERT_EQUAL(map->size, 2);

	// Clear map
	hashMap_clear(map);
	CU_ASSERT_EQUAL(map->size, 0);
}

void test_hashMap_isEmpty(void) {
	hashMap_clear(map);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);
	CU_ASSERT_EQUAL(map->size, 1);
	CU_ASSERT_FALSE(hashMap_isEmpty(map));

	// Remove entry
	hashMap_remove(map, key);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));
}

void test_hashMap_get(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	char * get = hashMap_get(map, key);
	CU_ASSERT_STRING_EQUAL(get, value);

	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, value2);

	char * neKey = "notExisting";
	get = hashMap_get(map, neKey);
	CU_ASSERT_EQUAL(get, NULL);

	get = hashMap_get(map, NULL);
	CU_ASSERT_EQUAL(get, NULL);

	// Add third entry with NULL key
	char * key3 = NULL;
	char * value3 = "value3";
	hashMap_put(map, key3, value3);

	get = hashMap_get(map, NULL);
	CU_ASSERT_STRING_EQUAL(get, value3);
}

void test_hashMap_containsKey(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	CU_ASSERT_TRUE(hashMap_containsKey(map, key));
	CU_ASSERT_TRUE(hashMap_containsKey(map, key2));
	char * neKey = "notExisting";
	CU_ASSERT_FALSE(hashMap_containsKey(map, neKey));
	CU_ASSERT_FALSE(hashMap_containsKey(map, NULL));

	// Add third entry with NULL key
	char * key3 = NULL;
	char * value3 = "value3";
	hashMap_put(map, key3, value3);

	CU_ASSERT_TRUE(hashMap_containsKey(map, key3));
}

void test_hashMap_getEntry(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	HASH_MAP_ENTRY entry = hashMap_getEntry(map, key);
	CU_ASSERT_STRING_EQUAL(entry->key, key);
	CU_ASSERT_STRING_EQUAL(entry->value, value);

	entry = hashMap_getEntry(map, key2);
	CU_ASSERT_STRING_EQUAL(entry->key, key2);
	CU_ASSERT_STRING_EQUAL(entry->value, value2);

	char * neKey = "notExisting";
	entry = hashMap_getEntry(map, neKey);
	CU_ASSERT_EQUAL(entry, NULL);

	entry = hashMap_getEntry(map, NULL);
	CU_ASSERT_EQUAL(entry, NULL);

	// Add third entry with NULL key
	char * key3 = NULL;
	char * value3 = "value3";
	hashMap_put(map, key3, value3);

	entry = hashMap_getEntry(map, key3);
	CU_ASSERT_EQUAL(entry->key, key3);
	CU_ASSERT_STRING_EQUAL(entry->value, value3);
}

void test_hashMap_put(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	char * get = hashMap_get(map, key);
	CU_ASSERT_STRING_EQUAL(get, value);

	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, value2);

	// Overwrite existing entry
	char * nkey2 = "key2";
	char * nvalue2 = "value3";
	char * old = (char *) hashMap_put(map, nkey2, nvalue2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(old);
	CU_ASSERT_STRING_EQUAL(old, value2)

	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, nvalue2);

	// Add third entry with NULL key
	char * key3 = NULL;
	char * value3 = "value3";
	hashMap_put(map, key3, value3);

	get = hashMap_get(map, key3);
	CU_ASSERT_STRING_EQUAL(get, value3);

	// Add fourth entry with NULL value
	char * key4 = "key4";
	char * value4 = NULL;
	hashMap_put(map, key4, value4);

	get = hashMap_get(map, key4);
	CU_ASSERT_EQUAL(get, value4);
}

void test_hashMap_resize(void) {
	hashMap_clear(map);

	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_EQUAL(map->tablelength, 16);
	CU_ASSERT_EQUAL(map->treshold, 12);
	int i;
	for (i = 0; i < 12; i++) {
		char key[6];
		sprintf(key, "key%d", i);
		char * k = strdup(key);
		hashMap_put(map, k, k);
	}
	CU_ASSERT_EQUAL(map->size, 12);
	CU_ASSERT_EQUAL(map->tablelength, 16);
	CU_ASSERT_EQUAL(map->treshold, 12);

	char key[6];
	sprintf(key, "key%d", i);
	hashMap_put(map, strdup(key), strdup(key));
	CU_ASSERT_EQUAL(map->size, 13);
	CU_ASSERT_EQUAL(map->tablelength, 32);
	CU_ASSERT_EQUAL(map->treshold, 24);
}

void test_hashMap_remove(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry with null key
	char * key2 = NULL;
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	// Remove unexisting entry for map
	char * removeKey = "unexisting";
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 2);
	CU_ASSERT_FALSE(hashMap_isEmpty(map));

	hashMap_remove(map, key);
	CU_ASSERT_EQUAL(map->size, 1);

	hashMap_remove(map, key2);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));

	// Remove unexisting entry for empty map
	removeKey = "unexisting";
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));
}

void test_hashMap_removeMapping(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry with null key
	char * key2 = NULL;
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	HASH_MAP_ENTRY entry1 = hashMap_getEntry(map, key);
	HASH_MAP_ENTRY entry2 = hashMap_getEntry(map, key2);

	CU_ASSERT_PTR_NOT_NULL_FATAL(entry1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(entry2);

	HASH_MAP_ENTRY removed = hashMap_removeMapping(map, entry1);
	CU_ASSERT_PTR_EQUAL(entry1, removed);
	CU_ASSERT_EQUAL(map->size, 1);

	removed = hashMap_removeMapping(map, entry2);
	CU_ASSERT_PTR_EQUAL(entry2, removed);
	CU_ASSERT_EQUAL(map->size, 0);

	// Remove unexisting entry for empty map
	hashMap_removeMapping(map, NULL);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));
}

void test_hashMap_clear(void) {
	hashMap_clear(map);
	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	// Add third entry with NULL key
	char * key3 = NULL;
	char * value3 = "value3";
	hashMap_put(map, key3, value3);

	// Add fourth entry with NULL value
	char * key4 = "key4";
	char * value4 = NULL;
	hashMap_put(map, key4, value4);

	hashMap_clear(map);
	CU_ASSERT_EQUAL(map->size, 0);
}

void test_hashMap_containsValue(void) {
	hashMap_clear(map);

	// Add one entry
	char * key = "key";
	char * value = "value";
	hashMap_put(map, key, value);

	// Add second entry
	char * key2 = "key2";
	char * value2 = "value2";
	hashMap_put(map, key2, value2);

	CU_ASSERT_TRUE(hashMap_containsValue(map, value));
	CU_ASSERT_TRUE(hashMap_containsValue(map, value2));
	char * neValue = "notExisting";
	CU_ASSERT_FALSE(hashMap_containsValue(map, neValue));
	CU_ASSERT_FALSE(hashMap_containsValue(map, NULL));

	// Add third entry with NULL value
	char * key3 = "key3";
	char * value3 = NULL;
	hashMap_put(map, key3, value3);

	CU_ASSERT_TRUE(hashMap_containsValue(map, value3));
}

int main (int argc, char** argv) {
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	  return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_1", setup, NULL);
	if (NULL == pSuite) {
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* add the tests to the suite */
	if (NULL == CU_add_test(pSuite, "Map Creation Test", test_hashMap_create)
		|| NULL == CU_add_test(pSuite, "Map Size Test", test_hashMap_size)
		|| NULL == CU_add_test(pSuite, "Map Is Empty Test", test_hashMap_isEmpty)
		|| NULL == CU_add_test(pSuite, "Map Get Test", test_hashMap_get)
		|| NULL == CU_add_test(pSuite, "Map Contains Key Test", test_hashMap_containsKey)
		|| NULL == CU_add_test(pSuite, "Map Get Entry Test", test_hashMap_getEntry)
		|| NULL == CU_add_test(pSuite, "Map Put Test", test_hashMap_put)
		|| NULL == CU_add_test(pSuite, "Map Resize Test", test_hashMap_resize)
		|| NULL == CU_add_test(pSuite, "Map Remove Test", test_hashMap_remove)
		|| NULL == CU_add_test(pSuite, "Map Remove Mapping Test", test_hashMap_removeMapping)
		|| NULL == CU_add_test(pSuite, "Map Clear Test", test_hashMap_clear)
		|| NULL == CU_add_test(pSuite, "Map Contains Value Test", test_hashMap_containsValue)


	)
	{
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
