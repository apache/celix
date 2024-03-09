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

#include "celix_version.h"

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
}

TEST_F(VersionTestSuite, CopyTest) {
    auto* version = celix_version_create(1, 2, 3, "abc");
    auto* copy = celix_version_copy(version);
    EXPECT_NE(nullptr, version);
    EXPECT_NE(nullptr, copy);
    expectVersion(version, 1, 2, 3, "abc");
    celix_version_destroy(copy);
    celix_version_destroy(version);

    copy = celix_version_copy(nullptr);
    EXPECT_EQ(nullptr, copy);
}

TEST_F(VersionTestSuite, CreateFromStringTest) {
    auto* version = celix_version_createVersionFromString("1");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 0, 0);
    celix_version_destroy(version);

    EXPECT_EQ(nullptr, celix_version_createVersionFromString("a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.1.a"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("-1"));
    EXPECT_EQ(nullptr, celix_version_createVersionFromString("1.2.3.abc|xyz"));

    version = celix_version_createVersionFromString("1.2");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 0);
    celix_version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3);
    celix_version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3.abc");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc");
    celix_version_destroy(version);

    version = celix_version_createVersionFromString("1.2.3.abc_xyz");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc_xyz");
    celix_version_destroy(version);


    version = celix_version_createVersionFromString("1.2.3.abc-xyz");
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 1, 2, 3, "abc-xyz");
    celix_version_destroy(version);
}

TEST_F(VersionTestSuite, CreateEmptyVersionTest) {
    auto* version = celix_version_createEmptyVersion();
    EXPECT_TRUE(version != nullptr);
    expectVersion(version, 0, 0, 0, "");
    celix_version_destroy(version);
}

TEST_F(VersionTestSuite, CompareTest) {
    // Base version to compare
    const char* str = "abc";
    celix_version_t* version = celix_version_create(1, 2, 3, str);
    EXPECT_TRUE(version != nullptr);

    // Compare equality
    celix_version_t* compare = celix_version_create(1, 2, 3, str);
    EXPECT_TRUE(compare != nullptr);
    int result = celix_version_compareTo(version, compare);
    EXPECT_EQ(0, result);
    celix_version_destroy(compare);

    // Compare against a higher version
    compare = celix_version_create(1, 2, 3, "bcd");
    EXPECT_TRUE(compare != nullptr);
    result = celix_version_compareTo(version, compare);
    EXPECT_TRUE(result < 0);
    celix_version_destroy(compare);

    // Compare against a lower version
    compare = celix_version_create(1, 1, 3, str);
    EXPECT_TRUE(compare != nullptr);
    result = celix_version_compareTo(version, compare);
    EXPECT_TRUE(result > 0);
    celix_version_destroy(compare);

    celix_version_destroy(version);
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
    celix_version_t* version = celix_version_create(1, 2, 3, "abc");
    EXPECT_TRUE(version != nullptr);
    char* result = celix_version_toString(version);
    EXPECT_TRUE(result != nullptr);
    EXPECT_STREQ("1.2.3.abc", result);
    free(result);
    celix_version_destroy(version);

    version = celix_version_create(1, 2, 3, nullptr);
    EXPECT_TRUE(version != nullptr);
    result = celix_version_toString(version);
    EXPECT_TRUE(result != nullptr);
    EXPECT_STREQ("1.2.3", result);
    celix_version_destroy(version);
    free(result);
}

TEST_F(VersionTestSuite, SemanticCompatibilityTest) {
    celix_version_t* provider = nullptr;
    celix_version_t* compatible_user = nullptr;
    bool isCompatible = celix_version_isCompatible(compatible_user, provider);
    EXPECT_TRUE(isCompatible);

    provider = celix_version_create(2, 3, 5, nullptr);
    EXPECT_FALSE(celix_version_isCompatible(compatible_user, provider));

    compatible_user = celix_version_create(2, 1, 9, nullptr);
    celix_version_t* incompatible_user_by_major = celix_version_create(1, 3, 5, nullptr);
    celix_version_t* incompatible_user_by_minor = celix_version_create(2, 5, 7, nullptr);

    isCompatible = celix_version_isCompatible(compatible_user, provider);
    EXPECT_TRUE(isCompatible);

    isCompatible = celix_version_isCompatible(incompatible_user_by_major, provider);
    EXPECT_FALSE(isCompatible);

    isCompatible = celix_version_isCompatible(incompatible_user_by_minor, provider);
    EXPECT_FALSE(isCompatible);

    celix_version_destroy(provider);
    celix_version_destroy(compatible_user);
    celix_version_destroy(incompatible_user_by_major);
    celix_version_destroy(incompatible_user_by_minor);
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

TEST_F(VersionTestSuite, ParseTest) {
    celix_version_t* result;
    celix_status_t parseStatus = celix_version_parse("1.2.3.alpha", &result);
    EXPECT_EQ(CELIX_SUCCESS, parseStatus);
    EXPECT_NE(nullptr, result);
    expectVersion(result, 1, 2, 3, "alpha");
    celix_version_destroy(result);

    parseStatus = celix_version_parse("1.2.3", &result);
    EXPECT_EQ(CELIX_SUCCESS, parseStatus);
    EXPECT_NE(nullptr, result);
    expectVersion(result, 1, 2, 3);
    celix_version_destroy(result);

    parseStatus = celix_version_parse("1.2", &result);
    EXPECT_EQ(CELIX_SUCCESS, parseStatus);
    EXPECT_NE(nullptr, result);
    expectVersion(result, 1, 2, 0);
    celix_version_destroy(result);

    parseStatus = celix_version_parse("1", &result);
    EXPECT_EQ(CELIX_SUCCESS, parseStatus);
    EXPECT_NE(nullptr, result);
    expectVersion(result, 1, 0, 0);
    celix_version_destroy(result);

    parseStatus = celix_version_parse("", &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);

    parseStatus = celix_version_parse(nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);

    parseStatus = celix_version_parse("invalid", &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);

    parseStatus = celix_version_parse("-1", &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);

    parseStatus = celix_version_parse("1.-2", &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);

    parseStatus = celix_version_parse("1.2.-3", &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, parseStatus);
    EXPECT_EQ(nullptr, result);
}
