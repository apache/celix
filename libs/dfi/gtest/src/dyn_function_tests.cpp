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

TEST_F(DynFunctionTests, DynFuncTest1) {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    void (*fp)(void) = (void (*)(void)) example1;

    rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    int32_t a = 2;
    int32_t b = 4;
    int32_t c = 8;
    void *values[3];
    int32_t rVal = 0;
    values[0] = &a;
    values[1] = &b;
    values[2] = &c;

    rc = dynFunction_call(dynFunc, fp, &rVal, values);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(14, rVal);
    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, DynFuncTest2) {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    void (*fp)(void) = (void (*)(void)) example2;

    rc = dynFunction_parseWithStr(EXAMPLE2_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

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

    rc = dynFunction_call(dynFunc, fp, &returnVal, values);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(19.2, returnVal);
    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, DynFuncAccTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc;
    rc = dynFunction_parseWithStr("add(D{DD a b}*D)V", nullptr, &dynFunc);

    ASSERT_EQ(0, rc);

    int nrOfArgs = dynFunction_nrOfArguments(dynFunc);
    ASSERT_EQ(3, nrOfArgs);

    dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 1);
    ASSERT_TRUE(arg1 != nullptr);
    ASSERT_EQ('{', (char) dynType_descriptorType(arg1));

    dyn_type *nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
    ASSERT_TRUE(nonExist == nullptr);

    dyn_type *returnType = dynFunction_returnType(dynFunc);
    ASSERT_EQ('V', (char) dynType_descriptorType(returnType));

    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, DynFuncTest3) {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) testExample3;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE3_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);
    double result = -1.0;
    double *input = &result;
    double a = 2.0;
    void *ptr = &a;
    void *args[3];
    args[0] = &ptr;
    args[1] = &a;
    args[2] = &input;
    int rVal = 0;
    rc = dynFunction_call(dynFunc, fp, &rVal, args);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(4.0, result);


    double *inMemResult = (double *)calloc(1, sizeof(double));
    a = 2.0;
    ptr = &a;
    args[0] = &ptr;
    args[1] = &a;
    args[2] = &inMemResult;
    rVal = 0;
    rc = dynFunction_call(dynFunc, fp, &rVal, args);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(4.0, result);
    free(inMemResult);

    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, DynFuncTest4) {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example4Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE4_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

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
    ASSERT_EQ(0, rc);

    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, DynFuncTest5) {
    dyn_function_type *dynFunc = nullptr;
    void (*fp)(void) = (void(*)(void)) example5Func;
    int rc;

    rc = dynFunction_parseWithStr(EXAMPLE5_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *a1 = "s1";
    char *a2 = strdup("s2");
    void *args[2];
    args[0] = &a1;
    args[1] = &a2;

    rc = dynFunction_call(dynFunc, fp, nullptr, args);
    ASSERT_EQ(0, rc);

    dynFunction_destroy(dynFunc);
}

TEST_F(DynFunctionTests, InvalidDynFuncTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr(INVALID_FUNC_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(2, rc); //Mem error

    dynFunc = nullptr;
    rc = dynFunction_parseWithStr(INVALID_FUNC_TYPE_DESCRIPTOR, nullptr, &dynFunc);
    ASSERT_EQ(3, rc); //Parse Error
}

