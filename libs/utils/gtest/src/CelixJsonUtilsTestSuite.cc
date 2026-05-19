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

#include <limits>

#include "celix_stdlib_cleanup.h"
#include "celix_json_utils_private.h"
#include "celix_err.h"

class CelixJsonUtilsTestSuite : public ::testing::Test {
public:
    CelixJsonUtilsTestSuite() { celix_err_resetErrors(); }
};

TEST_F(CelixJsonUtilsTestSuite, VersionToJsonTest) {
    celix_autoptr(celix_version_t) version = celix_version_create(1, 2, 3, nullptr);
    json_auto_t* json = nullptr;
    auto status = celix_utils_versionToJson(version, &json);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(json != nullptr);
    ASSERT_STREQ("version<1.2.3>", json_string_value(json));
}

TEST_F(CelixJsonUtilsTestSuite, JsonToVersionTest) {
    json_auto_t* json = json_string("version<1.2.3>");
    celix_autoptr(celix_version_t) version = nullptr;
    auto status = celix_utils_jsonToVersion(json, &version);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(version != nullptr);
    ASSERT_EQ(1, celix_version_getMajor(version));
    ASSERT_EQ(2, celix_version_getMinor(version));
    ASSERT_EQ(3, celix_version_getMicro(version));
}

TEST_F(CelixJsonUtilsTestSuite, WrongVersionStringToVersionTest) {
    {
        json_auto_t* json = json_string("1.2.3");
        celix_version_t* version = nullptr;
        auto status = celix_utils_jsonToVersion(json, &version);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }

    {
        json_auto_t* json = json_string("version<1.s.3>");
        celix_version_t* version = nullptr;
        auto status = celix_utils_jsonToVersion(json, &version);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }
}

TEST_F(CelixJsonUtilsTestSuite, InvalidVersionStringTest) {
    {
        json_auto_t* json = json_string("version<1.2.3");
        auto result = celix_utils_isVersionJsonString(json);
        ASSERT_FALSE(result);
    }

    {
        json_auto_t* json = json_string("version1.2.3>");
        auto result = celix_utils_isVersionJsonString(json);
        ASSERT_FALSE(result);
    }

    {
        json_auto_t* json = json_integer(1);
        auto result = celix_utils_isVersionJsonString(json);
        ASSERT_FALSE(result);
    }
}

TEST_F(CelixJsonUtilsTestSuite, JsonErrorToCelixStatusTest) {
    ASSERT_EQ(CELIX_ILLEGAL_STATE, celix_utils_jsonErrorToStatus(json_error_unknown));
    ASSERT_EQ(ENOMEM, celix_utils_jsonErrorToStatus(json_error_out_of_memory));
    ASSERT_EQ(ENOMEM, celix_utils_jsonErrorToStatus(json_error_stack_overflow));
    ASSERT_EQ(CELIX_FILE_IO_EXCEPTION, celix_utils_jsonErrorToStatus(json_error_cannot_open_file));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_invalid_argument));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_invalid_utf8));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_premature_end_of_input));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_end_of_input_expected));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_invalid_syntax));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_invalid_format));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_wrong_type));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_null_character));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_null_value));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_null_byte_in_key));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_duplicate_key));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_numeric_overflow));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_item_not_found));
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_utils_jsonErrorToStatus(json_error_index_out_of_range));
}

TEST_F(CelixJsonUtilsTestSuite, BinaryToJsonTest) {
    {
        char bin[3]= {1,2,3};
        json_auto_t* json = nullptr;
        auto status = celix_utils_binaryToJson(bin, sizeof(bin), &json);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(json != nullptr);
        ASSERT_STREQ("base64<AQID>", json_string_value(json));
    }

    {
        char bin[2]= {1,2};
        json_auto_t* json = nullptr;
        auto status = celix_utils_binaryToJson(bin, sizeof(bin), &json);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(json != nullptr);
        ASSERT_STREQ("base64<AQI=>", json_string_value(json));
    }

    {
        char bin[1]= {1};
        json_auto_t* json = nullptr;
        auto status = celix_utils_binaryToJson(bin, sizeof(bin), &json);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(json != nullptr);
        ASSERT_STREQ("base64<AQ==>", json_string_value(json));
    }

    {
        char bin[1];
        json_auto_t* json = nullptr;
        auto status = celix_utils_binaryToJson(bin, std::numeric_limits<int>::max(), &json);
        ASSERT_EQ(ENOMEM, status);
        ASSERT_EQ(nullptr, json);
    }
}

TEST_F(CelixJsonUtilsTestSuite, JsonToBinaryTest) {
    {
        json_auto_t* json = json_string("base64<AQID>");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(data != nullptr);
        ASSERT_EQ(3, size);
        char* bin = (char*)data;
        ASSERT_EQ(1, bin[0]);
        ASSERT_EQ(2, bin[1]);
        ASSERT_EQ(3, bin[2]);
    }

    {
        json_auto_t* json = json_string("base64<AQI=>");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(data != nullptr);
        ASSERT_EQ(2, size);
        char* bin = (char*)data;
        ASSERT_EQ(1, bin[0]);
        ASSERT_EQ(2, bin[1]);
    }

    {
        json_auto_t* json = json_string("base64<AQ==>");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_TRUE(data != nullptr);
        ASSERT_EQ(1, size);
        char* bin = (char*)data;
        ASSERT_EQ(1, bin[0]);
    }
}

TEST_F(CelixJsonUtilsTestSuite, WrongBinaryJsonToBinaryTest) {
    {
        json_auto_t* json = json_string("base64<Invalid>");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        ASSERT_TRUE(data == nullptr);
    }

    {
        json_auto_t* json = json_string("base64<AQID");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        ASSERT_TRUE(data == nullptr);
    }

    {
        json_auto_t* json = json_string("base64<>");
        celix_autofree void* data = nullptr;
        size_t size = 0;
        auto status = celix_utils_jsonToBinary(json, &data, &size);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        ASSERT_TRUE(data == nullptr);
    }
}

TEST_F(CelixJsonUtilsTestSuite, InvalidBinaryJsonTest) {
    {
        json_auto_t* json = json_string("base64<AQID");
        auto result = celix_utils_isBinaryJsonString(json);
        ASSERT_FALSE(result);
    }

    {
        json_auto_t* json = json_string("base64AQID>");
        auto result = celix_utils_isBinaryJsonString(json);
        ASSERT_FALSE(result);
    }

    {
        json_auto_t* json = json_integer(1);
        auto result = celix_utils_isBinaryJsonString(json);
        ASSERT_FALSE(result);
    }
}