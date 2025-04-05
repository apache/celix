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


#include "dyn_example_functions.h"
#include "dyn_common.h"
#include "dyn_function.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_err.h"

#include <ffi.h>
#include <gtest/gtest.h>

#define INVALID_FUNC_DESCRIPTOR "example$[D)V"//$ is an invalid symbol, missing (
#define INVALID_FUNC_TYPE_DESCRIPTOR "example(H)A"//H and A are invalid types


class DynFunctionTests : public ::testing::Test {
public:
    DynFunctionTests() {
    }
    ~DynFunctionTests() override {
        celix_err_resetErrors();
    }

};

TEST_F(DynFunctionTests, DynFuncTest1) {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    void (*fp)(void) = (void (*)(void)) example1;

    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    EXPECT_TRUE(dynFunction_hasReturn(dynFunc));
    EXPECT_EQ(3, dynFunction_nrOfArguments(dynFunc));
    EXPECT_STREQ("example", dynFunction_getName(dynFunc));
    auto args = dynFunction_arguments(dynFunc);
    dyn_function_argument_type* arg = NULL;
    TAILQ_FOREACH(arg, args, entries) {
        EXPECT_EQ(DYN_FUNCTION_ARGUMENT_META__STD, arg->argumentMeta);
        EXPECT_EQ('I', dynType_descriptorType(arg->type));
    }

    ffi_sarg rVal = 0;
    int32_t a = 2;
    int32_t b = 4;
    int32_t c = 8;
    void *values[3];
    values[0] = &a;
    values[1] = &b;
    values[2] = &c;

    rc = dynFunction_call(dynFunc, fp, &rVal, values);
    dynFunction_destroy(dynFunc);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(14, rVal);
}

extern "C" {
static bool func_test2() {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    void (*fp)(void) = (void (*)(void)) example2;

    rc = dynFunction_parseWithStr(EXAMPLE2_DESCRIPTOR, nullptr, &dynFunc);

    int32_t arg1 = 2;
    struct example2_arg arg2;
    arg2.val1 = 2;
    arg2.val2 = 3;
    arg2.val3 = 4.1;
    double arg3 = 8.1;
    double returnVal = 0;
    void *values[3];
    values[0] = &arg1;
    values[1] = &arg2;
    values[2] = &arg3;

    if (rc == 0) {
        rc = dynFunction_call(dynFunc, fp, &returnVal, values);
        dynFunction_destroy(dynFunc);
    }

    return rc == 0 && returnVal == 19.2;
}
}

TEST_F(DynFunctionTests, DynFuncTest2) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_test2());
}

extern "C" {
static bool func_acc() {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    rc = dynFunction_parseWithStr("add(D{DD a b}*D)V", nullptr, &dynFunc);

    int nrOfArgs = 0;
    bool isStruct = false;
    bool isVoid = false;
    const dyn_type *nonExist = nullptr;
    if (rc == 0) {
         nrOfArgs = dynFunction_nrOfArguments(dynFunc);
        const dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 1);
        if (arg1 != nullptr) {
            isStruct = '{' == dynType_descriptorType(arg1);
        }
        nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
        const dyn_type* returnType = dynFunction_returnType(dynFunc);
        if (returnType != nullptr) {
            isVoid = 'V' == dynType_descriptorType(returnType);
        }
        dynFunction_destroy(dynFunc);
    }

    return rc == 0 && nrOfArgs == 3 && isStruct && isVoid && nonExist == nullptr;
}
}

TEST_F(DynFunctionTests, DynFuncAccTest) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_acc());
}

extern "C" {
static bool func_test3() {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void (*)(void)) testExample3;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE3_DESCRIPTOR, nullptr, &dynFunc);
    double result1 = -1.0;
    double result2 = -1.0;

    if (rc == 0) {
        double *input = &result1;
        double a = 2.0;
        void *ptr = &a;
        void *args[3];
        args[0] = &ptr;
        args[1] = &a;
        args[2] = &input;
        ffi_sarg rVal = 0;

        rc = dynFunction_call(dynFunc, fp, &rVal, args);

        double *inMemResult = (double *) calloc(1, sizeof(double));
        a = 2.0;
        ptr = &a;
        args[0] = &ptr;
        args[1] = &a;
        args[2] = &inMemResult;
        rVal = 0;
        if (rc == 0) {
            rc = dynFunction_call(dynFunc, fp, &rVal, args);
        }
        result2 = *inMemResult;
        free(inMemResult);

        dynFunction_destroy(dynFunc);
    }

    return rc == 0 && result1 == 4.0 && result2 == 4.0;
}
}

