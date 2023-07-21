//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.

#ifndef CELIX_CELIX_STDLIB_CLEANUP_H
#define CELIX_CELIX_STDLIB_CLEANUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_cleanup.h"
#include <stdlib.h>


static __attribute__((__unused__)) inline void celix_autoptr_cleanup_generic_free(void* p) {
    void** pp = (void**)p;
    free(*pp);
}

/**
 * @brief Macro to add an attribute to pointer variable to ensure automatic
 * cleanup using free().
 *
 * This macro differs from celix_autoptr() in that it is an attribute supplied
 * before the type name, rather than wrapping the type definition.  Instead
 * of using a type-specific lookup, this macro always calls free() directly.
 *
 * This means it's useful for any type that is returned from malloc().
 */
#define celix_autofree __attribute__((__unused__)) _CELIX_CLEANUP(celix_autoptr_cleanup_generic_free)

#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_STDLIB_CLEANUP_H
