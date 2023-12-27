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

#include "celix_convert_utils.h"
#include <string>
#include <cmath>

class ConvertUtilsTestSuite : public ::testing::Test {
public:
    void checkVersion(const celix_version_t* version, int major, int minor, int micro, const char* qualifier) {
        EXPECT_TRUE(version != nullptr);
        if (version) {
            EXPECT_EQ(major, celix_version_getMajor(version));
            EXPECT_EQ(minor, celix_version_getMinor(version));
            EXPECT_EQ(micro, celix_version_getMicro(version));
            if (qualifier) {
                EXPECT_STREQ(qualifier, celix_version_getQualifier(version));
            } else {
                EXPECT_STREQ("", celix_version_getQualifier(version));
            }
        }
    }
};

TEST_F(ConvertUtilsTestSuite, ConvertToLongTest) {
    bool converted;
    //test for a valid string
    long result = celix_utils_convertStringToLong("10", 0, &converted);
    EXPECT_EQ(10, result);
    EXPECT_TRUE(converted);

    //test for an invalid string
    result = celix_utils_convertStringToLong("A", 0, &converted);
    EXPECT_EQ(0, result);
    EXPECT_FALSE(converted);

    //test for a string consisting of whitespaces
    result = celix_utils_convertStringToLong("   ", 1, &converted);
    EXPECT_EQ(1, result);
    EXPECT_FALSE(converted);

    //test for a string with a invalid number
    result = celix_utils_convertStringToLong("10A", 0, &converted);
    EXPECT_EQ(0, result);
    EXPECT_FALSE(converted);

    //test for a string with a number and a negative sign
    result = celix_utils_convertStringToLong("-10", 0, &converted);
    EXPECT_EQ(-10, result);
    EXPECT_TRUE(converted);

    //test for a string with a number and a positive sign
    result = celix_utils_convertStringToLong("+10", 0, &converted);
    EXPECT_EQ(10, result);
    EXPECT_TRUE(converted);

    //test for a convert with a nullptr for the converted parameter
    result = celix_utils_convertStringToLong("10", 0, nullptr);
    EXPECT_EQ(10, result);

    //test for a convert with a double value
    result = celix_utils_convertStringToLong("10.1", 0, &converted);
    EXPECT_EQ(0, result);
    EXPECT_FALSE(converted);

    //test for a convert with a long value with trailing whitespaces
    result = celix_utils_convertStringToLong("11 \t\n", 0, &converted);
    EXPECT_EQ(11, result);
    EXPECT_TRUE(converted);

    //test for a convert with a long value with starting and trailing whitespaces
    result = celix_utils_convertStringToLong("\t 12 \t\n", 0, &converted);
    EXPECT_EQ(12, result);
    EXPECT_TRUE(converted);
}

TEST_F(ConvertUtilsTestSuite, ConvertToDoubleTest) {
    bool converted;
    //test for a valid string
    double result = celix_utils_convertStringToDouble("10.5", 0, &converted);
    EXPECT_EQ(10.5, result);
    EXPECT_TRUE(converted);

    //test for an invalid string
    result = celix_utils_convertStringToDouble("A", 0, &converted);
    EXPECT_EQ(0, result);
    EXPECT_FALSE(converted);

    //test for an string consisting of whitespaces
    result = celix_utils_convertStringToDouble("  ", 1.0, &converted);
    EXPECT_EQ(1.0, result);
    EXPECT_FALSE(converted);

    //test for a string with a invalid number
    result = celix_utils_convertStringToDouble("10.5A", 0, &converted);
    EXPECT_FALSE(converted);

    //test for a string with a number and a negative sign
    result = celix_utils_convertStringToDouble("-10.5", 0, &converted);
    EXPECT_EQ(-10.5, result);
    EXPECT_TRUE(converted);

    //test for a string with a number and a positive sign
    result = celix_utils_convertStringToDouble("+10.5", 0, &converted);
    EXPECT_EQ(10.5, result);
    EXPECT_TRUE(converted);

    //test for a string with a scientific notation
    result = celix_utils_convertStringToDouble("1.0e-10", 0, &converted);
    EXPECT_EQ(1.0e-10, result);
    EXPECT_TRUE(converted);

    //test for a convert with a nullptr for the converted parameter
    result = celix_utils_convertStringToDouble("10.5", 0, nullptr);
    EXPECT_EQ(10.5, result);

    //test for a convert with an invalid double value with trailing info
    result = celix_utils_convertStringToDouble("11.1.2", 0, &converted);
    EXPECT_FALSE(converted);

    //test for a convert with a double value with trailing whitespaces
    result = celix_utils_convertStringToDouble("11.1 \t\n", 0, &converted);
    EXPECT_EQ(11.1, result);
    EXPECT_TRUE(converted);

    //test for a convert with a double value with starting and trailing whitespaces
    result = celix_utils_convertStringToDouble("\t 12.2 \t\n", 0, &converted);
    EXPECT_EQ(12.2, result);
    EXPECT_TRUE(converted);

    //test for a convert with an INF value
    result = celix_utils_convertStringToDouble("INF", 0, &converted);
    EXPECT_EQ(std::numeric_limits<double>::infinity(), result);
    EXPECT_TRUE(converted);

    //test for a convert with an -INF value
    result = celix_utils_convertStringToDouble(" -INF ", 0, &converted);
    EXPECT_EQ(-std::numeric_limits<double>::infinity(), result);
    EXPECT_TRUE(converted);

    //test for a convert with an NAN value
    result = celix_utils_convertStringToDouble(" NAN   ", 0, &converted);
    EXPECT_TRUE(std::isnan(result));
    EXPECT_TRUE(converted);
}

