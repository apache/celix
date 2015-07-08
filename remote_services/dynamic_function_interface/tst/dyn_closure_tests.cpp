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

static int g_count;

static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
}

#define EXAMPLE1_DESCRIPTOR "example(III)I"
static void example1_binding(void *userData, void* args[], void *out) {
    printf("example1 closure called\n");
    int32_t a = *((int32_t *)args[0]);
    int32_t b = *((int32_t *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    int32_t *ret = (int32_t *)out;
    *ret = a + b + c;
    g_count += 1;
}

#define EXAMPLE2_DESCRIPTOR "example(I{DDD val1 val2 val3}I)D"
struct example2_arg2 {
    double val1;
    double val2;
    double val3;
};
void example2_binding(void *userData, void* args[], void *out) {
    printf("example2 closure called\n");
    int32_t a = *((int32_t *)args[0]);
    struct example2_arg2 b =  *((struct example2_arg2 *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    double *ret = (double *)out;
    *ret = a + b.val1 + b.val2 + b.val3 + c;
    g_count += 1;
}


#define EXAMPLE3_DESCRIPTOR "example(III){III sum max min}"
struct example3_ret {
    int32_t sum;
    int32_t max;
    int32_t min;
};

static void example3_binding(void *userData, void* args[], void *out) {
    printf("example closure called\n");
    int32_t a = *((int32_t *)args[0]);
    int32_t b = *((int32_t *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    struct example3_ret *result = (struct example3_ret *)calloc(1,sizeof(struct example3_ret));
    result->sum = a + b + c;
    result->min = a <= b ? a : b;
    result->max = a >= b ? a : b;
    result->min = result->min <= c ? result->min : c;
    result->max = result->max >= c ? result->max : c;

    struct example3_ret **ret = (struct example3_ret **)out;
    (*ret) = result;
    g_count += 1;
}

static void tests() {
    dyn_closure_type *dynClosure = NULL;
    int rc = 0;

    {
        rc = dynClosure_create(EXAMPLE1_DESCRIPTOR, NULL, example1_binding, NULL, &dynClosure);
        CHECK_EQUAL(0, rc);
        int32_t (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        int rc = dynClosure_getFnPointer(dynClosure, (void(**)(void))&func);
        CHECK_EQUAL(0, rc);
        int32_t ret = func(2,3,4);
        //printf("Return value for example1 is %i\n", ret);
        CHECK_EQUAL(1, g_count);
	CHECK_EQUAL(9, ret);
        dynClosure_destroy(dynClosure);
    }

    {
        dynClosure = NULL;
        rc = dynClosure_create(EXAMPLE2_DESCRIPTOR, NULL, example2_binding, NULL, &dynClosure);
        CHECK_EQUAL(0, rc);
        double (*func)(int32_t a, struct example2_arg2 b, int32_t c) = NULL;
        rc = dynClosure_getFnPointer(dynClosure, (void(**)(void))&func);
        CHECK_EQUAL(0, rc);
        struct example2_arg2 b;
        b.val1 = 1.0;
        b.val2 = 1.5;
        b.val3 = 2.0;
        double ret = func(2,b,4);
        //printf("Return value for example2 is %f\n", ret);
        CHECK_EQUAL(2, g_count);
	CHECK_EQUAL(10.5, ret);
        dynClosure_destroy(dynClosure);
    }

    {
        dynClosure = NULL;
        rc = dynClosure_create(EXAMPLE3_DESCRIPTOR, NULL, example3_binding, NULL, &dynClosure);
        CHECK_EQUAL(0, rc);
        struct example3_ret * (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        rc = dynClosure_getFnPointer(dynClosure, (void(**)(void))&func);
        CHECK_EQUAL(0, rc);
        struct example3_ret *ret = func(2,8,4);
        //printf("Return value for example3 is {sum:%i, max:%i, min:%i}\n", ret->sum, ret->max, ret->min);
        CHECK_EQUAL(3, g_count);
	CHECK_EQUAL(14, ret->sum);
        dynClosure_destroy(dynClosure);
        free(ret);
    }
}

}


TEST_GROUP(DynClosureTests) {
    void setup() {
        dynFunction_logSetup(stdLog, NULL, 4);
        //TODO dynType_logSetup(stdLog, NULL, 4);
        g_count = 0;
    }
};

TEST(DynClosureTests, DynCLosureTest1) {
    //TODO split up
    tests();
}
