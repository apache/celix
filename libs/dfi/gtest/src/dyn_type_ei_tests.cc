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

#include "dyn_type.h"
#include "celix_err.h"
#include "malloc_ei.h"
#include "stdio_ei.h"
#include "string_ei.h"

#include <errno.h>
#include <gtest/gtest.h>
#include <string>

class DynTypeErrorInjectionTestSuite : public ::testing::Test {
public:
    DynTypeErrorInjectionTestSuite() {
    }

    ~DynTypeErrorInjectionTestSuite() override {
        celix_ei_expect_strdup(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_fmemopen(nullptr, 0, nullptr);
    }
    // delete other constructors and assign operators
    DynTypeErrorInjectionTestSuite(DynTypeErrorInjectionTestSuite const&) = delete;
    DynTypeErrorInjectionTestSuite(DynTypeErrorInjectionTestSuite&&) = delete;
    DynTypeErrorInjectionTestSuite& operator=(DynTypeErrorInjectionTestSuite const&) = delete;
    DynTypeErrorInjectionTestSuite& operator=(DynTypeErrorInjectionTestSuite&&) = delete;
};

TEST_F(DynTypeErrorInjectionTestSuite, ParseTypeErrors) {
    dyn_type *type = NULL;
    const char* descriptor = "{D{DD b_1 b_2}I a b c}";

    // fail to open memory as stream
    celix_ei_expect_fmemopen((void*)dynType_parseWithStr, 0, nullptr);
    int status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    std::string msg = "Error creating mem stream for descriptor string. ";
    msg += strerror(ENOMEM);
    ASSERT_STREQ(msg.c_str(), celix_err_popLastError());

    // fail to allocate dyn_type
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 2, nullptr);
    status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for type", celix_err_popLastError());

    // fail to duplicate type name
    celix_ei_expect_strdup((void*)dynType_parseWithStr, 1, nullptr);
    status = dynType_parseWithStr(descriptor, "hello", NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error strdup'ing name 'hello'", celix_err_popLastError());
}
