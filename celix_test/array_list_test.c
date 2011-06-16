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
 * array_list_test.c
 *
 *  Created on: Aug 4, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdbool.h>

#include "CUnit/Basic.h"

#include "array_list.h"
#include "array_list_private.h"

ARRAY_LIST list;

int setup(void) {
	list = arrayList_create();
	if (list == NULL) {
		return 1;
	}
	return 0;
}

void test_arrayList_create(void) {
	CU_ASSERT_PTR_NOT_NULL_FATAL(list);
	CU_ASSERT_EQUAL(list->size, 0);
}

void test_arrayList_trimToSize(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	CU_ASSERT_EQUAL(list->size, 1);
	CU_ASSERT_EQUAL(list->capacity, 10);

	arrayList_trimToSize(list);
	CU_ASSERT_EQUAL(list->size, 1);
	CU_ASSERT_EQUAL(list->capacity, 1);
}

void test_arrayList_ensureCapacity(void) {
	list = arrayList_create();
	arrayList_clear(list);
	CU_ASSERT_EQUAL(list->capacity, 10);
	CU_ASSERT_EQUAL(list->size, 0);
	int i;
	for (i = 0; i < 100; i++) {
		arrayList_add(list, "entry");
	}
	CU_ASSERT_EQUAL(list->capacity, 133);
	CU_ASSERT_EQUAL(list->size, 100);
	list = arrayList_create();
}

void test_arrayList_size(void) {
	arrayList_clear(list);
	CU_ASSERT_EQUAL(list->size, 0);

	char * entry = "entry";
	arrayList_add(list, entry);
	CU_ASSERT_EQUAL(list->size, 1);

	char * entry2 = "entry";
	arrayList_add(list, entry2);
	CU_ASSERT_EQUAL(list->size, 2);

	char * entry3 = "entry";
	arrayList_add(list, entry3);
	CU_ASSERT_EQUAL(list->size, 3);
}

void test_arrayList_isEmpty(void) {
	arrayList_clear(list);
	CU_ASSERT_EQUAL(list->size, 0);
	CU_ASSERT_TRUE(arrayList_isEmpty(list));
}

void test_arrayList_contains(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);

	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	CU_ASSERT_TRUE(arrayList_contains(list, entry));
	CU_ASSERT_TRUE(arrayList_contains(list, entry2));
	bool contains = arrayList_contains(list, NULL);
	CU_ASSERT_FALSE(contains);

	char * entry3 = NULL;
	arrayList_add(list, entry3);

	CU_ASSERT_TRUE(arrayList_contains(list, entry3));
}

void test_arrayList_indexOf(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);

	char * entry2 = "entry2";
	arrayList_add(list, entry2);
	arrayList_add(list, entry2);
	arrayList_add(list, entry2);
	arrayList_add(list, entry2);

	CU_ASSERT_EQUAL(arrayList_indexOf(list, entry), 0);
	CU_ASSERT_EQUAL(arrayList_indexOf(list, entry2), 1);
	CU_ASSERT_EQUAL(arrayList_lastIndexOf(list, entry), 0);
	CU_ASSERT_EQUAL(arrayList_lastIndexOf(list, entry2), 4);
}

void test_arrayList_get(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	char * get = arrayList_get(list, 0);
	CU_ASSERT_EQUAL(entry, get);

	get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry2, get);

	char * entry3 = NULL;
	arrayList_add(list, entry3);

	get = arrayList_get(list, 2);
	CU_ASSERT_PTR_NULL(get);

	get = arrayList_get(list, 42);
	CU_ASSERT_PTR_NULL(get);
}

void test_arrayList_set(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	char * get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry2, get);

	char * entry3 = "entry3";
	char * old = arrayList_set(list, 1, entry3);
	CU_ASSERT_EQUAL(entry2, old);
	get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry3, get);
}

