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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
#include "version_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x666;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
	if(s==NULL){
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc (len + 1,sizeof(char));

	if (d == NULL){
		return NULL;
	}

	strncpy (d,s,len);
	return d;
}

TEST_GROUP(version) {

	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}

};


TEST(version, create) {
	version_pt version = NULL;
	char * str;

//	str = my_strdup("abc");
//	status = version_createVersion(1, 2, 3, str, &version);
//	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	str = my_strdup("abc");
	LONGS_EQUAL(CELIX_SUCCESS, version_createVersion(1, 2, 3, str, &version));
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc", version->qualifier);

	version_destroy(version);
	version = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, version_createVersion(1, 2, 3, NULL, &version));
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("", version->qualifier);

	version_destroy(version);
	version = NULL;
	free(str);
	str = my_strdup("abc");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersion(-1, -2, -3, str, &version));

	version_destroy(version);
	version = NULL;
	free(str);
	str = my_strdup("abc|xyz");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersion(1, 2, 3, str, &version));

	version_destroy(version);
	free(str);
}

TEST(version, clone) {
	version_pt version = NULL, clone = NULL;
	char * str;

	str = my_strdup("abc");
	LONGS_EQUAL(CELIX_SUCCESS, version_createVersion(1, 2, 3, str, &version));
	LONGS_EQUAL(CELIX_SUCCESS, version_clone(version, &clone));
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, clone->major);
	LONGS_EQUAL(2, clone->minor);
	LONGS_EQUAL(3, clone->micro);
	STRCMP_EQUAL("abc", clone->qualifier);

	version_destroy(clone);
	version_destroy(version);
	free(str);
}

TEST(version, createFromString) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	char * str;

	str = my_strdup("1");
	LONGS_EQUAL(CELIX_SUCCESS, version_createVersionFromString(str, &version));
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);

	version_destroy(version);

	free(str);
	str = my_strdup("a");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

	free(str);
	str = my_strdup("1.a");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

	free(str);
	str = my_strdup("1.1.a");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

	free(str);
	str = my_strdup("-1");
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

	free(str);
	str = my_strdup("1.2");
	version = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, version_createVersionFromString(str, &version));
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);

	version_destroy(version);

	free(str);
	str = my_strdup("1.2.3");
	version = NULL;
	status = version_createVersionFromString(str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);

	version_destroy(version);
	free(str);
	str = my_strdup("1.2.3.abc");
	version = NULL;
	status = version_createVersionFromString(str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc", version->qualifier);

	version_destroy(version);
	free(str);
	str = my_strdup("1.2.3.abc_xyz");
	version = NULL;
	status = version_createVersionFromString(str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc_xyz", version->qualifier);

	version_destroy(version);
	free(str);
	str = my_strdup("1.2.3.abc-xyz");
	version = NULL;
	status = version_createVersionFromString(str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
    LONGS_EQUAL(1, version->major);
	LONGS_EQUAL(2, version->minor);
	LONGS_EQUAL(3, version->micro);
	STRCMP_EQUAL("abc-xyz", version->qualifier);

	version_destroy(version);
	free(str);
	str = my_strdup("1.2.3.abc|xyz");
	status = version_createVersionFromString(str, &version);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(str);
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

	version_destroy(version);
}

TEST(version, getters) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	char * str;
	int major, minor, micro;
	char *qualifier;

	str = my_strdup("abc");
	status = version_createVersion(1, 2, 3, str, &version);
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

	version_destroy(version);
	free(str);
}

TEST(version, compare) {
	version_pt version = NULL, compare = NULL;
	celix_status_t status = CELIX_SUCCESS;
	char * str;
	int result;

	// Base version to compare
	str = my_strdup("abc");
	status = version_createVersion(1, 2, 3, str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	// Compare equality
	free(str);
	str = my_strdup("abc");
	compare = NULL;
	status = version_createVersion(1, 2, 3, str, &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(0, result);

	// Compare against a higher version
	free(str);
	str = my_strdup("bcd");
	version_destroy(compare);
	compare = NULL;
	status = version_createVersion(1, 2, 3, str, &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(result < 0);

	// Compare againts a lower version
	free(str);
	str = my_strdup("abc");
	version_destroy(compare);
	compare = NULL;
	status = version_createVersion(1, 1, 3, str, &compare);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);
	status = version_compareTo(version, compare, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(result > 0);

	version_destroy(compare);
	version_destroy(version);
	free(str);
}

TEST(version, toString) {
	version_pt version = NULL;
	celix_status_t status = CELIX_SUCCESS;
	char * str;
	char *result = NULL;

	str = my_strdup("abc");
	status = version_createVersion(1, 2, 3, str, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	status = version_toString(version, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(result != NULL);
	STRCMP_EQUAL("1.2.3.abc", result);
	free(result);

	version_destroy(version);
	version = NULL;
	status = version_createVersion(1, 2, 3, NULL, &version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(version != NULL);

	status = version_toString(version, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(result != NULL);
	STRCMP_EQUAL("1.2.3", result);

	version_destroy(version);
	free(result);
	free(str);
}

TEST(version,semanticCompatibility) {
    version_pt provider = NULL;
    version_pt compatible_user = NULL;
    version_pt incompatible_user_by_major = NULL;
    version_pt incompatible_user_by_minor = NULL;
    celix_status_t status = CELIX_SUCCESS;
    bool isCompatible = false;

    status = version_isCompatible(compatible_user, provider, &isCompatible);
    LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

    version_createVersion(2, 3, 5, NULL, &provider);
    version_createVersion(2, 1, 9, NULL, &compatible_user);
    version_createVersion(1, 3, 5, NULL, &incompatible_user_by_major);
    version_createVersion(2, 5, 7, NULL, &incompatible_user_by_minor);

    status = version_isCompatible(compatible_user, provider, &isCompatible);
    CHECK(isCompatible == true);
    LONGS_EQUAL(CELIX_SUCCESS, status);

    status = version_isCompatible(incompatible_user_by_major, provider, &isCompatible);
    CHECK(isCompatible == false);
    LONGS_EQUAL(CELIX_SUCCESS, status);

    status = version_isCompatible(incompatible_user_by_minor, provider, &isCompatible);
    CHECK(isCompatible == false);
    LONGS_EQUAL(CELIX_SUCCESS, status);

    version_destroy(provider);
    version_destroy(compatible_user);
    version_destroy(incompatible_user_by_major);
    version_destroy(incompatible_user_by_minor);
}

