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


extern "C" {
    
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dyn_common.h"
#include "dyn_function.h"

static int g_count;

#define EXAMPLE1_DESCRIPTOR "example(III)I"
static void example1_binding(void*, void* args[], void *out) {
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
void example2_binding(void*, void* args[], void *out) {
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

static void example3_binding(void*, void* args[], void *out) {
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
    dyn_function_type *dynFunction = NULL;
    int rc = 0;

    {
        int32_t (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        rc = dynFunction_parseWithStr(EXAMPLE1_DESCRIPTOR, NULL, &dynFunction);
        ASSERT_EQ(0, rc);
        rc = dynFunction_createClosure(dynFunction, example1_binding, NULL, (void(**)(void))&func);
        ASSERT_EQ(0, rc);
        int32_t ret = func(2,3,4);
        ASSERT_EQ(1, g_count);
        ASSERT_EQ(9, ret);
        dynFunction_destroy(dynFunction);
    }

    {
        double (*func)(int32_t a, struct example2_arg2 b, int32_t c) = NULL;
        double (*func2)(int32_t a, struct example2_arg2 b, int32_t c) = NULL;
        dynFunction = NULL;
        rc = dynFunction_parseWithStr(EXAMPLE2_DESCRIPTOR, NULL, &dynFunction);
        ASSERT_EQ(0, rc);
        rc = dynFunction_createClosure(dynFunction, example2_binding, NULL, (void(**)(void))&func);
        ASSERT_EQ(0, rc);
        rc = dynFunction_getFnPointer(dynFunction, (void(**)(void))&func2);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(func == func2);
        struct example2_arg2 b;
        b.val1 = 1.0;
        b.val2 = 1.5;
        b.val3 = 2.0;
        double ret = func(2,b,4);
        ASSERT_EQ(2, g_count);
        ASSERT_EQ(10.5, ret);
        dynFunction_destroy(dynFunction);
    }

    {
        struct example3_ret * (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        dynFunction = NULL;
        rc = dynFunction_parseWithStr(EXAMPLE3_DESCRIPTOR, NULL, &dynFunction);
        ASSERT_EQ(0, rc);
        rc = dynFunction_createClosure(dynFunction, example3_binding, NULL, (void(**)(void))&func);
        ASSERT_EQ(0, rc);
        struct example3_ret *ret = func(2,8,4);
        ASSERT_EQ(3, g_count);
        ASSERT_EQ(14, ret->sum);
        dynFunction_destroy(dynFunction);
        free(ret);
    }
}

}

class DynClosureTests : public ::testing::Test {
public:
    DynClosureTests() {
        g_count = 0;
    }
    ~DynClosureTests() override {
    }

};

TEST_F(DynClosureTests, DynClosureTest1) {
    //TODO split up
    tests();
}
