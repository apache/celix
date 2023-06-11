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

TEST_F(CelixErrnoTestSuite, DoIfTest) {
    // Given a count of 0
    int count = 0;
    // And a CELIX_SUCCESS status
    celix_status_t status = CELIX_SUCCESS;
    // And a function that increments the count and returns CELIX_SUCCESS
    auto incrementAndReturnSuccess = [&count]() -> celix_status_t {
        count++;
        return CELIX_SUCCESS;
    };
    // And a function that increments the count and returns a error (CELIX_BUNDLE_EXCEPTION)
    auto incrementAndReturnError = [&count]() -> celix_status_t {
        count++;
        return CELIX_BUNDLE_EXCEPTION;
    };

    // When CELIX_DO_IF is called with a success status
    status = CELIX_DO_IF(CELIX_SUCCESS, incrementAndReturnSuccess());

    //Then the count is 1 and the status is CELIX_SUCCESS
    EXPECT_EQ(1, count);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //When CELIX_DO_IF is called with a error status
    status = CELIX_DO_IF(status, incrementAndReturnError());

    //Then the count is still 2 and the status is CELIX_BUNDLE_EXCEPTION
    EXPECT_EQ(2, count);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);

    //When CELIX_DO_IF is called with a error status
    status = CELIX_DO_IF(status, incrementAndReturnSuccess());

    //Then the count is still 2 and the status is still CELIX_BUNDLE_EXCEPTION (last incrementAndReturnSuccess not called)
    EXPECT_EQ(2, count);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(CelixErrnoTestSuite, GotoIfErrMacroTest) {
    // Given a count of 0
    int count = 0;
    // And a label exit_checkt with a count check (see end of test)


    //When CELIX_GOTO_IF_ERR is called with CELIX_SUCCESS 
    CELIX_GOTO_IF_ERR(CELIX_SUCCESS, exit_check);
    //Then the count is still 0
    EXPECT_EQ(count, 0);

    //When the count is increased
    count++;
    //And CELIX_GOTO_IF_ERR is called with a error (ENOMEM)
    CELIX_GOTO_IF_ERR(ENOMEM, exit_check);
    //Then this will not be reached
    FAIL() << "Shold not be reached";

    return;
exit_check:
    EXPECT_EQ(count, 1);
}
