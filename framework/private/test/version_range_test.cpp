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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"
#include "string.h"

extern "C"
{
#include "version_range_private.h"
#include "version_private.h"

#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s) {
	if (s == NULL) {
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc(len + 1, sizeof(char));

	if (d == NULL) {
		return NULL;
	}

	strncpy(d, s, len);
	return d;
}

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(version_range) {

	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(version_range, create) {
	celix_status_t status = CELIX_SUCCESS;
	version_range_pt range = NULL;
	version_pt version = (version_pt) malloc(sizeof(*version));

	status = versionRange_createVersionRange(version, false, version, true, &range);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C((range != NULL));
	LONGS_EQUAL(true, range->isHighInclusive);
	LONGS_EQUAL(false, range->isLowInclusive);
	POINTERS_EQUAL(version, range->low);
	POINTERS_EQUAL(version, range->high);

	mock().expectNCalls(2, "version_destroy")
			.withParameter("version", version);

	versionRange_destroy(range);
	free(version);
}

TEST(version_range, createInfinite) {
	celix_status_t status = CELIX_SUCCESS;
	version_range_pt range = NULL;
	version_pt version = (version_pt) malloc(sizeof(*version));
	version->major = 1;
	version->minor = 2;
	version->micro = 3;

	mock()
	.expectOneCall("version_createEmptyVersion")
	.withOutputParameterReturning("version", &version, sizeof("version"));
	status = versionRange_createInfiniteVersionRange(&range);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(range != NULL);
	LONGS_EQUAL(true, range->isHighInclusive);
	LONGS_EQUAL(true, range->isLowInclusive);
	POINTERS_EQUAL(version, range->low);
	POINTERS_EQUAL(NULL, range->high);

	mock().expectOneCall("version_destroy")
			.withParameter("version", version);

	versionRange_destroy(range);
	free(version);
}

TEST(version_range, isInRange) {
	bool result;
	version_range_pt range = NULL;
	version_pt version = (version_pt) malloc(sizeof(*version));
	version->major = 1;
	version->minor = 2;
	version->micro = 3;

	version_pt low = (version_pt) malloc(sizeof(*low));
	low->major = 1;
	low->minor = 2;
	low->micro = 3;

	version_pt high = (version_pt) malloc(sizeof(*high));
	high->major = 1;
	high->minor = 2;
	high->micro = 3;

	int stat = 1;
	mock()
	.expectNCalls(5, "version_compareTo")
	.withParameter("version", version)
	.withParameter("compare", low)
	.withOutputParameterReturning("result", &stat, sizeof(int));
	int stat2 = -1;
	mock()
	.expectNCalls(4, "version_compareTo")
	.withParameter("version", version)
	.withParameter("compare", high)
	.withOutputParameterReturning("result", &stat2, sizeof(int));

	versionRange_createVersionRange(low, true, high, true, &range);
	LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
	LONGS_EQUAL(true, result);

	mock().expectOneCall("version_destroy")
			.withParameter("version", low);
	mock().expectOneCall("version_destroy")
			.withParameter("version", high);
	versionRange_destroy(range);

	versionRange_createVersionRange(low, true, NULL, true, &range);
	LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
	LONGS_EQUAL(true, result);

	mock().expectOneCall("version_destroy")
			.withParameter("version", low);
	versionRange_destroy(range);

	versionRange_createVersionRange(low, false, high, true, &range);
	LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
	LONGS_EQUAL(true, result);

	mock().expectOneCall("version_destroy")
			.withParameter("version", low);
	mock().expectOneCall("version_destroy")
			.withParameter("version", high);
	versionRange_destroy(range);

	versionRange_createVersionRange(low, true, high, false, &range);
	LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
	LONGS_EQUAL(true, result);

	mock().expectOneCall("version_destroy")
			.withParameter("version", low);
	mock().expectOneCall("version_destroy")
			.withParameter("version", high);
	versionRange_destroy(range);

	versionRange_createVersionRange(low, false, high, false, &range);
	LONGS_EQUAL(CELIX_SUCCESS, versionRange_isInRange(range, version, &result));
	LONGS_EQUAL(true, result);

	mock().expectOneCall("version_destroy")
			.withParameter("version", low);
	mock().expectOneCall("version_destroy")
			.withParameter("version", high);
	versionRange_destroy(range);

	free(version);
	free(high);
	free(low);
}

TEST(version_range, parse) {
	version_range_pt range = NULL;
	version_pt low = (version_pt) malloc(sizeof(*low));
	version_pt high = (version_pt) malloc(sizeof(*high));
	char * version = my_strdup("[1.2.3,7.8.9]");
	low->major = 1;
	low->minor = 2;
	low->micro = 3;

	high->major = 7;
	high->minor = 8;
	high->micro = 9;

	mock().expectOneCall("version_destroy")
			.withParameter("version", high);

	mock().expectNCalls(2, "version_destroy")
			.withParameter("version", low);

	mock().expectOneCall("version_createVersionFromString")
		   .withParameter("versionStr", "1.2.3")
		   .withOutputParameterReturning("version", &low, sizeof(low));

	mock().expectOneCall("version_createVersionFromString")
			.withParameter("versionStr", "7.8.9")
			.withOutputParameterReturning("version", &high, sizeof(high));

	LONGS_EQUAL(CELIX_SUCCESS, versionRange_parse(version, &range));

	versionRange_destroy(range);
	free(version);
	version = my_strdup("[1.2.3");

	mock().expectOneCall("version_createVersionFromString")
			.withParameter("versionStr", "[1.2.3")
			.withOutputParameterReturning("version", &low, sizeof(low));

	LONGS_EQUAL(CELIX_SUCCESS, versionRange_parse(version, &range));

	versionRange_destroy(range);
	free(version);
	free(high);
	free(low);
}




