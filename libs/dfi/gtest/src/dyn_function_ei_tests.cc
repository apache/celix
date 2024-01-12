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

#include "dyn_function.h"
#include "dyn_example_functions.h"

#include "celix_err.h"
#include "ffi_ei.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

#include <errno.h>
#include <gtest/gtest.h>
#include <string>
#include <string.h>

class DynFunctionErrorInjectionTestSuite : public ::testing::Test {
public:
    DynFunctionErrorInjectionTestSuite() = default;
    ~DynFunctionErrorInjectionTestSuite() override {
        celix_ei_expect_fmemopen(nullptr, 0, nullptr);
        celix_ei_expect_ffi_prep_cif(nullptr, 0, FFI_OK);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(DynFunctionErrorInjectionTestSuite, ParseError) {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    celix_ei_expect_calloc((void*)dynFunction_parse, 0, nullptr);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    EXPECT_STREQ("Error allocating memory for dyn function", celix_err_popLastError());

    celix_ei_expect_ffi_prep_cif((void*)dynFunction_parse, 1, FFI_BAD_TYPEDEF);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    EXPECT_STREQ("Error initializing cif 1", celix_err_popLastError());

    celix_ei_expect_fmemopen((void*)dynFunction_parseWithStr, 0, nullptr);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    std::string result = "Error creating mem stream for descriptor string. ";
    result += strerror(ENOMEM);
    EXPECT_STREQ(result.c_str(), celix_err_popLastError());
}