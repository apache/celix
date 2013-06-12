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
#include <Automated.h>
#include <stddef.h>

#include "celixbool.h"

#include "hash_map.h"
#include "hash_map_private.h"

hash_map_pt map;

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
	// This fails on windows due to dllimport providing a proxy for exported functions.
	CU_ASSERT_EQUAL(map->equalsKey, hashMap_equals);
	CU_ASSERT_EQUAL(map->equalsValue, hashMap_equals);
	CU_ASSERT_EQUAL(map->hashKey, hashMap_hashCode);
	CU_ASSERT_EQUAL(map->hashValue, hashMap_hashCode);
}

void test_hashMap_size(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * key3 = strdup("key2");
	char * value3 = "value3";

	CU_ASSERT_EQUAL(map->size, 0);

	// Add one entry
	hashMap_put(map, key, value);
	CU_ASSERT_EQUAL(map->size, 1);

	// Add second entry
	hashMap_put(map, key2, value2);
	CU_ASSERT_EQUAL(map->size, 2);

	// Add entry using the same key, this does not overwrite an existing entry
	hashMap_put(map, key3, value3);
	CU_ASSERT_EQUAL(map->size, 3);

	// Clear map
	hashMap_clear(map, false, false);
	CU_ASSERT_EQUAL(map->size, 0);
}

void test_hashMap_isEmpty(void) {
	char * key = "key";
	char * value = "value";

	hashMap_clear(map, false, false);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));

	// Add one entry
	hashMap_put(map, key, value);
	CU_ASSERT_EQUAL(map->size, 1);
	CU_ASSERT_FALSE(hashMap_isEmpty(map));

	// Remove entry
	hashMap_remove(map, key);
	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_TRUE(hashMap_isEmpty(map));
}

void test_hashMap_get(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * neKey = "notExisting";
	char * key3 = NULL;
	char * value3 = "value3";
    char * get;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	get = hashMap_get(map, key);
	CU_ASSERT_STRING_EQUAL(get, value);

	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, value2);

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
	char * neKey = "notExisting";
	char * key3 = NULL;
	char * value3 = "value3";

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	CU_ASSERT_TRUE(hashMap_containsKey(map, key));
	CU_ASSERT_TRUE(hashMap_containsKey(map, key2));
	CU_ASSERT_FALSE(hashMap_containsKey(map, neKey));
	CU_ASSERT_FALSE(hashMap_containsKey(map, NULL));

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	CU_ASSERT_TRUE(hashMap_containsKey(map, key3));
}

void test_hashMap_getEntry(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * neKey = "notExisting";
	char * key3 = NULL;
	char * value3 = "value3";
	hash_map_entry_pt entry;
	
	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);
	entry = hashMap_getEntry(map, key);
	CU_ASSERT_STRING_EQUAL(entry->key, key);
	CU_ASSERT_STRING_EQUAL(entry->value, value);

	entry = hashMap_getEntry(map, key2);
	CU_ASSERT_STRING_EQUAL(entry->key, key2);
	CU_ASSERT_STRING_EQUAL(entry->value, value2);

	entry = hashMap_getEntry(map, neKey);
	CU_ASSERT_EQUAL(entry, NULL);

	entry = hashMap_getEntry(map, NULL);
	CU_ASSERT_EQUAL(entry, NULL);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	entry = hashMap_getEntry(map, key3);
	CU_ASSERT_EQUAL(entry->key, key3);
	CU_ASSERT_STRING_EQUAL(entry->value, value3);
}

void test_hashMap_put(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * nkey2 = strdup("key2");
	char * nvalue2 = "value3";
	char * key3 = NULL;
	char * value3 = "value3";
	char * key4 = "key4";
	char * value4 = NULL;
	char * old;
	char * get;
	
	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	get = hashMap_get(map, key);
	CU_ASSERT_STRING_EQUAL(get, value);

	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, value2);

	// Try to add an entry with the same key, since no explicit hash function is used,
	//   this will not overwrite an existing entry.
	old = (char *) hashMap_put(map, nkey2, nvalue2);
	CU_ASSERT_PTR_NULL_FATAL(old);

	// Retrieving the values will return the correct values
	get = hashMap_get(map, key2);
	CU_ASSERT_STRING_EQUAL(get, value2);
	get = hashMap_get(map, nkey2);
    CU_ASSERT_STRING_EQUAL(get, nvalue2);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	get = hashMap_get(map, key3);
	CU_ASSERT_STRING_EQUAL(get, value3);

	// Add fourth entry with NULL value
	hashMap_put(map, key4, value4);

	get = hashMap_get(map, key4);
	CU_ASSERT_EQUAL(get, value4);
}