void test_arrayList_add(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	char * get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry2, get);

	char * entry3 = "entry3";
	arrayList_addIndex(list, 1, entry3);

	get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry3, get);

	get = arrayList_get(list, 2);
	CU_ASSERT_EQUAL(entry2, get);
}

void test_arrayList_addAll(void) {
    arrayList_clear(list);

    ARRAY_LIST toAdd = arrayList_create();
    char * entry = "entry";
    arrayList_add(toAdd, entry);
    char * entry2 = "entry2";
    arrayList_add(toAdd, entry2);

    char * entry3 = "entry3";
    arrayList_add(list, entry3);

    char * get = arrayList_get(list, 0);
    CU_ASSERT_EQUAL(entry3, get);

    bool changed = arrayList_addAll(list, toAdd);
    CU_ASSERT_TRUE(changed);
    CU_ASSERT_EQUAL(arrayList_size(list), 3);

    get = arrayList_get(list, 1);
    CU_ASSERT_EQUAL(entry, get);

    get = arrayList_get(list, 2);
    CU_ASSERT_EQUAL(entry2, get);
}

void test_arrayList_remove(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	char * get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry2, get);

	// Remove first entry
	char * removed = arrayList_remove(list, 0);
	CU_ASSERT_EQUAL(entry, removed);

	// Check the new first element
	get = arrayList_get(list, 0);
	CU_ASSERT_EQUAL(entry2, get);

	// Add a new element
	char * entry3 = "entry3";
	arrayList_add(list, entry3);

	get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry3, get);
}

void test_arrayList_removeElement(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	// Remove entry
	CU_ASSERT_TRUE(arrayList_removeElement(list, entry));

	// Check the new first element
	char * get = arrayList_get(list, 0);
	CU_ASSERT_EQUAL(entry2, get);

	// Add a new element
	char * entry3 = "entry3";
	arrayList_add(list, entry3);

	get = arrayList_get(list, 1);
	CU_ASSERT_EQUAL(entry3, get);
}

void test_arrayList_clear(void) {
	arrayList_clear(list);

	char * entry = "entry";
	arrayList_add(list, entry);
	char * entry2 = "entry2";
	arrayList_add(list, entry2);

	CU_ASSERT_EQUAL(arrayList_size(list), 2);
	arrayList_clear(list);
	CU_ASSERT_EQUAL(arrayList_size(list), 0);
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
	if (NULL == CU_add_test(pSuite, "Array List Creation Test", test_arrayList_create)
		|| NULL == CU_add_test(pSuite, "Array List Trim Test", test_arrayList_trimToSize)
		|| NULL == CU_add_test(pSuite, "Array List Capacity Test", test_arrayList_ensureCapacity)
		|| NULL == CU_add_test(pSuite, "Array List Size Test", test_arrayList_size)
		|| NULL == CU_add_test(pSuite, "Array List Is Empty Test", test_arrayList_isEmpty)
		|| NULL == CU_add_test(pSuite, "Array List Contains Test", test_arrayList_contains)
		|| NULL == CU_add_test(pSuite, "Array List Index Of Test", test_arrayList_indexOf)
		|| NULL == CU_add_test(pSuite, "Array List Get Test", test_arrayList_get)
		|| NULL == CU_add_test(pSuite, "Array List Set Test", test_arrayList_set)
		|| NULL == CU_add_test(pSuite, "Array List Add Test", test_arrayList_add)
		|| NULL == CU_add_test(pSuite, "Array List Remove Test", test_arrayList_remove)
		|| NULL == CU_add_test(pSuite, "Array List Remove Element Test", test_arrayList_removeElement)
		|| NULL == CU_add_test(pSuite, "Array List Clear Test", test_arrayList_clear)
		|| NULL == CU_add_test(pSuite, "Array List Add All", test_arrayList_addAll)
	)
	{
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_set_output_filename("ArrayList");
	CU_list_tests_to_file();
	CU_automated_run_tests();
//	CU_basic_set_mode(CU_BRM_VERBOSE);
//	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}

