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
 * version_test.cpp
 *
 *  \date       Dec 18, 2012
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <apr_strings.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "version_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(version) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		logger = (framework_logger_pt) apr_palloc(pool, sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		apr_pool_destroy(pool);
	}
};


TEST(version, create) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;

	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc", version->qualifier);

	version = NULL;
	status = version_createVersion(1, 2, 3, NULL, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("", version->qualifier);

	str = "abc";
	status = version_createVersion(1, -2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "abc|xyz";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(version, clone) {
	version_pt version = NULL, clone = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;

	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	status = version_clone(version, &clone);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, clone->major);
	LONGS_EQUAL(2, clone->minor);
	LONGS_EQUAL(3, clone->micro);
	STRCMP_EQUAL("abc", clone->qualifier);
}

TEST(version, createFromString) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;

	str = "1";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);

	str = "a";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "1.a";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "1.1.a";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "-1";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = "1.2";
	version = NULL;
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);

	str = "1.2.3";
	version = NULL;
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);

	str = "1.2.3.abc";
	version = NULL;
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc", version->qualifier);

	str = "1.2.3.abc_xyz";
	version = NULL;
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc_xyz", version->qualifier);

	str = "1.2.3.abc-xyz";
	version = NULL;
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc-xyz", version->qualifier);

	str = "1.2.3.abc|xyz";
	status = version_createVersionFromString(apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(version, createEmptyVersion) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;

	status = version_createEmptyVersion(&version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(0, version->major);
	LONGS_EQUAL(0, version->minor);
	LONGS_EQUAL(0, version->micro);
	STRCMP_EQUAL("", version->qualifier);
}

TEST(version, getters) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;
	int major, minor, micro;
	char *qualifier;

	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	version_getMajor(version, &major);
    LONGS_EQUAL(1, major);

    version_getMinor(version, &minor);
	LONGS_EQUAL(2, minor);

	version_getMicro(version, &micro);
	LONGS_EQUAL(3, micro);

	version_getQualifier(version, &qualifier);
	STRCMP_EQUAL("abc", qualifier);
}

TEST(version, compare) {
	version_pt version = NULL, compare = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;
	int result;

	// Base version to compare
	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	// Compare equality
	str = "abc";
	compare = NULL;
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(0, result);

	// Compare against a higher version
	str = "bcd";
	compare = NULL;
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(result < 0);

	// Compare againts a lower version
	str = "abc";
	compare = NULL;
	status = version_createVersion(1, 1, 3, apr_pstrdup(pool, (const char *) str.c_str()), &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(result > 0);
}

TEST(version, toString) {
	version_pt version = NULL, compare = NULL;
	celix_status_t status = CELIX_SUCCESS;
	std::string str;
	char *result = NULL;

	str = "abc";
	status = version_createVersion(1, 2, 3, apr_pstrdup(pool, (const char *) str.c_str()), &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	status = version_toString(version, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(result != NULL);
	STRCMP_EQUAL("1.2.3.abc", result);

	version = NULL;
	status = version_createVersion(1, 2, 3, NULL, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	status = version_toString(version, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(result != NULL);
	STRCMP_EQUAL("1.2.3", result);
}



