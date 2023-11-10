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

#include <gtest/gtest.h>

#include "celix_utils_ei.h"
#include "asprintf_ei.h"
#include "malloc_ei.h"
#include "celix_version.h"
#include "celix_err.h"


class VersionErrorInjectionTestSuite : public ::testing::Test {
public:
    VersionErrorInjectionTestSuite() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_err_resetErrors();
    }
};

TEST_F(VersionErrorInjectionTestSuite, CreateFailureTest) {
    celix_ei_expect_calloc((void*)celix_version_create, 0, nullptr);
    celix_version_t *version = celix_version_create(2, 2, 0, nullptr);
    EXPECT_EQ(nullptr, version);

    celix_ei_expect_celix_utils_strdup((void*)celix_version_create, 0, nullptr);
    version = celix_version_create(2, 2, 0, "qualifier");
    EXPECT_EQ(nullptr, version);

    EXPECT_EQ(2, celix_err_getErrorCount());
}

TEST_F(VersionErrorInjectionTestSuite, ToStringFailureTest) {
    celix_autoptr(celix_version_t) version = celix_version_create(2, 2, 0, "qualifier");

    celix_ei_expect_asprintf((void*)celix_version_toString, 0, -1);
    char *str = celix_version_toString(version);
    EXPECT_EQ(nullptr, str);
    EXPECT_EQ(1, celix_err_getErrorCount());
}