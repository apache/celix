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

#include "celix/Exceptions.h"

class ExceptionsTestSuite : public ::testing::Test {
public:
};


TEST_F(ExceptionsTestSuite, ThrowExceptionTest) {
    EXPECT_THROW(celix::impl::throwException(CELIX_ILLEGAL_ARGUMENT, "Test"), celix::IllegalArgumentException);
    try {
        celix::impl::throwException(CELIX_ILLEGAL_ARGUMENT, "Test");
    } catch (const celix::IllegalArgumentException& ex) {
        EXPECT_STREQ("Test (Illegal argument)", ex.what());
    }

    EXPECT_THROW(celix::impl::throwException(CELIX_FILE_IO_EXCEPTION, "Test"), celix::IOException);
    try {
            celix::impl::throwException(CELIX_FILE_IO_EXCEPTION, "Test");
    } catch (const celix::IOException& ex) {
            EXPECT_STREQ("Test (File I/O exception)", ex.what());
    }

    //Not all celix_status_t values are mapped and in that case the default Exception is thrown
    EXPECT_THROW(celix::impl::throwException(CELIX_FRAMEWORK_EXCEPTION, "Test"), celix::Exception);
    try {
        celix::impl::throwException(CELIX_FRAMEWORK_EXCEPTION, "Test");
    } catch (const celix::Exception& ex) {
        EXPECT_STREQ("Test (Framework exception)", ex.what());
    }
}

