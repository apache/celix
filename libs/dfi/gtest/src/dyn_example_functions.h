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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE1_DESCRIPTOR "example(III)I"
int32_t example1(int32_t a, int32_t b, int32_t c);

#define EXAMPLE2_DESCRIPTOR "example(I{IID val1 val2 val3}D)D"
struct example2_arg {
    int32_t val1;
    int32_t val2;
    double val3;
};
double example2(int32_t arg1, struct example2_arg arg2, double arg3);

#define EXAMPLE3_DESCRIPTOR "example(PD*D)N"
int testExample3(void *ptr, double a, double *out);

#define EXAMPLE4_DESCRIPTOR "example([D)V"
struct tst_seq {
    uint32_t cap;
    uint32_t len;
    double *buf;
};
void example4Func(struct tst_seq seq);

#define EXAMPLE5_DESCRIPTOR "example(#const=true;tt)V"
void example5Func(const char *s1, char *s2);

#define EXAMPLE6_DESCRIPTOR "example()I"
int32_t example6Func();


#ifdef __cplusplus
}
#endif

