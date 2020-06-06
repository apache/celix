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

#include "gtest/gtest.h"

#include <stdarg.h>
#include "dyn_example_functions.h"

#include "dyn_common.h"
#include "dyn_function.h"


static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


#define INVALID_FUNC_DESCRIPTOR "example$[D)V"//$ is an invalid symbol, missing (
#define INVALID_FUNC_TYPE_DESCRIPTOR "example(H)A"//H and A are invalid types


class DynFunctionTests : public ::testing::Test {
public:
    DynFunctionTests() {
        int lvl = 1;
        dynFunction_logSetup(stdLog, nullptr, lvl);
        dynType_logSetup(stdLog, nullptr, lvl);
        dynCommon_logSetup(stdLog, nullptr, lvl);
    }
    ~DynFunctionTests() override = default;

};


extern "C" {
    static bool func_test1() {
        dyn_function_type *dynFunc = nullptr;
        int rc;
        void (*fp)(void) = (void (*)(void)) example1;

        rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);

        int32_t rVal = 0;
        int32_t a = 2;
        int32_t b = 4;
        int32_t c = 8;
        void *values[3];
        values[0] = &a;
        values[1] = &b;
        values[2] = &c;

        if (rc == 0) {
            rc = dynFunction_call(dynFunc, fp, &rVal, values);
            dynFunction_destroy(dynFunc);
        }

        return rc == 0 && rVal == 14;
    }
}

TEST_F(DynFunctionTests, DynFuncTest1) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_test1());
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
    dyn_type *nonExist = nullptr;
    if (rc == 0) {
         nrOfArgs = dynFunction_nrOfArguments(dynFunc);
        dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 1);
        if (arg1 != nullptr) {
            isStruct = '{' == dynType_descriptorType(arg1);
        }
        nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
        dyn_type *returnType = dynFunction_returnType(dynFunc);
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
#ifdef __clang__
[[clang::optnone]]
#else
__attribute__((optimize("0")))
#endif
static bool func_test3() {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) testExample3;
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
        int rVal = 0;

        rc = dynFunction_call(dynFunc, fp, &rVal, args);

        double *inMemResult = (double *)calloc(1, sizeof(double));
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

extern "C" {
static bool func_test4() {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example4Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE4_DESCRIPTOR, nullptr, &dynFunc);

    double buf[4];
    buf[0] = 1.1;
    buf[1] = 2.2;
    struct tst_seq seq;
    seq.cap = 4;
    seq.len = 2;
    seq.buf = buf;

    void *args[1];
    args[0] = &seq;
    if (rc == 0) {
        rc = dynFunction_call(dynFunc, fp, nullptr, args);
        dynFunction_destroy(dynFunc);
    }

    return rc == 0;
}
}

TEST_F(DynFunctionTests, DynFuncTest4) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_test4());
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

extern "C" {
static bool func_invalid() {
    dyn_function_type *dynFunc = nullptr;
    int rc1 = dynFunction_parseWithStr(INVALID_FUNC_DESCRIPTOR, nullptr, &dynFunc);

    dynFunc = nullptr;
    int rc2 = dynFunction_parseWithStr(INVALID_FUNC_TYPE_DESCRIPTOR, nullptr, &dynFunc);
    return rc1 != 0 && rc2 != 0;
}
}

TEST_F(DynFunctionTests, InvalidDynFuncTest) {
    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
    //corrupted memory. Note that libffi is a function for interfacing with C not C++
    EXPECT_TRUE(func_invalid());
}

