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

#ifndef CELIX_UV_CLEANUP_H
#define CELIX_UV_CLEANUP_H

#include <uv.h>

#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(uv_thread_t, uv_thread_join)
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(uv_mutex_t, uv_mutex_destroy)
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(uv_rwlock_t, uv_rwlock_destroy)
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(uv_cond_t, uv_cond_destroy)
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(uv_key_t, uv_key_delete)

/**
 * @brief Lock guard for uv mutexes.
 */
typedef struct celix_uv_mutex_lock_guard {
    uv_mutex_t* mutex;
} celix_uv_mutex_lock_guard_t;

/**
 * @brief Initialize a lock guard for @a mutex.
 *
 * Lock a mutex and return a celix_uv_mutex_lock_guard_t.
 * Unlock with celixUvMutexLockGuard_deinit(). Using uv_mutex_lock() on a mutex
 * while a celix_uv_mutex_lock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a uv_mutex_lock() call.
 * This is intended to be used with celix_auto().
 *
 * @param mutex A mutex to lock.
 * @return An initialized lock guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_uv_mutex_lock_guard_t celixUvMutexLockGuard_init(uv_mutex_t* mutex) {
    celix_uv_mutex_lock_guard_t guard;
    guard.mutex = mutex;
    uv_mutex_lock(mutex);
    return guard;
}

/**
 * @brief Deinitialize a lock guard for a mutex.
 *
 * Unlock the mutex of a guard.
 * No memory is freed, it is equivalent to a uv_mutex_unlock() call.
 *
 * @param guard A celix_uv_mutex_lock_guard_t.
 */
static CELIX_UNUSED inline void celixUvMutexLockGuard_deinit(celix_uv_mutex_lock_guard_t* guard) {
    if (guard->mutex) {
        uv_mutex_unlock(guard->mutex);
        guard->mutex = NULL;
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_uv_mutex_lock_guard_t, celixUvMutexLockGuard_deinit)

/**
 * @brief A RAII style write lock guard for uv_rwlock_t.
 *
 * The lock is obtained in the constructor and released in the destructor.
 * This is intended to be used with celix_auto().
 */
typedef struct celix_uv_rwlock_wlock_guard {
    uv_rwlock_t* lock;
} celix_uv_rwlock_wlock_guard_t;

/**
 * @brief Initialize a write lock guard for @a lock.
 *
 * Obtain a write lock on @a lock and return a celix_uv_rwlock_wlock_guard_t.
 * Unlock with celixUvRwlockWlockGuard_deinit(). Using uv_rwlock_wrunlock()
 * on @lock while a celix_uv_rwlock_wlock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a uv_rwlock_wrlock() call.
 * This is intended to be used with celix_auto().
 *
 * @param lock A read-write lock to lock.
 * @return An initialized write lock guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_uv_rwlock_wlock_guard_t celixUvRwlockWlockGuard_init(uv_rwlock_t* lock) {
    celix_uv_rwlock_wlock_guard_t guard;
    guard.lock = lock;
    uv_rwlock_wrlock(lock);
    return guard;
}

/**
 * @brief Deinitialize a write lock guard.
 *
 * Release a write lock on the read-write lock contained in @a guard.
 * See celixUvRwlockWlockGuard_init() for details.
 * No memory is freed, it is equivalent to a uv_rwlock_wrunlock() call.
 *
 * @param guard A celix_uv_rwlock_wlock_guard_t.
 */
static CELIX_UNUSED inline void celixUvRwlockWlockGuard_deinit(celix_uv_rwlock_wlock_guard_t* guard) {
    if (guard->lock) {
        uv_rwlock_wrunlock(guard->lock);
        guard->lock = NULL;
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_uv_rwlock_wlock_guard_t, celixUvRwlockWlockGuard_deinit)

/**
 * @brief A RAII style read lock guard for uv_rwlock_t.
 *
 * The lock is obtained in the constructor and released in the destructor.
 * This is intended to be used with celix_auto().
 */
typedef struct celix_uv_rwlock_rlock_guard {
    uv_rwlock_t* lock;
} celix_uv_rwlock_rlock_guard_t;

/**
 * @brief Initialize a read lock guard for a lock.
 *
 * Obtain a read lock on a lock and return a celix_uv_rwlock_rlock_guard_t.
 * Unlock with celixUvRwlockRlockGuard_deinit(). Using uv_rwlock_rdunlock()
 * on @lock while a celix_uv_rwlock_rlock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a uv_rwlock_rdlock() call.
 * This is intended to be used with celix_auto().
 *
 * @param lock A read-write lock to lock.
 * @return A guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_uv_rwlock_rlock_guard_t celixUvRwlockRlockGuard_init(uv_rwlock_t* lock) {
    celix_uv_rwlock_rlock_guard_t guard;
    guard.lock = lock;
    uv_rwlock_rdlock(lock);
    return guard;
}

/**
 * @brief Deinitialize a read lock guard.
 *
 * Release a read lock on the read-write lock contained in a guard.
 * See celixUvRwlockRlockGuard_init() for details.
 * No memory is freed, it is equivalent to a uv_rwlock_rdunlock() call.
 *
 * @param guard A celix_uv_rwlock_rlock_guard_t.
 */
static CELIX_UNUSED inline void celixUvRwlockRlockGuard_deinit(celix_uv_rwlock_rlock_guard_t* guard) {
    if (guard->lock) {
        uv_rwlock_rdunlock(guard->lock);
        guard->lock = NULL;
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_uv_rwlock_rlock_guard_t, celixUvRwlockRlockGuard_deinit)

#ifdef __cplusplus
}
#endif

#endif /* CELIX_UV_CLEANUP_H */
