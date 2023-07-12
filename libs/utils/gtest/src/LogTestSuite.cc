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

#include "celix_log_level.h"

class LogTestSuite : public ::testing::Test {};

TEST_F(LogTestSuite, LogLevelToString) {
    EXPECT_STREQ("trace", celix_logLevel_toString(CELIX_LOG_LEVEL_TRACE));
    EXPECT_STREQ("debug", celix_logLevel_toString(CELIX_LOG_LEVEL_DEBUG));
    EXPECT_STREQ("info", celix_logLevel_toString(CELIX_LOG_LEVEL_INFO));
    EXPECT_STREQ("warning", celix_logLevel_toString(CELIX_LOG_LEVEL_WARNING));
    EXPECT_STREQ("error", celix_logLevel_toString(CELIX_LOG_LEVEL_ERROR));
    EXPECT_STREQ("fatal", celix_logLevel_toString(CELIX_LOG_LEVEL_FATAL));
    EXPECT_STREQ("disabled", celix_logLevel_toString(CELIX_LOG_LEVEL_DISABLED));
}

TEST_F(LogTestSuite, LogLevelFromString) {
    EXPECT_EQ(CELIX_LOG_LEVEL_TRACE, celix_logLevel_fromString("trace", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DEBUG, celix_logLevel_fromString("debug", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_INFO, celix_logLevel_fromString("info", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_WARNING, celix_logLevel_fromString("warning", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_ERROR, celix_logLevel_fromString("error", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_FATAL, celix_logLevel_fromString("fatal", CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logLevel_fromString("disabled", CELIX_LOG_LEVEL_DISABLED));


    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logLevel_fromString(nullptr, CELIX_LOG_LEVEL_DISABLED));
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logLevel_fromString("garbage", CELIX_LOG_LEVEL_DISABLED));

    bool converted;
    EXPECT_EQ(CELIX_LOG_LEVEL_FATAL, celix_logLevel_fromStringWithCheck("fatal", CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_TRUE(converted);
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logLevel_fromStringWithCheck(nullptr, CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_FALSE(converted);
    EXPECT_EQ(CELIX_LOG_LEVEL_DISABLED, celix_logLevel_fromStringWithCheck("garbage", CELIX_LOG_LEVEL_DISABLED, &converted));
    EXPECT_FALSE(converted);
}
