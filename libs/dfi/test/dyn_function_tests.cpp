/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>


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

    #define EXAMPLE1_DESCRIPTOR "example(III)I"
    int32_t example1(int32_t a, int32_t b, int32_t c) {
        CHECK_EQUAL(2, a);
        CHECK_EQUAL(4, b);
        CHECK_EQUAL(8, c);
        return 1;
    }

    void test_example1(void) {
        dyn_function_type *dynFunc = NULL;
        int rc;
        void (*fp)(void) = (void (*)(void)) example1;

        rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

        int32_t a = 2;
        int32_t b = 4;
        int32_t c = 8;
        void *values[3];
        int32_t rVal = 0;
        values[0] = &a;
        values[1] = &b;
        values[2] = &c;

        rc = dynFunction_call(dynFunc, fp, &rVal, values);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(1, rVal);
        dynFunction_destroy(dynFunc);
    }

    #define EXAMPLE2_DESCRIPTOR "example(I{IID val1 val2 val3}D)D"
    struct example2_arg {
        int32_t val1;
        int32_t val2;
        double val3;
    };

    double example2(int32_t arg1, struct example2_arg arg2, double arg3) {
        CHECK_EQUAL(2, arg1);
        CHECK_EQUAL(2, arg2.val1);
        CHECK_EQUAL(3, arg2.val2);
        CHECK_EQUAL(4.1, arg2.val3);
        CHECK_EQUAL(8.1, arg3);
        return 2.2;
    }

    void test_example2(void) {
        dyn_function_type *dynFunc = NULL;
        int rc;
        void (*fp)(void) = (void (*)(void)) example2;

        rc = dynFunction_parseWithStr(EXAMPLE2_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

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
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(2.2, returnVal);
        dynFunction_destroy(dynFunc);
    }

    static void test_access_functions(void) {
        dyn_function_type *dynFunc = NULL;
        int rc;
        rc = dynFunction_parseWithStr("add(D{DD a b}*D)V", NULL, &dynFunc);

        CHECK_EQUAL(0, rc);

        int nrOfArgs = dynFunction_nrOfArguments(dynFunc);
        CHECK_EQUAL(3, nrOfArgs);

        dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 1);
        CHECK(arg1 != NULL);
        CHECK_EQUAL('{', (char) dynType_descriptorType(arg1));

        dyn_type *nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
        CHECK(nonExist == NULL);

        dyn_type *returnType = dynFunction_returnType(dynFunc);
        CHECK_EQUAL('V', (char) dynType_descriptorType(returnType));

        dynFunction_destroy(dynFunc);
    }

    //example with gen pointer and output
    #define EXAMPLE3_DESCRIPTOR "example(PD*D)N"

    static int testExample3(void *ptr, double a, double *out) {
        double *b = (double *)ptr;
        CHECK_EQUAL(2.0, *b)
        CHECK_EQUAL(a, 2.0);
        *out = *b * a;
        return 0;
    }

    static void test_example3(void) {
        dyn_function_type *dynFunc = NULL;
        void (*fp)(void) = (void(*)(void)) testExample3;
        int rc;

        rc = dynFunction_parseWithStr(EXAMPLE3_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(0, rc);
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
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(4.0, result);


        double *inMemResult = (double *)calloc(1, sizeof(double));
        a = 2.0;
        ptr = &a;
        args[0] = &ptr;
        args[1] = &a;
        args[2] = &inMemResult;
        rVal = 0;
        rc = dynFunction_call(dynFunc, fp, &rVal, args);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(4.0, result);
        free(inMemResult);

        dynFunction_destroy(dynFunc);
    }

    struct tst_seq {
        uint32_t cap;
        uint32_t len;
        double *buf;
    };

    #define EXAMPLE4_DESCRIPTOR "example([D)V"

    static void example4Func(struct tst_seq seq) {
        CHECK_EQUAL(4, seq.cap);
        CHECK_EQUAL(2, seq.len);
        CHECK_EQUAL(1.1, seq.buf[0]);
        CHECK_EQUAL(2.2, seq.buf[1]);
    }

    static void test_example4(void) {
        dyn_function_type *dynFunc = NULL;
        void (*fp)(void) = (void(*)(void)) example4Func;
        int rc;

        rc = dynFunction_parseWithStr(EXAMPLE4_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

        double buf[4];
        buf[0] = 1.1;
        buf[1] = 2.2;
        struct tst_seq seq;
        seq.cap = 4;
        seq.len = 2;
        seq.buf = buf;

        void *args[1];
        args[0] = &seq;
        rc = dynFunction_call(dynFunc, fp, NULL, args);
        CHECK_EQUAL(0, rc);

        dynFunction_destroy(dynFunc);
    }

    #define INVALID_FUNC_DESCRIPTOR "example$[D)V"//$ is an invalid symbol, missing (

    static void test_invalidDynFunc(void) {
        dyn_function_type *dynFunc = NULL;
        int rc = dynFunction_parseWithStr(INVALID_FUNC_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(2, rc); //Mem error
    }

    #define INVALID_FUNC_TYPE_DESCRIPTOR "example(H)A"//H and A are invalid types

    static void test_invalidDynFuncType(void) {
        dyn_function_type *dynFunc = NULL;
        int rc = dynFunction_parseWithStr(INVALID_FUNC_TYPE_DESCRIPTOR, NULL, &dynFunc);
        CHECK_EQUAL(3, rc); //Parse Error
    }
}

TEST_GROUP(DynFunctionTests) {
    void setup() {
        int lvl = 1;
        dynFunction_logSetup(stdLog, NULL, lvl);
        dynType_logSetup(stdLog, NULL, lvl);
        dynCommon_logSetup(stdLog, NULL, lvl);
    }
};

TEST(DynFunctionTests, DynFuncTest1) {
    test_example1();
}

TEST(DynFunctionTests, DynFuncTest2) {
    test_example2();
}

TEST(DynFunctionTests, DynFuncAccTest) {
    test_access_functions();
}

TEST(DynFunctionTests, DynFuncTest3) {
    test_example3();
}

TEST(DynFunctionTests, DynFuncTest4) {
    test_example4();
}

TEST(DynFunctionTests, InvalidDynFuncTest) {
    test_invalidDynFunc();
    test_invalidDynFuncType();
}
