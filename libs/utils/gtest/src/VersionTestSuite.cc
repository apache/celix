/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>

#include "celix_errno.h"
#include "celix_version.h"
#include "celix_utils.h"
#include "version.h"

extern "C"
{
#include "version_private.h"
}

class VersionTestSuite : public ::testing::Test {};

TEST_F(VersionTestSuite, create) {
    celix_version_t* version = nullptr;
    char * str;

    str = celix_utils_strdup("abc");
    EXPECT_EQ(CELIX_SUCCESS, version_createVersion(1, 2, 3, str, &version));
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);
    EXPECT_STREQ("abc", version->qualifier);

    version_destroy(version);
    version = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, version_createVersion(1, 2, 3, nullptr, &version));
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);
    EXPECT_STREQ("", version->qualifier);

    version_destroy(version);
    version = nullptr;
    free(str);
    str = celix_utils_strdup("abc");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersion(-1, -2, -3, str, &version));

    version_destroy(version);
    version = nullptr;
    free(str);
    str = celix_utils_strdup("abc|xyz");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersion(1, 2, 3, str, &version));

    version_destroy(version);
    free(str);
}

TEST_F(VersionTestSuite, clone) {
    celix_version_t* version = nullptr;
    celix_version_t* clone = nullptr;
    char * str;

    str = celix_utils_strdup("abc");
    EXPECT_EQ(CELIX_SUCCESS, version_createVersion(1, 2, 3, str, &version));
    EXPECT_EQ(CELIX_SUCCESS, version_clone(version, &clone));
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, clone->major);
    EXPECT_EQ(2, clone->minor);
    EXPECT_EQ(3, clone->micro);
    EXPECT_STREQ("abc", clone->qualifier);

    version_destroy(clone);
    version_destroy(version);
    free(str);
}

TEST_F(VersionTestSuite, createFromString) {
    celix_version_t* version = nullptr;
    celix_status_t status = CELIX_SUCCESS;
    char * str;

    str = celix_utils_strdup("1");
    EXPECT_EQ(CELIX_SUCCESS, version_createVersionFromString(str, &version));
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);

    version_destroy(version);

    free(str);
    str = celix_utils_strdup("a");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

    free(str);
    str = celix_utils_strdup("1.a");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

    free(str);
    str = celix_utils_strdup("1.1.a");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

    free(str);
    str = celix_utils_strdup("-1");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString(str, &version));

    free(str);
    str = celix_utils_strdup("1.2");
    version = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, version_createVersionFromString(str, &version));
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);

    version_destroy(version);

    free(str);
    str = celix_utils_strdup("1.2.3");
    version = nullptr;
    status = version_createVersionFromString(str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);

    version_destroy(version);
    free(str);
    str = celix_utils_strdup("1.2.3.abc");
    version = nullptr;
    status = version_createVersionFromString(str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);
    EXPECT_STREQ("abc", version->qualifier);

    version_destroy(version);
    free(str);
    str = celix_utils_strdup("1.2.3.abc_xyz");
    version = nullptr;
    status = version_createVersionFromString(str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);
    EXPECT_STREQ("abc_xyz", version->qualifier);

    version_destroy(version);
    free(str);
    str = celix_utils_strdup("1.2.3.abc-xyz");
    version = nullptr;
    status = version_createVersionFromString(str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(1, version->major);
    EXPECT_EQ(2, version->minor);
    EXPECT_EQ(3, version->micro);
    EXPECT_STREQ("abc-xyz", version->qualifier);

    version_destroy(version);
    free(str);
    str = celix_utils_strdup("1.2.3.abc|xyz");
    status = version_createVersionFromString(str, &version);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    free(str);
}

