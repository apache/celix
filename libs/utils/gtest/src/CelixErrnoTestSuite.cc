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

class CelixErrnoTestSuite : public ::testing::Test {
public:
};

TEST_F(CelixErrnoTestSuite, TestCheckFacility) {
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_SUCCESS, CELIX_FACILITY_FRAMEWORK));

    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_INVALID_BUNDLE_CONTEXT, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_ILLEGAL_ARGUMENT, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_INVALID_SYNTAX, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_FRAMEWORK_SHUTDOWN, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_ILLEGAL_STATE, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_FRAMEWORK_EXCEPTION, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_FILE_IO_EXCEPTION, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_SERVICE_EXCEPTION, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_SERVICE_EXCEPTION, CELIX_FACILITY_FRAMEWORK));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(
            CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK,CELIX_INTERCEPTOR_EXCEPTION/*last exp*/+1),
            CELIX_FACILITY_FRAMEWORK));

    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_ERROR_MAKE(CELIX_FACILITY_HTTP,1), CELIX_FACILITY_FRAMEWORK));
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP,1), CELIX_FACILITY_FRAMEWORK));
}

TEST_F(CelixErrnoTestSuite, TestCheckCustomerError) {
    EXPECT_FALSE(celix_utils_isCustomerStatusCode(CELIX_SUCCESS));
    EXPECT_FALSE(celix_utils_isCustomerStatusCode(CELIX_INVALID_SYNTAX));
    EXPECT_FALSE(celix_utils_isCustomerStatusCode(ENOMEM));
    EXPECT_TRUE(celix_utils_isCustomerStatusCode(CELIX_CUSTOMER_ERROR_MAKE(1, 1)));
}


TEST_F(CelixErrnoTestSuite, TestFrameworkFacility) {
    //test celix_strerror
    EXPECT_STREQ("Success", celix_strerror(CELIX_SUCCESS));
    EXPECT_STREQ("Bundle exception", celix_strerror(CELIX_BUNDLE_EXCEPTION));
    EXPECT_STREQ("Invalid bundle context", celix_strerror(CELIX_INVALID_BUNDLE_CONTEXT));
    EXPECT_STREQ("Illegal argument", celix_strerror(CELIX_ILLEGAL_ARGUMENT));
    EXPECT_STREQ("Invalid syntax", celix_strerror(CELIX_INVALID_SYNTAX));
    EXPECT_STREQ("Framework shutdown", celix_strerror(CELIX_FRAMEWORK_SHUTDOWN));
    EXPECT_STREQ("Illegal state", celix_strerror(CELIX_ILLEGAL_STATE));
    EXPECT_STREQ("Framework exception", celix_strerror(CELIX_FRAMEWORK_EXCEPTION));
    EXPECT_STREQ("File I/O exception", celix_strerror(CELIX_FILE_IO_EXCEPTION));
    EXPECT_STREQ("Service exception", celix_strerror(CELIX_SERVICE_EXCEPTION));
    EXPECT_STREQ("Interceptor exception", celix_strerror(CELIX_INTERCEPTOR_EXCEPTION));
    EXPECT_STREQ("Unknown framework error code", celix_strerror(
            CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK,CELIX_INTERCEPTOR_EXCEPTION/*last exp*/+1)));
}

TEST_F(CelixErrnoTestSuite, TestCErrnoFacility) {
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(ENOMEM, CELIX_FACILITY_CERRNO));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(EAGAIN, CELIX_FACILITY_CERRNO));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_ENOMEM, CELIX_FACILITY_CERRNO));

    EXPECT_TRUE(strcasestr(celix_strerror(ENOMEM), "memory") != nullptr);
}

TEST_F(CelixErrnoTestSuite, TestHttpFacility) {
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_SUCCESS, CELIX_FACILITY_HTTP));
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_ILLEGAL_STATE, CELIX_FACILITY_HTTP));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_ERROR_MAKE(CELIX_FACILITY_HTTP, 1), CELIX_FACILITY_HTTP));

    EXPECT_STREQ("HTTP error", celix_strerror(CELIX_ERROR_MAKE(CELIX_FACILITY_HTTP, 1)));
}

TEST_F(CelixErrnoTestSuite, TestZipFacility) {
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_SUCCESS, CELIX_FACILITY_ZIP));
    EXPECT_FALSE(celix_utils_isStatusCodeFromFacility(CELIX_ILLEGAL_STATE, CELIX_FACILITY_ZIP));
    EXPECT_TRUE(celix_utils_isStatusCodeFromFacility(CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, 1), CELIX_FACILITY_ZIP));

    EXPECT_STREQ("ZIP error", celix_strerror(CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, 1)));
}
