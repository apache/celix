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

#include "celix_json_utils_private.h"
#include "celix_err.h"
#include "celix_version_ei.h"
#include "jansson_ei.h"
#include "asprintf_ei.h"


class CelixJsonUtilsErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixJsonUtilsErrorInjectionTestSuite() = default;

    ~CelixJsonUtilsErrorInjectionTestSuite() override {
        celix_ei_expect_celix_version_toString(nullptr, 0, nullptr);
        celix_ei_expect_json_sprintf(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(CelixJsonUtilsErrorInjectionTestSuite, VersionToStringErrorWhenConvertingVersionToStringTest) {
    celix_ei_expect_celix_version_toString((void*)&celix_utils_versionToJson, 0, nullptr);
    celix_autoptr(celix_version_t) version = celix_version_create(1, 2, 3, nullptr);
    json_t* json = nullptr;
    auto status = celix_utils_versionToJson(version, &json);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixJsonUtilsErrorInjectionTestSuite, JsonSprintfErrorWhenConvertingVersionToStringTest) {
    celix_ei_expect_json_sprintf((void*)&celix_utils_versionToJson, 0, nullptr);
    celix_autoptr(celix_version_t) version = celix_version_create(1, 2, 3, nullptr);
    json_t* json = nullptr;
    auto status = celix_utils_versionToJson(version, &json);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixJsonUtilsErrorInjectionTestSuite, ExtractVersionErrorWhenConvertingJsonToVersionTest) {
    //celix_utils_jsonToVersion->celix_utils_writeOrCreateString->celix_utils_writeOrCreateVString->vasprintf,
    celix_ei_expect_vasprintf((void*)&celix_utils_jsonToVersion, 2, -1);
    json_auto_t* json = json_string("version<123456789.123456789.123456789.beta>");
    celix_autoptr(celix_version_t) version = nullptr;
    auto status = celix_utils_jsonToVersion(json, &version);
    ASSERT_EQ(ENOMEM, status);
}