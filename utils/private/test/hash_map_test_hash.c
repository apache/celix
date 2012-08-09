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
 *  \date       Jul 25, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <Automated.h>

#include "hash_map.h"
#include "hash_map_private.h"

HASH_MAP map;

unsigned int test_hashKeyChar(void * k) {
	char * str = (char *) k;

	unsigned int hash = 1315423911;
	unsigned int i    = 0;
	int len = strlen(str);

	for(i = 0; i < len; str++, i++)
	{
	  hash ^= ((hash << 5) + (*str) + (hash >> 2));
	}

	return hash;
}

unsigned int test_hashValueChar(void * v) {
	return 0;
}

int test_equalsKeyChar(void * k, void * o) {
	return strcmp((char *)k, (char *) o) == 0;
}

int test_equalsValueChar(void * v, void * o) {
	return strcmp((char *)v, (char *) o) == 0;
}

int setup(void) {
	printf("test\n");
	map = hashMap_create(test_hashKeyChar, test_hashValueChar, test_equalsKeyChar, test_equalsValueChar);
	if(map == NULL) {
		return 1;
	}
	return 0;
}

void test_hashMap_get(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * getKey = strdup("key");
	char * get;
	char * neKey;
	char * key3 = NULL;
	char * value3 = "value3";

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	// Get with new created key
	getKey = strdup("key");
	get = hashMap_get(map, getKey);
	CU_ASSERT_PTR_NOT_NULL_FATAL(get);
	CU_ASSERT_STRING_EQUAL(get, value);

	getKey = strdup("key2");
	get = hashMap_get(map, getKey);
	CU_ASSERT_PTR_NOT_NULL_FATAL(get);
	CU_ASSERT_STRING_EQUAL(get, value2);

	neKey = strdup("notExisting");
	get = hashMap_get(map, neKey);
	CU_ASSERT_EQUAL(get, NULL);

	get = hashMap_get(map, NULL);
	CU_ASSERT_EQUAL(get, NULL);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	get = hashMap_get(map, NULL);
	CU_ASSERT_STRING_EQUAL(get, value3);
}

void test_hashMap_containsKey(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * containsKey;
	char * key3 = NULL;
	char * value3 = "value3";

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	containsKey = strdup("key");
	CU_ASSERT_TRUE(hashMap_containsKey(map, containsKey));
	containsKey = strdup("key2");
	CU_ASSERT_TRUE(hashMap_containsKey(map, containsKey));
	containsKey = strdup("notExisting");
	CU_ASSERT_FALSE(hashMap_containsKey(map, containsKey));
	containsKey = NULL;
	CU_ASSERT_FALSE(hashMap_containsKey(map, containsKey));

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	containsKey = NULL;
	CU_ASSERT_TRUE(hashMap_containsKey(map, containsKey));
}

void test_hashMap_getEntry(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * getEntryKey;
	char * key3 = NULL;
	char * value3 = "value3";	
	HASH_MAP_ENTRY entry;
	
	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	// Get with new created key
	getEntryKey = strdup("key");
	entry = hashMap_getEntry(map, getEntryKey);
	CU_ASSERT_PTR_NOT_NULL_FATAL(entry);
	CU_ASSERT_STRING_EQUAL(entry->key, key);
	CU_ASSERT_STRING_EQUAL(entry->value, value);

	getEntryKey = strdup("key2");
	entry = hashMap_getEntry(map, getEntryKey);
	CU_ASSERT_PTR_NOT_NULL(entry);
	CU_ASSERT_STRING_EQUAL(entry->key, key2);
	CU_ASSERT_STRING_EQUAL(entry->value, value2);

	getEntryKey = strdup("notExisting");
	entry = hashMap_getEntry(map, getEntryKey);
	CU_ASSERT_EQUAL(entry, NULL);

	getEntryKey = NULL;
	entry = hashMap_getEntry(map, getEntryKey);
	CU_ASSERT_EQUAL(entry, NULL);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	getEntryKey = NULL;
	entry = hashMap_getEntry(map, getEntryKey);
	CU_ASSERT_EQUAL(entry->key, key3);
	CU_ASSERT_STRING_EQUAL(entry->value, value3);
}

void test_hashMap_put(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * getKey;
	char * get;
	char * nkey2 = strdup("key2");
	char * nvalue2 = "value3";
	char * old;
	char * key3 = NULL;
	char * value3 = "value3";
	char * key4 = "key4";
	char * value4 = NULL;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	// Get with new key
	getKey = strdup("key");
	get = hashMap_get(map, getKey);
	CU_ASSERT_STRING_EQUAL(get, value);

	getKey = strdup("key2");
	get = hashMap_get(map, getKey);
	CU_ASSERT_STRING_EQUAL(get, value2);

	// Overwrite existing entry
	old = (char *) hashMap_put(map, nkey2, nvalue2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(old);
	CU_ASSERT_STRING_EQUAL(old, value2)

	getKey = strdup("key2");
	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, nvalue2);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	getKey = NULL;
	get = hashMap_get(map, key3);
	CU_ASSERT_STRING_EQUAL(get, value3);

	// Add fourth entry with NULL value
	hashMap_put(map, key4, value4);

	getKey = strdup("key4");
	get = hashMap_get(map, key4);
	CU_ASSERT_EQUAL(get, value4);
}

void test_hashMap_remove(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = NULL;
	char * value2 = "value2";
	char * removeKey;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry with null key
	hashMap_put(map, key2, value2);

	// Remove unexisting entry for map
	removeKey = strdup("unexisting");
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 2);
	CU_ASSERT_FALSE(hashMap_isEmpty(map));

	// Remove entry with new key
	removeKey = strdup("key");
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 1);

	removeKey = NULL;
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));

	// Remove unexisting entry for empty map
	removeKey = strdup("unexisting");
	hashMap_remove(map, removeKey);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));
}

void test_hashMap_containsValue(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * containsValue;
	char * key3 = "key3";
	char * value3 = NULL;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	containsValue = strdup("value");
	CU_ASSERT_TRUE(hashMap_containsValue(map, containsValue));
	containsValue = strdup("value2");
	CU_ASSERT_TRUE(hashMap_containsValue(map, containsValue));
	containsValue = strdup("notExisting");
	CU_ASSERT_FALSE(hashMap_containsValue(map, containsValue));
	containsValue = NULL;
	CU_ASSERT_FALSE(hashMap_containsValue(map, containsValue));

	// Add third entry with NULL value
	hashMap_put(map, key3, value3);

	containsValue = NULL;
	CU_ASSERT_TRUE(hashMap_containsValue(map, containsValue));
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
	if (NULL == CU_add_test(pSuite, "Map Get Test", test_hashMap_get)
		|| NULL == CU_add_test(pSuite, "Map Contains Key Test", test_hashMap_containsKey)
		|| NULL == CU_add_test(pSuite, "Map Get Entry Test", test_hashMap_getEntry)
		|| NULL == CU_add_test(pSuite, "Map Put Test", test_hashMap_put)
		|| NULL == CU_add_test(pSuite, "Map Remove Test", test_hashMap_remove)
		|| NULL == CU_add_test(pSuite, "Map Contains Value Test", test_hashMap_containsValue)


	)
	{
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	CU_set_output_filename(argv[1]);
	CU_list_tests_to_file();
	CU_automated_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