TEST_F(DynFunctionTests, DynFuncTest3) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_test3());
}

TEST_F(DynFunctionTests, DynFuncTest4) {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example4Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE4_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynFunction_hasReturn(dynFunc));
    EXPECT_EQ(1, dynFunction_nrOfArguments(dynFunc));

    double buf[4];
    buf[0] = 1.1;
    buf[1] = 2.2;
    struct tst_seq seq;
    seq.cap = 4;
    seq.len = 2;
    seq.buf = buf;

    void *args[1];
    args[0] = &seq;
    rc = dynFunction_call(dynFunc, fp, nullptr, args);
    dynFunction_destroy(dynFunc);
    EXPECT_EQ(0, rc);
}

extern "C" {
static bool func_test5() {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example5Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE5_DESCRIPTOR, nullptr, &dynFunc);

    const char *a1 = "s1";
    char *a2 = strdup("s2");
    void *args[2];
    args[0] = &a1;
    args[1] = &a2;

    if (rc == 0) {
        rc = dynFunction_call(dynFunc, fp, nullptr, args);
        dynFunction_destroy(dynFunc);
    }

    free(a2);

    return rc == 0;
}
}

TEST_F(DynFunctionTests, DynFuncTest5) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_test5());
}

TEST_F(DynFunctionTests, DynFuncTest6) {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example6Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE6_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    EXPECT_TRUE(dynFunction_hasReturn(dynFunc));
    EXPECT_EQ(0, dynFunction_nrOfArguments(dynFunc));
    auto args = dynFunction_arguments(dynFunc);
    EXPECT_TRUE(TAILQ_EMPTY(args));

    ffi_sarg rVal = 0;
    rc = dynFunction_call(dynFunc, fp, &rVal, nullptr);
    dynFunction_destroy(dynFunc);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(1234, rVal);
}

TEST_F(DynFunctionTests, InvalidDynFuncTest) {

    dyn_function_type *dynFunc = nullptr;
    int rc1 = dynFunction_parseWithStr(INVALID_FUNC_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc1);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Error parsing, expected token '(' got '$' at position 8", celix_err_popLastError());

    dynFunc = nullptr;
    int rc2 = dynFunction_parseWithStr(INVALID_FUNC_TYPE_DESCRIPTOR, nullptr, &dynFunc);
    EXPECT_NE(0, rc2);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Error unsupported type 'H'", celix_err_popLastError());

    dynFunc = nullptr;
    int rc3 = dynFunction_parseWithStr("$xample(III)I", nullptr, &dynFunc);
    EXPECT_NE(0, rc3);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Parsed empty name", celix_err_popLastError());

    dynFunc = nullptr;
    int rc4 = dynFunction_parseWithStr("example(III", nullptr, &dynFunc);
    EXPECT_NE(0, rc4);
    EXPECT_STREQ("Error parsing descriptor", celix_err_popLastError());
    EXPECT_STREQ("Error missing ')'", celix_err_popLastError());
}

