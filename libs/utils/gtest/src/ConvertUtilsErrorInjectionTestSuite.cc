/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include "celix_convert_utils.h"
#include "celix_utils_ei.h"
#include <gtest/gtest.h>

class ConvertUtilsWithErrorInjectionTestSuite : public ::testing::Test {
public:
    ~ConvertUtilsWithErrorInjectionTestSuite() override {
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
    }
};

TEST_F(ConvertUtilsWithErrorInjectionTestSuite, ConvertToVersionTest) {
    celix_version_t* defaultVersion = celix_version_create(1, 2, 3, "B");
    celix_ei_expect_celix_utils_writeOrCreateString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    celix_version_t* result = celix_utils_convertStringToVersion("1.2.3", nullptr, nullptr);
    EXPECT_EQ(nullptr, result);

    celix_version_destroy(defaultVersion);
}
