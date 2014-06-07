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
 * version_range_test.cpp
 *
 *  \date       Dec 18, 2012
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <apr_strings.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
	#include "CppUTestExt/MockSupport_c.h"

	#include "version_range_private.h"
	#include "version_private.h"

    #include "celix_log.h"

    framework_logger_pt logger;

	celix_status_t version_createEmptyVersion(version_pt *version) {
		mock_c()->actualCall("version_createEmptyVersion")
				->_andPointerOutputParameters("version", (void **) version);
		return CELIX_SUCCESS;
	}

	celix_status_t version_compareTo(version_pt version, version_pt compare, int *result) {
//		*result = (int) mock_c()->getData("result").value.intValue;
		mock_c()->actualCall("version_compareTo")
			->withPointerParameters("version", version)
			->withPointerParameters("compare", compare)
			->_andIntOutputParameters("result", result);
		return CELIX_SUCCESS;
	}

	celix_status_t version_createVersionFromString(char * versionStr, version_pt *version) {
		mock_c()->actualCall("version_createVersionFromString")
			->withStringParameters("versionStr", versionStr)
			->_andPointerOutputParameters("version", (void **) version);
		return CELIX_SUCCESS;
	}
}

int main(int argc, char** argv) {
	RUN_ALL_TESTS(argc, argv);
	return 0;
}

TEST_GROUP(version_range) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		logger = (framework_logger_pt) apr_palloc(pool, sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		apr_pool_destroy(pool);
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(version_range, create) {
	celix_status_t status = APR_SUCCESS;
	version_range_pt range = NULL;
	version_pt version = (version_pt) apr_palloc(pool, sizeof(*version));

	status = versionRange_createVersionRange(version, false, version, true, &range);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C((range != NULL));
	LONGS_EQUAL(true, range->isHighInclusive);
	LONGS_EQUAL(false, range->isLowInclusive);
	POINTERS_EQUAL(version, range->low);
	POINTERS_EQUAL(version, range->high);
}

TEST(version_range, createInfinite) {
	celix_status_t status = APR_SUCCESS;
	version_range_pt range = NULL;
	version_pt version = (version_pt) apr_palloc(pool, sizeof(*version));
	version->major = 1;
	version->minor = 2;
	version->micro = 3;

	mock()
		.expectOneCall("version_createEmptyVersion")
		.withParameter("pool", pool)
		.andOutputParameter("version", version);
	status = versionRange_createInfiniteVersionRange(&range);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(range != NULL);
	LONGS_EQUAL(true, range->isHighInclusive);
	LONGS_EQUAL(true, range->isLowInclusive);
	POINTERS_EQUAL(version, range->low);
	POINTERS_EQUAL(NULL, range->high);
}

TEST(version_range, isInRange) {
	celix_status_t status = APR_SUCCESS;
	version_range_pt range = NULL;
	version_pt version = (version_pt) apr_palloc(pool, sizeof(*version));
	version->major = 1;
	version->minor = 2;
	version->micro = 3;

	version_pt low = (version_pt) apr_palloc(pool, sizeof(*low));
	low->major = 1;
	low->minor = 2;
	low->micro = 3;

	version_pt high = (version_pt) apr_palloc(pool, sizeof(*high));
	high->major = 1;
	high->minor = 2;
	high->micro = 3;

	versionRange_createVersionRange(low, true, high, true, &range);

	mock()
		.expectOneCall("version_compareTo")
		.withParameter("version", version)
		.withParameter("compare", low)
		.andOutputParameter("result", 1);
	mock()
		.expectOneCall("version_compareTo")
		.withParameter("version", version)
		.withParameter("compare", high)
		.andOutputParameter("result", -1);

	bool result;
	status = versionRange_isInRange(range, version, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(true, result);
}

TEST(version_range, parse) {
	celix_status_t status = APR_SUCCESS;
	version_range_pt range = NULL;
	version_pt low = (version_pt) apr_palloc(pool, sizeof(*low));
	version_pt high = (version_pt) apr_palloc(pool, sizeof(*high));

	low->major = 1;
	low->minor = 2;
	low->micro = 3;

	high->major = 7;
	high->minor = 8;
	high->micro = 9;

	mock().strictOrder();
	mock()
		.expectOneCall("version_createVersionFromString")
		.withParameter("pool", pool)
		.withParameter("versionStr", "1.2.3")
		.andOutputParameter("version", low);
	mock()
		.expectOneCall("version_createVersionFromString")
		.withParameter("pool", pool)
		.withParameter("versionStr", "7.8.9")
		.andOutputParameter("version", high);

	std::string version = "[1.2.3, 7.8.9]";
	status = versionRange_parse((char *) version.c_str(), &range);
	LONGS_EQUAL(CELIX_SUCCESS, status);
}




