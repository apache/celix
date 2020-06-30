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
    EXPECT_STREQ("debug", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_DEBUG));
    EXPECT_STREQ("info", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_INFO));
    EXPECT_STREQ("warning", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_WARNING));
    EXPECT_STREQ("error", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_ERROR));
    EXPECT_STREQ("fatal", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_FATAL));
    EXPECT_STREQ("disabled", celix_logUtils_logLevelToString(CELIX_LOG_LEVEL_DISABLED));
}

TEST_F(LogUtilsTestSuite, LogLevelFromString) {
    EXPECT_EQ(CELIX_LOG_LEVEL_TRACE, celix_logUtils_logLevelFromString("trace", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DEBUG, celix_logUtils_logLevelFromString("debug", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_INFO, celix_logUtils_logLevelFromString("info", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_WARNING, celix_logUtils_logLevelFromString("warning", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_ERROR, celix_logUtils_logLevelFromString("error", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_FATAL, celix_logUtils_logLevelFromString("fatal", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logUtils_logLevelFromString("disabled", CELIX_LOG_LEVEL_DISABLED));


    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logUtils_logLevelFromString(nullptr, CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logUtils_logLevelFromString("garbage", CELIX_LOG_LEVEL_DISABLED));

    bool converted;
    EXPECT_EQ(CELIX_LOG_LEVEL_FATAL, celix_logUtils_logLevelFromStringWithCheck("fatal", CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_TRUE(converted);
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logUtils_logLevelFromStringWithCheck(nullptr, CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_FALSE(converted);
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logUtils_logLevelFromStringWithCheck("garbage", CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_FALSE(converted);
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