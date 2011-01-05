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
 * test.c
 *
 *  Created on: Jul 16, 2010
 *      Author: alexanderb
 */
#include <stdio.h>

#include "CUnit/Basic.h"
#include "linkedlist.h"

void test_linkedList_create(void) {
	LINKED_LIST list = linkedList_create();
	CU_ASSERT(list != NULL);
}

void test_linkedList_add(void) {
	LINKED_LIST list = linkedList_create();
	linkedList_addElement(list, "element");

	CU_ASSERT_EQUAL(linkedList_size(list), 2);
}

int main (int argc, char** argv) {
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	  return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_1", NULL, NULL);
	if (NULL == pSuite) {
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* add the tests to the suite */
	if (NULL == CU_add_test(pSuite, "List Creation Test", test_linkedList_create) ||
		NULL == CU_add_test(pSuite, "List Add Test", test_linkedList_add))
	{
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	//return CU_get_error();
	return -1;
}
