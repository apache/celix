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

class VersionTestSuite : public ::testing::Test {
public:
    void expectVersion(const celix_version_t* version, int major, int minor, int micro, const char* qualifier = "") {
        if (version) {
            EXPECT_EQ(major, celix_version_getMajor(version));
            EXPECT_EQ(minor, celix_version_getMinor(version));
            EXPECT_EQ(micro, celix_version_getMicro(version));
            EXPECT_STREQ(qualifier, celix_version_getQualifier(version));
        }
    }
};

TEST_F(VersionTestSuite, CreateTest) {
    celix_version_t* version = nullptr;

    version = celix_version_create(1, 2, 3, "abc");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc");
    celix_version_destroy(version);

    EXPECT_EQ(nullptr, celix_version_create(-1, -2, -3, "abc"));
    EXPECT_EQ(nullptr, celix_version_create(-1, -2, -3, "abc|xyz"));


    //Testing deprecated api
    version = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, version_createVersion(1, 2, 3, nullptr, &version));
    EXPECT_TRUE(version != nullptr);
    int major;
    int minor;
    int micro;
    const char* q;
    EXPECT_EQ(CELIX_SUCCESS, version_getMajor(version, &major));
    EXPECT_EQ(CELIX_SUCCESS, version_getMinor(version, &minor));
    EXPECT_EQ(CELIX_SUCCESS, version_getMicro(version, &micro));
    EXPECT_EQ(CELIX_SUCCESS, version_getQualifier(version, &q));
    EXPECT_EQ(1, major);
    EXPECT_EQ(2, minor);
    EXPECT_EQ(3, micro);
    EXPECT_STREQ("", q);
    version_destroy(version);

    version = nullptr;
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersion(-1, -2, -3, "abc", &version));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersion(1, 2, 3, "abc|xyz", &version));
}

TEST_F(VersionTestSuite, CopyTest) {
    auto* version = celix_version_create(1, 2, 3, "abc");
    auto* copy = celix_version_copy(version);
    EXPECT_NE(nullptr, version);
    EXPECT_NE(nullptr, copy);
    expectVersion(version, 1, 2, 3, "abc");
    celix_version_destroy(copy);
    celix_version_destroy(version);

    copy = celix_version_copy(nullptr); //returns "empty" version
    EXPECT_NE(nullptr, copy);
    expectVersion(copy, 0, 0, 0, "");
    celix_version_destroy(copy);
}

TEST_F(VersionTestSuite, CreateFromStringTest) {
    auto* version = celix_version_createVersionFromString("1");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 0, 0);
    version_destroy(version);

    EXPECT_EQ(nullptr, celix_version_createVersionFromString("a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.1.a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("-1"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.2.3.abc|xyz"));

    version = celix_version_createVersionFromString("1.2");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 0);
    version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3);
    version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3.abc");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc");
    version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3.abc_xyz");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc_xyz");
    version_destroy(version);


    version = celix_version_createVersionFromString("1.2.3.abc-xyz");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc-xyz");
    version_destroy(version);


    //Testing deprecated api
    version = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, version_createVersionFromString("1", &version));
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 0, 0);
    version_destroy(version);

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, version_createVersionFromString("a", &version));
}

TEST_F(VersionTestSuite, CreateEmptyVersionTest) {
    auto* version = celix_version_createEmptyVersion();
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 0, 0, 0, "");
    version_destroy(version);

    //Testing deprecated api
    version = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, version_createEmptyVersion(&version));
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 0, 0, 0, "");
    version_destroy(version);
}

TEST_F(VersionTestSuite, CompareTest) {
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

    // Compare against a lower version
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

TEST_F(VersionTestSuite, CompareToMajorMinorTest) {
    auto* version1 = celix_version_create(2, 2, 0, nullptr);
    auto* version2 = celix_version_create(2, 2, 4, "qualifier");

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

TEST_F(VersionTestSuite, ToStringTest) {
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

TEST_F(VersionTestSuite, SemanticCompatibilityTest) {
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

TEST_F(VersionTestSuite, CompareEmptyAndNullQualifierTest) {
    //nullptr or "" qualifier should be the same
    auto* v1 = celix_version_create(0, 0, 0, nullptr);
    auto* v2 = celix_version_create(0, 0, 0, "");
    EXPECT_EQ(0, celix_version_compareTo(v1, v1));
    EXPECT_EQ(0, celix_version_compareTo(v1, v2));
    EXPECT_EQ(0, celix_version_compareTo(v2, v2));

    celix_version_destroy(v1);
    celix_version_destroy(v2);
}

TEST_F(VersionTestSuite, FillStringTest) {
    // Create a version object
    auto* version = celix_version_create(1, 2, 3, "alpha");

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