TEST_F(DynFunctionTests, WrongArgumentMetaTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc1 = dynFunction_parseWithStr("example(#am=handle;tt)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc1);
    EXPECT_STREQ("Error 'handle' is only allowed for untyped pointer not 't'", celix_err_popLastError());

    int rc2 = dynFunction_parseWithStr("example(#am=handle;THandle=P;lHandle;t)N", nullptr, &dynFunc);
    EXPECT_EQ(0, rc2);
    dynFunction_destroy(dynFunc);

    int rc3 = dynFunction_parseWithStr("example(#am=handle;P#am=out;t)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc3);
    EXPECT_STREQ("Error 'out' is only allowed for typed pointer not 't'", celix_err_popLastError());

    int rc4 = dynFunction_parseWithStr("example(#am=handle;P#am=out;*D)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc4);
    EXPECT_STREQ("Error 'out' is only allowed for pointer to text or built-in object or typed pointer not to 'D'", celix_err_popLastError());

    // #am=pre argument is not allowed for non pointer types
    int rc5 = dynFunction_parseWithStr("example(#am=pre;I)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc5);
    EXPECT_STREQ("Error 'pre' is only allowed for typed pointer not 'I'", celix_err_popLastError());

    // #am=pre argument is not allowed for pointer to nontrivial types
    int rc6 = dynFunction_parseWithStr("example(#am=pre;**D)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc6);
    EXPECT_STREQ("Error 'pre' is only allowed for pointer to trivial types not non-trivial '*'", celix_err_popLastError());

    int rc7 = dynFunction_parseWithStr("example(#am=handle;P#am=out;p)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc7);
    EXPECT_STREQ("Error 'out' is only allowed for typed pointer not 'p'", celix_err_popLastError());

    int rc8 = dynFunction_parseWithStr("example(#am=handle;P#am=out;a)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc8);
    EXPECT_STREQ("Error 'out' is only allowed for typed pointer not 'a'", celix_err_popLastError());

    int rc9 = dynFunction_parseWithStr("example(#am=handle;P#am=pre;p)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc9);
    EXPECT_STREQ("Error 'pre' is only allowed for typed pointer not 'p'", celix_err_popLastError());

    int rc10 = dynFunction_parseWithStr("example(#am=handle;P#am=pre;a)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc10);
    EXPECT_STREQ("Error 'pre' is only allowed for typed pointer not 'a'", celix_err_popLastError());

    int rc11 = dynFunction_parseWithStr("example(#am=handle;P#am=pre;*p)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc11);
    EXPECT_STREQ("Error 'pre' is only allowed for pointer to trivial types not non-trivial 'p'", celix_err_popLastError());

    int rc12 = dynFunction_parseWithStr("example(#am=handle;P#am=pre;*a)N", nullptr, &dynFunc);
    EXPECT_NE(0, rc12);
    EXPECT_STREQ("Error 'pre' is only allowed for pointer to trivial types not non-trivial 'a'", celix_err_popLastError());
}

TEST_F(DynFunctionTests, DynFuncWithPropertiesArgTest) {
    dyn_function_type *dynFunc = nullptr;

    void (*fp)(const celix_properties_t *constProps, celix_properties_t *props, celix_properties_t **out) =
            [](const celix_properties_t *constProps, celix_properties_t *props, celix_properties_t **out) {
                EXPECT_TRUE(celix_properties_get(constProps, "key1", nullptr) != nullptr);
                *out = celix_properties_copy(props);
                celix_properties_destroy(props);
                return;
            };
    int rc = dynFunction_parseWithStr("example(#const=true;pp#am=out;*p)V", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    EXPECT_EQ(3, dynFunction_nrOfArguments(dynFunc));

    celix_autoptr(celix_properties_t) constProps = celix_properties_create();//owner is caller
    celix_properties_set(constProps, "key1", "value1");
    celix_properties_t* props = celix_properties_create();//owner is callee
    celix_properties_set(props, "key2", "value2");
    void *args[3];
    args[0] = &constProps;
    args[1] = &props;
    celix_autoptr(celix_properties_t) result = nullptr;
    celix_properties_t** ptrToResult = &result;
    args[2] = &ptrToResult;
    rc = dynFunction_call(dynFunc, (void (*)(void))fp, nullptr, args);
    EXPECT_EQ(0, rc);
    dynFunction_destroy(dynFunc);
    EXPECT_TRUE(celix_properties_get(result, "key2", nullptr) != nullptr);
}

TEST_F(DynFunctionTests, DynFuncWithArrayListArgTest) {
    dyn_function_type *dynFunc = nullptr;

    void (*fp) (const celix_array_list_t* constProps, celix_array_list_t* props, celix_array_list_t** out) =
            [](const celix_array_list_t* constProps, celix_array_list_t* props, celix_array_list_t** out) {
                EXPECT_EQ(1, celix_arrayList_getLong(constProps, 0));
                *out = celix_arrayList_copy(props);
                celix_arrayList_destroy(props);
                return;
            };
    int rc = dynFunction_parseWithStr("example(#const=true;aa#am=out;*a)V", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    EXPECT_EQ(3, dynFunction_nrOfArguments(dynFunc));

    celix_autoptr(celix_array_list_t) constList = celix_arrayList_createLongArray();//owner is caller
    celix_arrayList_addLong(constList, 1);
    celix_array_list_t* list = celix_arrayList_createLongArray();//owner is callee
    celix_arrayList_addLong(list, 2);
    void *args[3];
    args[0] = &constList;
    args[1] = &list;
    celix_autoptr(celix_array_list_t) result = nullptr;
    celix_array_list_t** ptrToResult = &result;
    args[2] = &ptrToResult;
    rc = dynFunction_call(dynFunc, (void (*)(void))fp, nullptr, args);
    EXPECT_EQ(0, rc);
    dynFunction_destroy(dynFunc);
    EXPECT_EQ(2, celix_arrayList_getLong(result, 0));
}