void test_hashMap_resize(void) {
	int i;
	char * k;
	char key[6];
	
	hashMap_clear(map, false, false);

	CU_ASSERT_EQUAL(map->size, 0);
	CU_ASSERT_EQUAL(map->tablelength, 16);
	CU_ASSERT_EQUAL(map->treshold, 12);
	for (i = 0; i < 12; i++) {
		char key[6];
		sprintf(key, "key%d", i);
		k = strdup(key);
		hashMap_put(map, k, k);
	}
	CU_ASSERT_EQUAL(map->size, 12);
	CU_ASSERT_EQUAL(map->tablelength, 16);
	CU_ASSERT_EQUAL(map->treshold, 12);

	sprintf(key, "key%d", i);
	hashMap_put(map, strdup(key), strdup(key));
	CU_ASSERT_EQUAL(map->size, 13);
	CU_ASSERT_EQUAL(map->tablelength, 32);
	CU_ASSERT_EQUAL(map->treshold, 24);
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
	removeKey = "unexisting";
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
	char * key = "key";
	char * value = "value";
	char * key2 = NULL;
	char * value2 = "value2";
	hash_map_entry_pt entry1;
	hash_map_entry_pt entry2;
	hash_map_entry_pt removed;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry with null key
	hashMap_put(map, key2, value2);

	entry1 = hashMap_getEntry(map, key);
	entry2 = hashMap_getEntry(map, key2);

	CU_ASSERT_PTR_NOT_NULL_FATAL(entry1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(entry2);

	removed = hashMap_removeMapping(map, entry1);
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
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * key3 = NULL;
	char * value3 = "value3";
	char * key4 = "key4";
	char * value4 = NULL;

	hashMap_clear(map, false, false);
	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	// Add third entry with NULL key
	hashMap_put(map, key3, value3);

	// Add fourth entry with NULL value
	hashMap_put(map, key4, value4);

	hashMap_clear(map, false, false);
	CU_ASSERT_EQUAL(map->size, 0);
}

void test_hashMap_containsValue(void) {
	char * key = "key";
	char * value = "value";
	char * key2 = "key2";
	char * value2 = "value2";
	char * neValue = "notExisting";
	char * key3 = "key3";
	char * value3 = NULL;

	hashMap_clear(map, false, false);

	// Add one entry
	hashMap_put(map, key, value);

	// Add second entry
	hashMap_put(map, key2, value2);

	CU_ASSERT_TRUE(hashMap_containsValue(map, value));
	CU_ASSERT_TRUE(hashMap_containsValue(map, value2));
	CU_ASSERT_FALSE(hashMap_containsValue(map, neValue));
	CU_ASSERT_FALSE(hashMap_containsValue(map, NULL));

	// Add third entry with NULL value
	hashMap_put(map, key3, value3);

	CU_ASSERT_TRUE(hashMap_containsValue(map, value3));
}

void test_hashMapValues_toArray(void) {
    char * key = "key";
    char * value = "value";
    char * key2 = "key2";
    char * value2 = "value2";
    char **array;
    int size;
	hash_map_values_pt values;

	hashMap_clear(map, false, false);

    // Add one entry
    hashMap_put(map, key, value);

    // Add second entry
    hashMap_put(map, key2, value2);

    values = hashMapValues_create(map);
    hashMapValues_toArray(values, (void*)&array, &size);
    CU_ASSERT_EQUAL(size, 2);
    CU_ASSERT_TRUE(hashMapValues_contains(values, array[0]));
    CU_ASSERT_TRUE(hashMapValues_contains(values, array[1]));
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
		|| NULL == CU_add_test(pSuite, "Map To Array Test", test_hashMapValues_toArray)


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
