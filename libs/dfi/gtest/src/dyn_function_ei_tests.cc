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
#include "asprintf_ei.h"
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
        celix_ei_expect_asprintf(nullptr, 0, -1);
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
    EXPECT_STREQ("Error ffi_prep_cif 1", celix_err_popLastError());

    celix_ei_expect_calloc((void*)dynFunction_parse, 1, nullptr, 4);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    EXPECT_STREQ("Error allocating memory for ffi args", celix_err_popLastError());

    celix_ei_expect_fmemopen((void*)dynFunction_parseWithStr, 0, nullptr);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    std::string result = "Error creating mem stream for descriptor string. ";
    result += strerror(ENOMEM);
    EXPECT_STREQ(result.c_str(), celix_err_popLastError());

    celix_ei_expect_asprintf((void*) dynFunction_parse, 1, -1, 3);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Error allocating argument name", celix_err_popLastError());


    celix_ei_expect_calloc((void*)dynFunction_parse, 1, nullptr, 3);
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Error allocating arg", celix_err_popLastError());
}

class DynClosureErrorInjectionTestSuite : public ::testing::Test {
public:
    DynClosureErrorInjectionTestSuite() = default;
    ~DynClosureErrorInjectionTestSuite() override {
        celix_ei_expect_ffi_prep_closure_loc(nullptr, 0, FFI_OK);
        celix_ei_expect_ffi_closure_alloc(nullptr, 0, nullptr);
    }
};


static void example1_binding(void*, void* args[], void *out) {
    int32_t a = *((int32_t *)args[0]);
    int32_t b = *((int32_t *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    int32_t *ret = (int32_t *)out;
    *ret = a + b + c;
}

TEST_F(DynClosureErrorInjectionTestSuite, CreatError) {
    celix_autoptr(dyn_function_type) dynFunc = nullptr;
    int rc;
    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    int32_t (*func)(int32_t a, int32_t b, int32_t c) = NULL;
    celix_ei_expect_ffi_closure_alloc((void*)dynFunction_createClosure, 0, nullptr);
    rc = dynFunction_createClosure(dynFunc, example1_binding, NULL, (void(**)(void))&func);
    ASSERT_NE(0, rc);
    EXPECT_NE(0, dynFunction_getFnPointer(dynFunc, (void(**)(void))&func));

    celix_ei_expect_ffi_prep_closure_loc((void*)dynFunction_createClosure, 0, FFI_BAD_ABI);
    rc = dynFunction_createClosure(dynFunc, example1_binding, NULL, (void(**)(void))&func);
    ASSERT_NE(0, rc);
    EXPECT_NE(0, dynFunction_getFnPointer(dynFunc, (void(**)(void))&func));
}