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

#include "celix_err.h"

class ConvertUtilsTestSuite : public ::testing::Test {
  public:
    ~ConvertUtilsTestSuite() noexcept override { celix_err_printErrors(stderr, nullptr, nullptr); }

    static void checkVersion(const celix_version_t* version, int major, int minor, int micro, const char* qualifier) {
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

    //test for a string with an invalid number
    result = celix_utils_convertStringToDouble("10.5A", 0, &converted);
    EXPECT_EQ(0, result);
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
    EXPECT_EQ(0, result);
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
    result = celix_utils_convertStringToBool("true and ok", false, &converted);
    EXPECT_FALSE(result);
    EXPECT_FALSE(converted);

    //test for a convert with a bool value with trailing whitespaces
    result = celix_utils_convertStringToBool("true \t\n", false, &converted);
    EXPECT_TRUE(result);
    EXPECT_TRUE(converted);

    //test for a convert with a bool value with starting and trailing whitespaces
    result = celix_utils_convertStringToBool("\t false \t\n", false, &converted);
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
    celix_version_t* result;
    celix_status_t convertStatus = celix_utils_convertStringToVersion("1.2.3", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_TRUE(result != nullptr);
    checkVersion(result, 1, 2, 3, nullptr);
    celix_version_destroy(result);

    //test for an invalid string
    convertStatus = celix_utils_convertStringToVersion("A", nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertStatus);
    EXPECT_EQ(nullptr, result);

    //test for a string with a number
    convertStatus = celix_utils_convertStringToVersion("1.2.3.A", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_TRUE(result != nullptr);
    checkVersion(result, 1, 2, 3, "A");
    celix_version_destroy(result);

    //test for a string with a partly (strict) version
    convertStatus = celix_utils_convertStringToVersion("1", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_NE(nullptr, result);
    checkVersion(result, 1, 0, 0, nullptr);
    celix_version_destroy(result);

    //test for a string with a partly (strict) version
    convertStatus = celix_utils_convertStringToVersion("1.2", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_NE(nullptr, result);
    checkVersion(result, 1, 2, 0, nullptr);
    celix_version_destroy(result);

    //test for a string with a valid version, default version and a converted bool arg
    convertStatus = celix_utils_convertStringToVersion("1.2.3", defaultVersion, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_NE(nullptr, result);
    checkVersion(result, 1, 2, 3, nullptr);
    celix_version_destroy(result);

    //test for a string with an invalid version and a default version
    convertStatus = celix_utils_convertStringToVersion("A", defaultVersion, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertStatus);
    EXPECT_NE(nullptr, result);
    checkVersion(result, 1, 2, 3, "B"); //default version
    celix_version_destroy(result);

    //test for a convert with a version value with trailing chars
    convertStatus = celix_utils_convertStringToVersion("2.1.1 and something else", nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertStatus);
    EXPECT_EQ(nullptr, result);

    //test for a convert with a version value with trailing whitespaces
    convertStatus = celix_utils_convertStringToVersion("1.2.3 \t\n", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_NE(nullptr, result);
    celix_version_destroy(result);

    //test for a convert with a version value with starting and trailing whitespaces
    convertStatus = celix_utils_convertStringToVersion("\t 3.2.2 \t\n", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertStatus);
    EXPECT_NE(nullptr, result);
    celix_version_destroy(result);

    //test for a convert with a super long invalid version string
    std::string longString = "1";
    for (int i = 0; i < 128; ++i) {
        longString += ".1";
    }
    convertStatus = celix_utils_convertStringToVersion(longString.c_str(), nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertStatus);
    EXPECT_EQ(nullptr, result);

    convertStatus = celix_utils_convertStringToVersion(nullptr, defaultVersion, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertStatus);
    EXPECT_NE(nullptr, result); //copy of default version
    celix_version_destroy(result);

    celix_version_destroy(defaultVersion);
}

TEST_F(ConvertUtilsTestSuite, ConvertToLongArrayTest) {
    celix_array_list_t* result;
    celix_status_t convertState = celix_utils_convertStringToLongArrayList("1,2,3", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    EXPECT_EQ(2L, celix_arrayList_getLong(result, 1));
    celix_arrayList_destroy(result);

    convertState = celix_utils_convertStringToLongArrayList("invalid", nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertState);
    EXPECT_TRUE(result == nullptr);

    celix_autoptr(celix_array_list_t) defaultList = celix_arrayList_create();
    celix_arrayList_addLong(defaultList, 42L);
    convertState = celix_utils_convertStringToLongArrayList("1,2,3,invalid", defaultList, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertState);
    EXPECT_TRUE(result != nullptr); //copy of default list
    EXPECT_EQ(1, celix_arrayList_size(result));
    EXPECT_EQ(42L, celix_arrayList_getLong(result, 0));
    celix_arrayList_destroy(result);

    convertState = celix_utils_convertStringToLongArrayList("  1  ,  2  ,  3   ", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    EXPECT_EQ(2L, celix_arrayList_getLong(result, 1));
    celix_arrayList_destroy(result);

    convertState = celix_utils_convertStringToLongArrayList(nullptr, defaultList, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertState);
    EXPECT_TRUE(result != nullptr); //copy of default list
    EXPECT_EQ(1, celix_arrayList_size(result));
    celix_arrayList_destroy(result);
}

TEST_F(ConvertUtilsTestSuite, LongArrayToStringTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    celix_arrayList_addLong(list, 1L);
    celix_arrayList_addLong(list, 2L);
    celix_arrayList_addLong(list, 3L);

    char* result = celix_utils_longArrayListToString(list);
    EXPECT_STREQ("1,2,3", result);
    free(result);

    celix_autoptr(celix_array_list_t) emptyList = celix_arrayList_create();
    result = celix_utils_longArrayListToString(emptyList);
    EXPECT_STREQ("", result);
    free(result);

    celix_autoptr(celix_array_list_t) singleEntryList = celix_arrayList_create();
    celix_arrayList_addLong(singleEntryList, 1L);
    result = celix_utils_longArrayListToString(singleEntryList);
    EXPECT_STREQ("1", result);
    free(result);
}

TEST_F(ConvertUtilsTestSuite, ConvertToDoubleArrayList) {
    celix_array_list_t* result;
    celix_status_t convertState = celix_utils_convertStringToDoubleArrayList("0.1,2.0,3.1,4,5", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(5, celix_arrayList_size(result));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(result, 1));
    EXPECT_DOUBLE_EQ(5.0, celix_arrayList_getDouble(result, 4));
    celix_arrayList_destroy(result);

    // NOTE celix_utils_convertStringToDoubleArrayList uses the same generic function as is used in
    // celix_utils_convertStringToLongArrayList and because celix_utils_convertStringToLongArrayList is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, DoubleArrayToStringTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    celix_arrayList_addDouble(list, 0.1);
    celix_arrayList_addDouble(list, 2.0);
    celix_arrayList_addDouble(list, 3.3);

    char* result = celix_utils_doubleArrayListToString(list); //note result is not limited to 2 decimals, so using strstr
    EXPECT_TRUE(strstr(result, "0.1") != nullptr);
    EXPECT_TRUE(strstr(result, "2.0") != nullptr);
    EXPECT_TRUE(strstr(result, "3.3") != nullptr);
    free(result);

    // NOTE celix_utils_doubleArrayListToString uses the same generic function as is used in
    // celix_utils_longArrayListToString and because celix_utils_longArrayListToString is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, ConvertToBoolArrayList) {
    celix_array_list_t* result;
    celix_status_t convertState = celix_utils_convertStringToBoolArrayList("true,false,true", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    EXPECT_TRUE(celix_arrayList_getBool(result, 0));
    EXPECT_FALSE(celix_arrayList_getBool(result, 1));
    EXPECT_TRUE(celix_arrayList_getBool(result, 2));
    celix_arrayList_destroy(result);

    // NOTE celix_utils_convertStringToBoolArrayList uses the same generic function as is used in
    // celix_utils_convertStringToLongArrayList and because celix_utils_convertStringToLongArrayList is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, BoolArrayToStringTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    celix_arrayList_addBool(list, true);
    celix_arrayList_addBool(list, false);
    celix_arrayList_addBool(list, true);

    char* result = celix_utils_boolArrayListToString(list);
    EXPECT_STREQ("true,false,true", result);
    free(result);

    // NOTE celix_utils_boolArrayListToString uses the same generic function as is used in
    // celix_utils_longArrayListToString and because celix_utils_longArrayListToString is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, ConvertToStringArrayList) {
    celix_array_list_t* result;
    celix_status_t convertState = celix_utils_convertStringToStringArrayList("a,b,c", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    EXPECT_STREQ("a", (char*)celix_arrayList_get(result, 0));
    EXPECT_STREQ("b", (char*)celix_arrayList_get(result, 1));
    EXPECT_STREQ("c", (char*)celix_arrayList_get(result, 2));
    celix_arrayList_destroy(result);

    convertState = celix_utils_convertStringToStringArrayList(R"(a,b\\\,,c)", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    EXPECT_STREQ("a", celix_arrayList_getString(result, 0));
    EXPECT_STREQ("b\\,", celix_arrayList_getString(result, 1));
    EXPECT_STREQ("c", celix_arrayList_getString(result, 2));
    celix_arrayList_destroy(result);

    convertState = celix_utils_convertStringToStringArrayList("a,,b,", nullptr, &result); //4 entries, second and last are empty strings
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(4, celix_arrayList_size(result));
    EXPECT_STREQ("a", celix_arrayList_getString(result, 0));
    EXPECT_STREQ("", celix_arrayList_getString(result, 1));
    EXPECT_STREQ("b", celix_arrayList_getString(result, 2));
    EXPECT_STREQ("", celix_arrayList_getString(result, 3));
    celix_arrayList_destroy(result);

    //invalid escape sequence
    convertState = celix_utils_convertStringToStringArrayList(R"(a,b\c,d)", nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertState);
    EXPECT_TRUE(result == nullptr);
    convertState = celix_utils_convertStringToStringArrayList(R"(a,b,c\)", nullptr, &result);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, convertState);
    EXPECT_TRUE(result == nullptr);

    // NOTE celix_utils_convertStringToStringArrayList uses the same generic function as is used in
    // celix_utils_convertStringToLongArrayList and because celix_utils_convertStringToLongArrayList is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, StringArrayToStringTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    celix_arrayList_addString(list, "a");
    celix_arrayList_addString(list, "b");
    celix_arrayList_addString(list, "c");

    char* result = celix_utils_stringArrayListToString(list);
    EXPECT_STREQ("a,b,c", result);
    free(result);

    celix_arrayList_addString(list, "d\\,");
    celix_arrayList_addString(list, "e");
    result = celix_utils_stringArrayListToString(list);
    EXPECT_STREQ(R"(a,b,c,d\\\,,e)", result);

    //Check if the result can be converted back to an equal list
    celix_array_list_t* listResult;
    celix_status_t convertState = celix_utils_convertStringToStringArrayList(result, nullptr, &listResult);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(listResult != nullptr);

    EXPECT_EQ(celix_arrayList_size(list), celix_arrayList_size(listResult));
    for (int i = 0; i < celix_arrayList_size(list); ++i) {
        EXPECT_STREQ((char*)celix_arrayList_get(list, i), (char*)celix_arrayList_get(listResult, i));
    }
}

TEST_F(ConvertUtilsTestSuite, ConvertToVersionArrayList) {
    celix_array_list_t* result;
    celix_status_t convertState = celix_utils_convertStringToVersionArrayList("1.2.3,2.3.4,3.4.5.qualifier", nullptr, &result);
    EXPECT_EQ(CELIX_SUCCESS, convertState);
    EXPECT_TRUE(result != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(result));
    checkVersion((celix_version_t*)celix_arrayList_get(result, 0), 1, 2, 3, nullptr);
    checkVersion((celix_version_t*)celix_arrayList_get(result, 1), 2, 3, 4, nullptr);
    checkVersion((celix_version_t*)celix_arrayList_get(result, 2), 3, 4, 5, "qualifier");
    celix_arrayList_destroy(result);

    // NOTE celix_utils_convertStringToVersionArrayList uses the same generic function as is used in
    // celix_utils_convertStringToLongArrayList and because celix_utils_convertStringToLongArrayList is already
    // tested, we only test a few cases here.
}

TEST_F(ConvertUtilsTestSuite, VersionArrayToStringTest) {
    celix_autoptr(celix_version_t) v1 = celix_version_create(1, 2, 3, nullptr);
    celix_autoptr(celix_version_t) v2 = celix_version_create(2, 3, 4, nullptr);
    celix_autoptr(celix_version_t) v3 = celix_version_create(3, 4, 5, "qualifier");
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    celix_arrayList_add(list, v1);
    celix_arrayList_add(list, v2);
    celix_arrayList_add(list, v3);

    char* result = celix_utils_versionArrayListToString(list);
    EXPECT_STREQ("1.2.3,2.3.4,3.4.5.qualifier", result);
    free(result);

    // NOTE celix_utils_versionArrayListToString uses the same generic function as is used in
    // celix_utils_longArrayListToString and because celix_utils_longArrayListToString is already
    // tested, we only test a few cases here.
}


