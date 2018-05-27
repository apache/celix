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
 * framework_test.c
 *
 *  \date       Jul 16, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>

#include "CUnit/Basic.h"
#include "framework_private.h"

void test_framework_create(void) {
	struct framework * framework;
	apr_pool_t *memoryPool;

	apr_status_t rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        CU_FAIL("Could not initialize APR");
        return;
    }

    apr_pool_create(&memoryPool, NULL);

	framework_create(&framework, memoryPool, NULL);


	CU_ASSERT(framework != NULL);
}

void frameworkTest_startFw() {
	apr_status_t rv = apr_initialize();
	if (rv != APR_SUCCESS) {
		CU_FAIL("Could not initialize APR");
	} else {
		apr_pool_t *memoryPool;
		apr_status_t s = apr_pool_create(&memoryPool, NULL);
		if (s != APR_SUCCESS) {
			CU_FAIL("Could not create memory pool");
		} else {
			framework_pt framework = NULL;
			if (framework_create(&framework, memoryPool, NULL) == CELIX_SUCCESS) {
				if (fw_init(framework) == CELIX_SUCCESS) {
					if (framework_start(framework) == CELIX_SUCCESS) {
						CU_PASS("Framework started");
						framework_stop(framework);
						framework_destroy(framework);
						apr_pool_destroy(memoryPool);
						apr_terminate();
					} else {
						CU_FAIL("Could not create memory pool");
					}
				} else {
					CU_FAIL("Could not create memory pool");
				}
			} else {
				CU_FAIL("Could not create memory pool");
			}
		}
	}
}

void frameworkTest_installBundle() {
	apr_status_t rv = apr_initialize();
	if (rv != APR_SUCCESS) {
		CU_FAIL("Could not initialize APR");
	} else {
		apr_pool_t *memoryPool;
		apr_status_t s = apr_pool_create(&memoryPool, NULL);
		if (s != APR_SUCCESS) {
			CU_FAIL("Could not create memory pool");
		} else {
			framework_pt framework = NULL;
			if (framework_create(&framework, memoryPool, NULL) == CELIX_SUCCESS) {
				if (fw_init(framework) == CELIX_SUCCESS) {
					if (framework_start(framework) == CELIX_SUCCESS) {

						// fw_installBundle(); // Needs a fake bundle..

						framework_stop(framework);
						framework_destroy(framework);
						apr_pool_destroy(memoryPool);
						apr_terminate();
					} else {
						CU_FAIL("Could not create memory pool");
					}
				} else {
					CU_FAIL("Could not create memory pool");
				}
			} else {
				CU_FAIL("Could not create memory pool");
			}
		}
	}
}


int main (int argc, char** argv) {
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	  return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Framework", NULL, NULL);
	if (NULL == pSuite) {
	  CU_cleanup_registry();
	  return CU_get_error();
	}

	/* add the tests to the suite */
	if (
		NULL == CU_add_test(pSuite, "Framework Creation Test", test_framework_create)
		|| NULL == CU_add_test(pSuite, "Start/Stop test", frameworkTest_startFw)
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
