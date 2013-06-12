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
 * test_hessian_out.c
 *
 *  \date       Aug 4, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <Automated.h>

#include "hessian_2.0_out.h"

hessian_out_pt out;

int setup() {
	out = malloc(sizeof(*out));

	return 0;
}

void test_hessian_writeBoolean() {
	out = malloc(sizeof(*out));
	out->offset = 0;
	out->buffer = NULL;
	out->capacity = 0;
	out->chunkLength = 0;
	out->lastChunk = false;
	out->length = 0;

	hessian_writeBoolean(out, true);

	CU_ASSERT_EQUAL(out->length, 1);
	CU_ASSERT_EQUAL(out->buffer[0], 'T');
}

void test_hessian_writeInt() {
	out = malloc(sizeof(*out));
	out->offset = 0;
	out->buffer = NULL;
	out->capacity = 0;
	out->chunkLength = 0;
	out->lastChunk = false;
	out->length = 0;

	hessian_writeInt(out, 0);

	unsigned char c1 = out->buffer[0];
	unsigned char expect = 0x90;
	CU_ASSERT_EQUAL(out->length, 1);
	CU_ASSERT_EQUAL(c1, expect);

	hessian_writeInt(out, -256);

	unsigned char c2[] =  { out->buffer[1], out->buffer[2] };
	unsigned char expect2[] = { 0xC7, 0x00 };
	CU_ASSERT_EQUAL(out->length, 3);
	// CU_ASSERT_EQUAL(c2, expect2);
}

int main (int argc, char** argv) {
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	  return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Hessian output", setup, NULL);
	if (NULL == pSuite) {
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* add the tests to the suite */
	if (
			NULL == CU_add_test(pSuite, "test boolean", test_hessian_writeBoolean)
			|| NULL == CU_add_test(pSuite, "test int", test_hessian_writeInt)
		) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_set_output_filename(argv[1]);
	CU_list_tests_to_file();
	CU_automated_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
