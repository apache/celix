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
 * netstring_test.c
 *
 *  \date       Sep 13, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Automated.h>

#include <apr_general.h>

#include "celixbool.h"
#include "hash_map.h"
#include "utils.h"
#include "netstring.h"



apr_pool_t *pool;

int setup(void)
{
	apr_initialize();
	apr_pool_create(&pool, NULL);
	return 0;
}


void test_netstring_encode(void)
{
	celix_status_t status;
	char* inStr = "hello";
	char* outNetStr = NULL;

	status = netstring_encode(pool, inStr, &outNetStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("5:hello,", outNetStr);
}



void test_netstring_decode(void)
{
	celix_status_t status;
	char* inNetStr = "5:hello,";
	char* outStr = NULL;

	status = netstring_decode(pool, inNetStr, &outStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("hello", outStr);

	inNetStr = "26:celixnowsupportsnetstrings,33:netstringsareavailablewithincelix,";

	status = netstring_decode(pool, inNetStr, &outStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("celixnowsupportsnetstrings", outStr);


}



void test_netstring_decodetoArray(void)
{
	celix_status_t status;
	char* inNetStr = "5:hello,3:tom,";
	char** outStrArr = NULL;
	int size;

	status = netstring_decodetoArray(pool, inNetStr, &outStrArr, &size);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_EQUAL(size, 2);
	CU_ASSERT_STRING_EQUAL("hello", outStrArr[0]);
	CU_ASSERT_STRING_EQUAL("tom", outStrArr[1]);

	inNetStr = "26:celixnowsupportsnetstrings,33:netstringsareavailablewithincelix,";

	status = netstring_decodetoArray(pool, inNetStr, &outStrArr, &size);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_EQUAL(size, 2);

	CU_ASSERT_STRING_EQUAL("celixnowsupportsnetstrings", outStrArr[0]);
	CU_ASSERT_STRING_EQUAL("netstringsareavailablewithincelix", outStrArr[1]);
}


void test_netstring_encodeFromArray(void)
{
	celix_status_t status;
	char *inStrArr[4];
	char *outStr;

	inStrArr[0] = "celix";
	inStrArr[1] = "-";
	inStrArr[2] = "osgi";
	inStrArr[3] = "in c";

	status = netstring_encodeFromArray(pool, inStrArr, 4, &outStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("5:celix,1:-,4:osgi,4:in c,", outStr);
}



void test_netstring_decodetoHashMap(void)
{
	celix_status_t status;
	char* inNetStr = "31:(key:helloKey=value:helloValue),27:(key:tomKey=value:tomValue),";
	hash_map_pt outHashMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

	status = netstring_decodeToHashMap(pool, inNetStr, outHashMap);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_EQUAL(hashMap_size(outHashMap), 2);

	CU_ASSERT_STRING_EQUAL(hashMap_get(outHashMap, "helloKey"), "helloValue");
	CU_ASSERT_STRING_EQUAL(hashMap_get(outHashMap, "tomKey"), "tomValue");

	hashMap_destroy(outHashMap, false, false);
}



void test_netstring_encodeFromHashMap(void)
{
	celix_status_t status;
	hash_map_pt inHashMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
	char *outNetStr;

	hashMap_put(inHashMap, "helloKey", "helloValue");
	hashMap_put(inHashMap, "tomKey", "tomValue");

	status = netstring_encodeFromHashMap(pool, inHashMap, &outNetStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("31:(key:helloKey=value:helloValue),27:(key:tomKey=value:tomValue),", outNetStr);

	hashMap_destroy(inHashMap, false, false);
}


void test_netstring_decodetoArrayList(void)
{
	celix_status_t status;
	char* inNetStr = "11:(misschien),11:(schrijven),";
	array_list_pt outArrayList = NULL;

	status = arrayList_create(&outArrayList);
	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);

	status = netstring_decodeToArrayList(pool, inNetStr, outArrayList);

	CU_ASSERT_EQUAL(arrayList_size(outArrayList), 2);

	CU_ASSERT_STRING_EQUAL(arrayList_get(outArrayList, 0), "misschien");
	CU_ASSERT_STRING_EQUAL(arrayList_get(outArrayList, 1), "schrijven");

	arrayList_destroy(outArrayList);
}



void test_netstring_encodeFromArrayList(void)
{
	celix_status_t status;
	array_list_pt inArrayList = NULL;
	char *outNetStr;

	status = arrayList_create(&inArrayList);
	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);

	CU_ASSERT_TRUE(arrayList_add(inArrayList, "heel"));
	CU_ASSERT_TRUE(arrayList_add(inArrayList, "bedankt"));

	status = netstring_encodeFromArrayList(pool, inArrayList, &outNetStr);

	CU_ASSERT_EQUAL(CELIX_SUCCESS, status);
	CU_ASSERT_STRING_EQUAL("6:(heel),9:(bedankt),", outNetStr);

	arrayList_destroy(inArrayList);
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
	if (NULL == CU_add_test(pSuite, "netstring encode test", test_netstring_encode)
		|| NULL == CU_add_test(pSuite, "netstring decode test", test_netstring_decode)
		|| NULL == CU_add_test(pSuite, "netstring decodeToArray test", test_netstring_decodetoArray)
		|| NULL == CU_add_test(pSuite, "netstring encodeFromArray test", test_netstring_encodeFromArray)
		|| NULL == CU_add_test(pSuite, "netstring decodeToHashMap test", test_netstring_decodetoHashMap)
		|| NULL == CU_add_test(pSuite, "netstring encodeFromHashMap test", test_netstring_encodeFromHashMap)
		|| NULL == CU_add_test(pSuite, "netstring decodeToArrayList test", test_netstring_decodetoArrayList)
		|| NULL == CU_add_test(pSuite, "netstring encodeFromArrayList test", test_netstring_encodeFromArrayList)

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

