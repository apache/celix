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
        celix_ei_expect_realloc(nullptr, 0, nullptr);
        celix_ei_expect_strdup(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_fmemopen(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
    // delete other constructors and assign operators
    DynTypeErrorInjectionTestSuite(DynTypeErrorInjectionTestSuite const&) = delete;
    DynTypeErrorInjectionTestSuite(DynTypeErrorInjectionTestSuite&&) = delete;
    DynTypeErrorInjectionTestSuite& operator=(DynTypeErrorInjectionTestSuite const&) = delete;
    DynTypeErrorInjectionTestSuite& operator=(DynTypeErrorInjectionTestSuite&&) = delete;
};

TEST_F(DynTypeErrorInjectionTestSuite, ParseComplexTypeErrors) {
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

    // fail to allocate complex_type_entry
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 4, nullptr);
    status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for complex_type_entry", celix_err_popLastError());

    // fail to allocate ffi_type elements
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 4, nullptr, 4);
    status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for ffi_type elements", celix_err_popLastError());

    // fail to allocate complex types
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 4, nullptr, 5);
    status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for complex types", celix_err_popLastError());
}

TEST_F(DynTypeErrorInjectionTestSuite, ParseNestedTypeErrors) {
    dyn_type *type = NULL;
    const char* descriptor = "Tnode={Lnode;Lnode; left right};{Lnode; head}";
    int status = 0;

    // fail to allocate type_entry
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 4, nullptr);
    status = dynType_parseWithStr(descriptor, NULL, NULL, &type);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating entry", celix_err_popLastError());
}

TEST_F(DynTypeErrorInjectionTestSuite, ParseEnumTypeErrors) {
    dyn_type *type = NULL;
    int rc = 0;
    // fail to allocate meta_entry
    celix_ei_expect_calloc((void*)dynType_parseWithStr, 4, nullptr, 1);
    rc = dynType_parseWithStr("#v1=0;#v2=1;E", NULL, NULL, &type);
    ASSERT_NE(0, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeErrorInjectionTestSuite, AllocateErrors) {
    celix_autoptr(dyn_type) type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("#v1=0;#v2=1;E", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    celix_ei_expect_calloc((void*)dynType_alloc, 0, nullptr);
    void* buf = nullptr;
    rc = dynType_alloc(type, &buf);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error allocating memory for type 'E'", celix_err_popLastError());
}

TEST_F(DynTypeErrorInjectionTestSuite, SequenceAllocateError) {
    struct double_sequence {
        uint32_t cap;
        uint32_t len;
        double* buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("[D", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    struct double_sequence *seq = NULL;
    rc = dynType_alloc(type, (void **)&seq);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(seq != NULL);
    celix_ei_expect_calloc((void*)dynType_sequence_alloc, 0, nullptr);
    rc = dynType_sequence_alloc(type, seq, 1);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error allocating memory for buf", celix_err_popLastError());

    dynType_free(type, seq);
    dynType_destroy(type);
}

TEST_F(DynTypeErrorInjectionTestSuite, SequenceReserveError) {
    struct double_sequence {
        uint32_t cap;
        uint32_t len;
        double* buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("[D", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    struct double_sequence *seq = NULL;
    rc = dynType_alloc(type, (void **)&seq);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(seq != NULL);
    celix_ei_expect_realloc((void*)dynType_sequence_reserve, 0, nullptr);
    rc = dynType_sequence_reserve(type, seq, 1);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error allocating memory for buf", celix_err_popLastError());

    dynType_free(type, seq);
    dynType_destroy(type);
}