TEST_F(ConvertUtilsTestSuite, ConvertToBoolTest) {
    bool converted;
    //test for a valid string
    bool result = celix_utils_convertStringToBool("true", false, &converted);
    EXPECT_EQ(true, result);
    EXPECT_TRUE(converted);

    //test for an invalid string
    result = celix_utils_convertStringToBool("A", false, &converted);
    EXPECT_EQ(false, result);
    EXPECT_FALSE(converted);

    //test for a almost valid string
    result = celix_utils_convertStringToBool("trueA", false, &converted);
    EXPECT_EQ(false, result);
    EXPECT_FALSE(converted);

    //test for a almost valid string
    result = celix_utils_convertStringToBool("falseA", true, &converted);
    EXPECT_EQ(true, result);
    EXPECT_FALSE(converted);

    //test for a convert with a nullptr for the converted parameter
    result = celix_utils_convertStringToBool("true", false, nullptr);
    EXPECT_EQ(true, result);

    //test for a convert with a bool value with trailing chars
    result = celix_utils_convertStringToBool("true and ok", 0, &converted);
    EXPECT_FALSE(converted);

    //test for a convert with a bool value with trailing whitespaces
    result = celix_utils_convertStringToBool("true \t\n", 0, &converted);
    EXPECT_TRUE(result);
    EXPECT_TRUE(converted);

    //test for a convert with a bool value with starting and trailing whitespaces
    result = celix_utils_convertStringToBool("\t false \t\n", 0, &converted);
    EXPECT_FALSE(result);
    EXPECT_TRUE(converted);

    //test for a convert with nullptr for the val parameter
    result = celix_utils_convertStringToBool(nullptr, true, &converted);
    EXPECT_TRUE(result);
    EXPECT_FALSE(converted);

    result = celix_utils_convertStringToBool(nullptr, false, &converted);
    EXPECT_FALSE(result);
    EXPECT_FALSE(converted);
}

TEST_F(ConvertUtilsTestSuite, ConvertToVersionTest) {
    celix_version_t* defaultVersion = celix_version_create(1, 2, 3, "B");

    //test for a valid string
    celix_version_t* result = celix_utils_convertStringToVersion("1.2.3", nullptr, nullptr);
    checkVersion(result, 1, 2, 3, nullptr);
    celix_version_destroy(result);

    //test for an invalid string
    result = celix_utils_convertStringToVersion("A", nullptr, nullptr);
    EXPECT_EQ(nullptr, result);

    //test for a string with a number
    result = celix_utils_convertStringToVersion("1.2.3.A", nullptr, nullptr);
    checkVersion(result, 1, 2, 3, "A");
    celix_version_destroy(result);

    //test for a string with a partly (strict) version
    result = celix_utils_convertStringToVersion("1", nullptr, nullptr);
    EXPECT_EQ(nullptr, result);

    //test for a string with a partly (strict) version
    result = celix_utils_convertStringToVersion("1.2", nullptr, nullptr);
    EXPECT_EQ(nullptr, result);

    //test for a string with a valid version, default version and a converted bool arg
    bool converted;
    result = celix_utils_convertStringToVersion("1.2.3", defaultVersion, &converted);
    checkVersion(result, 1, 2, 3, nullptr);
    celix_version_destroy(result);
    EXPECT_TRUE(converted);

    //test for a string with a invalid version, default version and a converted bool arg
    result = celix_utils_convertStringToVersion("A", defaultVersion, &converted);
    checkVersion(result, 1, 2, 3, "B"); //default version
    celix_version_destroy(result);
    EXPECT_FALSE(converted);

    //test for a convert with a version value with trailing chars
    celix_utils_convertStringToVersion("2.1.1 and something else", nullptr, &converted);
    EXPECT_FALSE(converted);

    //test for a convert with a version value with trailing whitespaces
    result = celix_utils_convertStringToVersion("1.2.3 \t\n", nullptr, &converted);
    EXPECT_TRUE(converted);
    celix_version_destroy(result);

    //test for a convert with a version value with starting and trailing whitespaces
    result = celix_utils_convertStringToVersion("\t 3.2.2 \t\n", nullptr, &converted);
    EXPECT_TRUE(converted);
    celix_version_destroy(result);

    //test for a convert with a super long invalid version string
    std::string longString = "1";
    for (int i = 0; i < 128; ++i) {
        longString += ".1";
    }
    result = celix_utils_convertStringToVersion(longString.c_str(), nullptr, &converted);
    EXPECT_FALSE(converted);
    EXPECT_EQ(nullptr, result);

    celix_version_destroy(defaultVersion);
}