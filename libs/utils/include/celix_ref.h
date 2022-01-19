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

#ifndef CELIX_CELIX_REF_H
#define CELIX_CELIX_REF_H

#include <stdatomic.h>
#include <limits.h>
#include <assert.h>

struct celix_ref {
    atomic_int count;
};

static inline void celix_ref_init(struct celix_ref *ref) {
    ref->count = 1;
}

static inline void celix_ref_get(struct celix_ref *ref) {
    int val;
    val = atomic_fetch_add_explicit(&(ref->count), 1, memory_order_relaxed);
    assert(val < INT_MAX && val > 0);
    (void)val;
}

static inline int celix_ref_put(struct celix_ref *ref, void (*release)(struct celix_ref *ref)) {
    int val;
    val = atomic_fetch_sub_explicit(&(ref->count), 1, memory_order_release);
    if(val == 1) {
        atomic_thread_fence(memory_order_acquire);
        release(ref);
        return 1;
    }
    assert(val > 0);
    return 0;
}

#endif //CELIX_CELIX_REF_H
