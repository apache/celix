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
//TODO: Move it to libs/error_injector and use conan to import thpool
#include "thpool_ei.h"

extern "C" {
struct thpool_* __real_thpool_init(int num_threads);
CELIX_EI_DEFINE(thpool_init, struct thpool_*)
struct thpool_* __wrap_thpool_init(int num_threads) {
    CELIX_EI_IMPL(thpool_init);
    return __real_thpool_init(num_threads);
}

int __real_thpool_add_work(struct thpool_* thpool_p, void (*function_p)(void*), void* arg_p);
CELIX_EI_DEFINE(thpool_add_work, int)
int __wrap_thpool_add_work(struct thpool_* thpool_p, void (*function_p)(void*), void* arg_p) {
    CELIX_EI_IMPL(thpool_add_work);
    return __real_thpool_add_work(thpool_p, function_p, arg_p);
}

}