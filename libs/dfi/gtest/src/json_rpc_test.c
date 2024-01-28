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

#include "json_rpc_test.h"
#include <assert.h>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int add(void*, double a, double b, double *result) {
    *result = a + b;
    return 0;
}

int stats(void*, struct tst_seq input, struct tst_StatsResult **out) {
    assert(out != NULL);
    assert(*out == NULL);
    double total = 0.0;
    unsigned int count = 0;
    double max = DBL_MIN;
    double min = DBL_MAX;

    unsigned int i;
    for (i = 0; i<input.len; i += 1) {
        total += input.buf[i];
        count += 1;
        if (input.buf[i] > max) {
            max = input.buf[i];
        }
        if (input.buf[i] < min) {
            min = input.buf[i];
        }
    }

    struct tst_StatsResult* result = (struct tst_StatsResult *)(calloc(1, sizeof(struct tst_StatsResult)));
    if(count>0) {
        result->average = total / count;
    }
    result->min = min;
    result->max = max;
    double* buf = (double *)(calloc(input.len, sizeof(double)));
    memcpy(buf, input.buf, input.len * sizeof(double));
    result->input.len = input.len;
    result->input.cap = input.len;
    result->input.buf = buf;

    *out = result;
    return 0;
}
