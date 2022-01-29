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

#include <limits.h>
#include <assert.h>

struct celix_ref {
    int count;
};

/**
 * @brief Initialize object.
 * @param ref object in question.
 */
static inline void celix_ref_init(struct celix_ref *ref) {
    ref->count = 1;
}

/**
 * @brief Read reference count for object.
 * @param ref object.
 * @return the current reference count.
 */
static inline int celix_ref_read(const struct celix_ref *ref)
{
    return __atomic_load_n(&ref->count, __ATOMIC_RELAXED);
}

/**
 * @brief Increase reference count for object.
 * @param ref object.
 */
static inline void celix_ref_get(struct celix_ref *ref) {
    int val;
    val = __atomic_fetch_add(&(ref->count), 1, __ATOMIC_RELAXED);
    assert(val < INT_MAX && val > 0);
    (void)val;
}

 /**
  * @brief Decrease reference count for object.
  *
  * Decrement the reference count, and if 0, call release().
  *
  * @param ref object.
  * @param release pointer to the function that will clean up the object when the
  * last reference to the object is released.
  * @return 1 if the object was removed, otherwise return 0.
  */
static inline int celix_ref_put(struct celix_ref *ref, void (*release)(struct celix_ref *ref)) {
    int val;
    val = __atomic_fetch_sub(&(ref->count), 1, __ATOMIC_RELEASE);
    if(val == 1) {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        release(ref);
        return 1;
    }
    assert(val > 0);
    return 0;
}

#endif //CELIX_CELIX_REF_H
