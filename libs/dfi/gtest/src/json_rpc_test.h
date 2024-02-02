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
#ifndef CELIX_JSON_RPC_TEST_H
#define CELIX_JSON_RPC_TEST_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct tst_seq {
    uint32_t cap;
    uint32_t len;
    double *buf;
};


//StatsResult={DDD[D average min max input}
struct tst_StatsResult {
    double average;
    double min;
    double max;
    struct tst_seq input;
};

int add(void*, double a, double b, double *result);
int stats(void*, struct tst_seq input, struct tst_StatsResult **out);

#ifdef __cplusplus
}
#endif
#endif //CELIX_JSON_RPC_TEST_H
