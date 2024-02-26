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

#include <assert.h>
#include <string.h>
#include "dyn_example_functions.h"

int32_t example1(int32_t a, int32_t b, int32_t c) {
    return a + b + c;
}

double example2(int32_t arg1, struct example2_arg arg2, double arg3) {
    return 0.0 + arg1 + arg2.val1 + arg2.val2 + arg2.val3 + arg3;
}

void example4Func(struct tst_seq seq) {
    assert(4 == seq.cap);
    assert(2 == seq.len);
    assert(1.1 == seq.buf[0]);
    assert(2.2 == seq.buf[1]);
}

int testExample3(void *ptr, double a, double *out) {
    double *b = (double *)ptr;
    *out = *b * a;
    return 0;
}

void example5Func(const char *s1, char *s2) {
    assert(strncmp("s1", s1, 5) == 0);
    assert(strncmp("s2", s2, 5) == 0);
}

int32_t example6Func() {
    return 1234;
}