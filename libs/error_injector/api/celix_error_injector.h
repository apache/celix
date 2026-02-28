/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <stdio.h>

#ifndef CELIX_ERROR_INJECTOR_H
#define CELIX_ERROR_INJECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <dlfcn.h>
#include <stddef.h>

//NOLINTBEGIN

/**
 * @brief This macro find the address of target function's caller, or caller's caller.
 * The level argument is number of frames to scan up the call stack: 0 means caller, 1 means caller's caller.
 * The result is stored in the addr argument.
 */
#define CELIX_EI_GET_CALLER(addr, level)                                   \
do {                                                                       \
    Dl_info dlinfo;                                                        \
    dladdr(__builtin_return_address(level), &dlinfo);                      \
    (addr) = dlinfo.dli_saddr;                                             \
} while(0)

#define CELIX_EI_UNKNOWN_CALLER ((void *)-1)

#define CELIX_EI_DECLARE(name, ret_type) \
void celix_ei_expect_##name(void *caller, unsigned int level, ret_type ret, size_t ordinal=1)

#define CELIX_EI_DEFINE(name, ret_type)                                                     \
static void *name ## _caller;                                                               \
static unsigned int name ## _caller_level;                                                  \
static ret_type name ## _ret;                                                               \
static size_t name ## _ordinal;                                                             \
                                                                                            \
void celix_ei_expect_##name(void *caller, unsigned int level, ret_type ret, size_t ordinal) \
{                                                                                           \
    name ## _caller = (caller);                                                             \
    name ## _caller_level = (level);                                                        \
    name ## _ret = (ret);                                                                   \
    name ## _ordinal = (ordinal);                                                           \
}

#define CELIX_EI_IMPL(name)                                                                              \
do {                                                                                                     \
    void *addr = CELIX_EI_UNKNOWN_CALLER;                                                                \
    if(name##_caller) {                                                                                  \
        if(name ## _caller != CELIX_EI_UNKNOWN_CALLER) {                                                 \
            /* we can not use CELIX_EI_GET_CALLER(addr, name ## _caller_level) */                        \
            switch(name ## _caller_level) {                                                              \
            case 0:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 0);                                                            \
                break;                                                                                   \
            case 1:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 1);                                                            \
                break;                                                                                   \
            case 2:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 2);                                                            \
                break;                                                                                   \
            case 3:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 3);                                                            \
                break;                                                                                   \
            case 4:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 4);                                                            \
                break;                                                                                   \
            case 5:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 5);                                                            \
                break;                                                                                   \
            case 6:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 6);                                                            \
                break;                                                                                   \
            case 7:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 7);                                                            \
                break;                                                                                   \
            case 8:                                                                                      \
                CELIX_EI_GET_CALLER(addr, 8);                                                            \
                break;                                                                                   \
            default:                                                                                     \
                assert(0);                                                                               \
            }                                                                                            \
        }                                                                                                \
        if(name ## _caller == addr) {                                                                    \
            if(__atomic_fetch_sub(&(name ## _ordinal), 1, __ATOMIC_RELAXED) == 1)                        \
                return name ## _ret;                                                                     \
        }                                                                                                \
    }                                                                                                    \
} while(0)

//NOLINTEND

#ifdef __cplusplus
}
#endif
#endif //CELIX_ERROR_INJECTOR_H
