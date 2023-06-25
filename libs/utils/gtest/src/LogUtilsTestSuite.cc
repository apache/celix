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

#include "celix_log_utils.h"

class LogUtilsTestSuite : public ::testing::Test {};

TEST_F(LogUtilsTestSuite, LogLevelToString) {
    EXPECT_STREQ("trace", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_TRACE));
    //note rest is tested in LogTestSuite
}

TEST_F(LogUtilsTestSuite, LogLevelFromString) {
    EXPECT_EQ(CELIX_LOG_LEVEL_TRACE, celix_logUtils_logLevelFromString("trace", CELIX_LOG_LEVEL_DISABLED));
    //note rest is tested in LogTestSuite
}

TEST_F(LogUtilsTestSuite, LogToStdout) {
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    celix_logUtils_logToStdout("TestLogger", CELIX_LOG_LEVEL_INFO, "testing_stdout %i %i %i", 1, 2, 3);
    celix_logUtils_logToStdout("TestLogger", CELIX_LOG_LEVEL_ERROR, "testing_stderr %i %i %i", 1, 2, 3);
    celix_logUtils_logToStdout("TestLogger", CELIX_LOG_LEVEL_DISABLED, "ignore"); //will be ignored
    celix_logUtils_logToStdoutDetails("TestLogger", CELIX_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "testing_stderr %i %i %i", 1, 2, 3);

    std::string stdOutput = testing::internal::GetCapturedStdout();
    std::string errOutput = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(strstr(stdOutput.c_str(), "testing_stdout 1 2 3") != nullptr);
    EXPECT_TRUE(strstr(stdOutput.c_str(), "ignore") == nullptr);
    EXPECT_TRUE(strstr(errOutput.c_str(), "testing_stderr 1 2 3") != nullptr);
    EXPECT_TRUE(strstr(stdOutput.c_str(), "ignore") == nullptr);
}
