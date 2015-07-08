/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>

    #include "dyn_function.h"

    static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
        va_list ap;
        const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
        fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        fprintf(stderr, "\n");
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

        rc = dynFunction_create(EXAMPLE1_DESCRIPTOR, NULL, fp, &dynFunc);
        CHECK_EQUAL(0, rc);

        int32_t a = 2;
        int32_t b = 4;
        int32_t c = 8;
        void *values[3];
        int32_t rVal = 0;
        values[0] = &a;
        values[1] = &b;
        values[2] = &c;

        rc = dynFunction_call(dynFunc, &rVal, values);
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

        rc = dynFunction_create(EXAMPLE2_DESCRIPTOR, NULL, fp, &dynFunc);
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

        rc = dynFunction_call(dynFunc, &returnVal, values);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(2.2, returnVal);
        dynFunction_destroy(dynFunc);
    }
}

TEST_GROUP(DynFunctionTests) {
    void setup() {
        dynFunction_logSetup(stdLog, NULL, 4);
        //TODO dynType_logSetup(stdLog, NULL, 4);
    }
};

TEST(DynFunctionTests, DynFuncTest1) {
    test_example1();
}

TEST(DynFunctionTests, DynFuncTest2) {
    test_example2();
}