TEST_F(VersionTestSuite, createEmptyVersion) {
    celix_version_t* version = nullptr;
    celix_status_t status = CELIX_SUCCESS;

    status = version_createEmptyVersion(&version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    EXPECT_EQ(0, version->major);
    EXPECT_EQ(0, version->minor);
    EXPECT_EQ(0, version->micro);
    EXPECT_STREQ("", version->qualifier);

    version_destroy(version);
}

TEST_F(VersionTestSuite, getters) {
    celix_version_t* version = nullptr;
    celix_status_t status = CELIX_SUCCESS;
    char * str;
    int major, minor, micro;
    const char *qualifier;

    str = celix_utils_strdup("abc");
    status = version_createVersion(1, 2, 3, str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);

    version_getMajor(version, &major);
    EXPECT_EQ(1, major);

    version_getMinor(version, &minor);
    EXPECT_EQ(2, minor);

    version_getMicro(version, &micro);
    EXPECT_EQ(3, micro);

    version_getQualifier(version, &qualifier);
    EXPECT_STREQ("abc", qualifier);

    version_destroy(version);
    free(str);
}

TEST_F(VersionTestSuite, compare) {
    celix_version_t* version = nullptr;
    celix_version_t* compare = nullptr;
    celix_status_t status = CELIX_SUCCESS;
    char * str;
    int result;

    // Base version to compare
    str = celix_utils_strdup("abc");
    status = version_createVersion(1, 2, 3, str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);

    // Compare equality
    free(str);
    str = celix_utils_strdup("abc");
    compare = nullptr;
    status = version_createVersion(1, 2, 3, str, &compare);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    status = version_compareTo(version, compare, &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(0, result);

    // Compare against a higher version
    free(str);
    str = celix_utils_strdup("bcd");
    version_destroy(compare);
    compare = nullptr;
    status = version_createVersion(1, 2, 3, str, &compare);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    status = version_compareTo(version, compare, &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(result < 0);

    // Compare againts a lower version
    free(str);
    str = celix_utils_strdup("abc");
    version_destroy(compare);
    compare = nullptr;
    status = version_createVersion(1, 1, 3, str, &compare);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);
    status = version_compareTo(version, compare, &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(result > 0);

    version_destroy(compare);
    version_destroy(version);
    free(str);
}

TEST_F(VersionTestSuite, celix_version_compareToMajorMinor) {
    celix_version_t *version1 = celix_version_createVersion(2, 2, 0, nullptr);
    celix_version_t *version2 = celix_version_createVersion(2, 2, 4, "qualifier");

    EXPECT_EQ(0, celix_version_compareToMajorMinor(version1, 2, 2));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(version2, 2, 2));

    EXPECT_TRUE(celix_version_compareToMajorMinor(version1, 2, 3) < 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version2, 2, 3) < 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version1, 3, 3) < 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version2, 3, 3) < 0);


    EXPECT_TRUE(celix_version_compareToMajorMinor(version1, 2, 1) > 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version2, 2, 1) > 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version1, 1, 1) > 0);
    EXPECT_TRUE(celix_version_compareToMajorMinor(version2, 1, 1) > 0);

    celix_version_destroy(version1);
    celix_version_destroy(version2);
}

TEST_F(VersionTestSuite, toString) {
    celix_version_t* version = nullptr;
    celix_status_t status = CELIX_SUCCESS;
    char * str;
    char *result = nullptr;

    str = celix_utils_strdup("abc");
    status = version_createVersion(1, 2, 3, str, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);

    status = version_toString(version, &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(result != nullptr);
    EXPECT_STREQ("1.2.3.abc", result);
    free(result);

    version_destroy(version);
    version = nullptr;
    status = version_createVersion(1, 2, 3, nullptr, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(version != nullptr);

    status = version_toString(version, &result);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(result != nullptr);
    EXPECT_STREQ("1.2.3", result);

    version_destroy(version);
    free(result);
    free(str);
}

TEST_F(VersionTestSuite,semanticCompatibility) {
    celix_version_t* provider = nullptr;
    celix_version_t* compatible_user = nullptr;
    celix_version_t* incompatible_user_by_major = nullptr;
    celix_version_t* incompatible_user_by_minor = nullptr;
    celix_status_t status = CELIX_SUCCESS;
    bool isCompatible = false;

    status = version_isCompatible(compatible_user, provider, &isCompatible);
    EXPECT_EQ(CELIX_SUCCESS, status);

    version_createVersion(2, 3, 5, nullptr, &provider);
    version_createVersion(2, 1, 9, nullptr, &compatible_user);
    version_createVersion(1, 3, 5, nullptr, &incompatible_user_by_major);
    version_createVersion(2, 5, 7, nullptr, &incompatible_user_by_minor);

    status = version_isCompatible(compatible_user, provider, &isCompatible);
    EXPECT_TRUE(isCompatible == true);
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = version_isCompatible(incompatible_user_by_major, provider, &isCompatible);
    EXPECT_TRUE(isCompatible == false);
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = version_isCompatible(incompatible_user_by_minor, provider, &isCompatible);
    EXPECT_TRUE(isCompatible == false);
    EXPECT_EQ(CELIX_SUCCESS, status);

    version_destroy(provider);
    version_destroy(compatible_user);
    version_destroy(incompatible_user_by_major);
    version_destroy(incompatible_user_by_minor);
}

TEST_F(VersionTestSuite, compareEmptyAndNullQualifier) {
    //nullptr or "" qualifier should be the same
    auto* v1 = celix_version_createVersion(0, 0, 0, nullptr);
    auto* v2 = celix_version_createVersion(0, 0, 0, "");
    EXPECT_EQ(0, celix_version_compareTo(v1, v1));
    EXPECT_EQ(0, celix_version_compareTo(v1, v2));
    EXPECT_EQ(0, celix_version_compareTo(v2, v2));

    celix_version_destroy(v1);
    celix_version_destroy(v2);
}

TEST_F(VersionTestSuite, fillString) {
    // Create a version object
    celix_version_t* version = celix_version_createVersion(1, 2, 3, "alpha");

    // Test with buffer large enough to hold the formatted string
    char buffer[32];
    bool success = celix_version_fillString(version, buffer, 32);
    EXPECT_TRUE(success);
    EXPECT_STREQ("1.2.3.alpha", buffer);

    // Test with buffer too small to hold the formatted string
    success = celix_version_fillString(version, buffer, 5);
    EXPECT_FALSE(success);

    celix_version_destroy(version);
}