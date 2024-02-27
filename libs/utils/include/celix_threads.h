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

#ifndef CELIX_THREADS_H_
#define CELIX_THREADS_H_

#include <pthread.h>
#include <stdbool.h>

#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct celix_thread {
    bool threadInitialized;
    pthread_t thread;
};

typedef pthread_once_t celix_thread_once_t;
#define CELIX_THREAD_ONCE_INIT PTHREAD_ONCE_INIT

typedef struct celix_thread celix_thread_t;
typedef pthread_attr_t celix_thread_attr_t;

typedef void *(*celix_thread_start_t)(void *);

CELIX_UTILS_EXPORT extern const celix_thread_t celix_thread_default;

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_thread_attr_t, pthread_attr_destroy)

CELIX_UTILS_EXPORT celix_status_t
celixThread_create(celix_thread_t *new_thread, const celix_thread_attr_t *attr, celix_thread_start_t func, void *data);

/**
 * If supported by the platform sets the name of the thread.
 */
CELIX_UTILS_EXPORT void celixThread_setName(celix_thread_t *thread, const char *threadName);

CELIX_UTILS_EXPORT void celixThread_exit(void *exitStatus);

CELIX_UTILS_EXPORT celix_status_t celixThread_detach(celix_thread_t thread);

CELIX_UTILS_EXPORT celix_status_t celixThread_join(celix_thread_t thread, void **status);

CELIX_UTILS_EXPORT celix_status_t celixThread_kill(celix_thread_t thread, int sig);

CELIX_UTILS_EXPORT celix_thread_t celixThread_self(void);

/**
 * Return true - as int - if the threads are equals
 * @param[in] thread1
 * @param[in] thread2
 * @return non-zero if the thread IDs t1 and t2 correspond to the same thread, otherwise it will return zero.
 */
CELIX_UTILS_EXPORT int celixThread_equals(celix_thread_t thread1, celix_thread_t thread2);

CELIX_UTILS_EXPORT bool celixThread_initialized(celix_thread_t thread);


typedef pthread_mutex_t celix_thread_mutex_t;
typedef pthread_mutexattr_t celix_thread_mutexattr_t;

#define CELIX_THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

//MUTEX TYPES
enum {
    CELIX_THREAD_MUTEX_NORMAL,
    CELIX_THREAD_MUTEX_RECURSIVE,
    CELIX_THREAD_MUTEX_ERRORCHECK,
    CELIX_THREAD_MUTEX_DEFAULT
};


CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_thread_mutex_t, celixThreadMutex_destroy)

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_tryLock(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex);

/**
 * @brief Lock guard for mutex.
 */
typedef struct celix_mutex_lock_guard {
    celix_thread_mutex_t *mutex;
} celix_mutex_lock_guard_t;

/**
 * @brief Initialize a lock guard for @a mutex.
 *
 * Lock @a mutex and return a celix_mutex_lock_guard_t.
 * Unlock with celixMutexLockGuard_deinit(). Using celixThreadMutex_lock() on @a mutex
 * while a celix_mutex_lock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a celixThreadMutex_lock() call.
 * This is intended to be used with celix_auto().
 *
 * @param mutex A mutex to lock.
 * @return An initialized lock guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_mutex_lock_guard_t celixMutexLockGuard_init(celix_thread_mutex_t* mutex) {
    celix_mutex_lock_guard_t guard;
    guard.mutex = mutex;
    celixThreadMutex_lock(mutex);
    return guard;
}

/**
 * @brief Deinitialize a lock guard for @a mutex.
 *
 * Unlock the mutex of @a guard.
 * No memory is freed, it is equivalent to a celixThreadMutex_unlock() call.
 *
 * @param guard A celix_mutex_lock_guard_t.
 */
static CELIX_UNUSED inline void celixMutexLockGuard_deinit(celix_mutex_lock_guard_t* guard) {
    if (guard->mutex) {
        celixThreadMutex_unlock(guard->mutex);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_mutex_lock_guard_t, celixMutexLockGuard_deinit)

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_create(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_destroy(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_settype(celix_thread_mutexattr_t *attr, int type);

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_thread_mutexattr_t, celixThreadMutexAttr_destroy)

typedef pthread_rwlock_t celix_thread_rwlock_t;
typedef pthread_rwlockattr_t celix_thread_rwlockattr_t;

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_thread_rwlock_t, celixThreadRwlock_destroy)

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_tryReadLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_tryWriteLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock);

/**
 * @brief A RAII style write lock guard for celix_thread_rwlock_t.
 *
 * The lock is obtained in the constructor and released in the destructor.
 * This is intended to be used with celix_auto().
 */
typedef struct celix_rwlock_wlock_guard {
    celix_thread_rwlock_t *lock;
} celix_rwlock_wlock_guard_t;

/**
 * @brief Initialize a write lock guard for @a lock.
 *
 * Obtain a write lock on @a lock and return a celix_rwlock_wlock_guard_t.
 * Unlock with celixRwlockWlockGuard_deinit(). Using celixThreadRwlock_unlock()
 * on @lock while a celix_rwlock_rlock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a celixThreadRwlock_readLock() call.
 * This is intended to be used with celix_auto().
 *
 * @param lock A read-write lock to lock.
 * @return An initialized write lock guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_rwlock_wlock_guard_t celixRwlockWlockGuard_init(celix_thread_rwlock_t* lock) {
    celix_rwlock_wlock_guard_t guard;
    guard.lock = lock;
    celixThreadRwlock_writeLock(lock);
    return guard;
}

/**
 * @brief Deinitialize a write lock guard.
 *
 * Release a write lock on the read-write lock contained in @a guard.
 * See celixRwlockWlockGuard_init() for details.
 * No memory is freed, it is equivalent to a celixThreadRwlock_unlock() call.
 *
 * @param guard A celix_rwlock_wlock_guard_t.
 */
static CELIX_UNUSED inline void celixRwlockWlockGuard_deinit(celix_rwlock_wlock_guard_t* guard) {
    if (guard->lock) {
        celixThreadRwlock_unlock(guard->lock);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_rwlock_wlock_guard_t, celixRwlockWlockGuard_deinit)

/**
 * @brief A RAII style read lock guard for celix_thread_rwlock_t.
 *
 * The lock is obtained in the constructor and released in the destructor.
 * This is intended to be used with celix_auto().
 */
typedef struct celix_rwlock_rlock_guard {
    celix_thread_rwlock_t *lock;
} celix_rwlock_rlock_guard_t;

/**
 * @brief Initialize a read lock guard for @a lock.
 *
 * Obtain a read lock on @a lock and return a celix_rwlock_rlock_guard_t.
 * Unlock with celix_RwlockRlockGuard_deinit(). Using celixThreadRwlock_unlock()
 * on @lock while a celix_rwlock_rlock_guard_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a celixThreadRwlock_readLock() call.
 * This is intended to be used with celix_auto().
 *
 * @param lock A read-write lock to lock.
 * @return A guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_rwlock_rlock_guard_t celixRwlockRlockGuard_init(celix_thread_rwlock_t *lock) {
    celix_rwlock_rlock_guard_t guard;
    guard.lock = lock;
    celixThreadRwlock_readLock(lock);
    return guard;
}

/**
 * @brief Deinitialize a read lock guard.
 *
 * Release a read lock on the read-write lock contained in @a guard.
 * See celixRwlockRlockGuard_init() for details.
 * No memory is freed, it is equivalent to a celixThreadRwlock_unlock() call.
 *
 * @param guard A celix_rwlock_rlock_guard_t.
 */
static CELIX_UNUSED inline void celixRwlockRlockGuard_deinit(celix_rwlock_rlock_guard_t* guard) {
    if (guard->lock) {
        celixThreadRwlock_unlock(guard->lock);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_rwlock_rlock_guard_t, celixRwlockRlockGuard_deinit)

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_create(celix_thread_rwlockattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_destroy(celix_thread_rwlockattr_t *attr);
//NOTE: No support yet for setting specific rw lock attributes

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_thread_rwlockattr_t, celixThreadRwlockAttr_destroy)

typedef pthread_cond_t celix_thread_cond_t;
typedef pthread_condattr_t celix_thread_condattr_t;

/**
 * @brief Initialize the given condition variable.
 *
 * For Linux the condition clock is set to CLOCK_MONOTONIC whether or not the attr is NULL.
 *
 * @param[in] condition The condition variable to initialize.
 * @param[in] attr The condition variable attributes to use. Can be NULL for default attributes.
 * @return CELIX_SUCCESS if the condition variable is initialized successfully.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_thread_cond_t, celixThreadCondition_destroy)

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex);

CELIX_UTILS_DEPRECATED_EXPORT celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds);

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_thread_condattr_t, pthread_condattr_destroy)

/**
 * @brief Get the current time suitable for Celix thread conditions.
 *
 * This function returns the current time compatible with the Celix thread conditions, specifically for
 * the function celixThreadCondition_waitUntil, as long as the condition is initialized with
 * celixThreadCondition_init.
 *
 * Note: Do not use the returned time for logging or displaying the current time as the choice of clock
 * varies based on the operating system.
 *
 * @return A struct timespec denoting the current time.
 */
CELIX_UTILS_EXPORT struct timespec celixThreadCondition_getTime();

/**
 * @brief Calculate the current time incremented by a given delay, suitable for Celix thread conditions.
 *
 * This function provides the current time, increased by a specified delay (in seconds), compatible
 * with Celix thread conditions. The resulting struct timespec can be used with the function
 * celixThreadCondition_waitUntil, as long as the condition is initialized with celixThreadCondition_init.
 *
 * Note: Do not use the returned time for logging or displaying the current time as the choice of clock
 * varies based on the operating system.
 *
 * @param[in] delayInSeconds The desired delay in seconds to be added to the current time.
 * @return A struct timespec denoting the current time plus the provided delay.
 */
CELIX_UTILS_EXPORT struct timespec celixThreadCondition_getDelayedTime(double delayInSeconds);


/**
 * @brief Wait for the condition to be signaled or until the given absolute time is reached.
 * 
 * @section Errors
 * - CELIX_SUCCESS if the condition is signaled before the delayInSeconds is reached.
 * - CELIX_ILLEGAL_ARGUMENT if the absTime is null.
 * - ETIMEDOUT If the absTime has passed.
 * - EINTR Wait was interrupted by a signal.
 *
 *  Values for absTime should be obtained by celixThreadCondition_getTime, celixThreadCondition_getDelayedTime or
 *  a modified timespec based on celixThreadCondition_getTime/celixThreadCondition_getDelayedTime.
 * 
 * @param[in] cond The condition to wait for.
 * @param[in] mutex The (locked) mutex to use.
 * @param[in] absTime The absolute time to wait for the condition to be signaled.
 * @return CELIX_SUCCESS if the condition is signaled before the delayInSeconds is reached.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_waitUntil(celix_thread_cond_t* cond, 
                                                                 celix_thread_mutex_t* mutex, 
                                                                 const struct timespec* absTime);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond);

CELIX_UTILS_EXPORT celix_status_t celixThread_once(celix_thread_once_t *once_control, void (*init_routine)(void));

//Thread Specific Storage (TSS) Abstraction
typedef pthread_key_t celix_tss_key_t;

/**
 * @brief Create a thread specific storage key visible for all threads.
 *
 *
 * Upon creation, the value NULL shall be associated with
 * the new key in all active threads of the process. Upon thread creation,
 * the value NULL shall be associated with all defined keys in the new thread
 *
 * An optional destructor function may be associated with each key value. At thread exit, if a key value has a
 * non-NULL destructor pointer, and the thread has a non-NULL value associated with that key, the value of the key is
 * set to NULL, and then the function pointed to is called with the previously associated value as its sole argument.
 * The order of  destructor calls is unspecified if more than one destructor exists for a thread when it exits.
 *
 * @param key The key to create.
 * @param destroyFunction The function to call when the key is destroyed.
 * @return CELIX_SUCCESS if the key is created successfully.
 *
 * @retval CELIX_ENOMEM if there was insufficient memory for the key creation.
 * @retval CELIX_EAGAIN if the system lacked the necessary resources to create another thread specific data key.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_create(celix_tss_key_t* key, void (*destroyFunction)(void*));

/**
 * @brief Delete a thread specific storage key previously created by celix_tss_create.
 *
 * @param key The key to delete.
 * @return CELIX_SUCCESS if the key is deleted successfully.
 *
 * @retval CELIX_ILLEGAL_ARGUMENT if the key is invalid.
 * @retval CELIX_ILLEGAL_STATE if the key is otherwise not deleted successfully.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_delete(celix_tss_key_t key);

/**
 * @brief Set a thread-specific value for the provide thread specific storage key.
 *
 * @param key The key to set the value for.
 * @param value The thread-specific value to set.
 * @return CELIX_SUCCESS if the value is set successfully.
 *
 * @retval CELIX_ILLEGAL_ARGUMENT if the key is invalid.
 * @retval CELIX_ENOMEM if there was insufficient memory to set the value.
 * @retval CELIX_ILLEGAL_STATE if the value is not set successfully.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_set(celix_tss_key_t key, void* value);

/**
 * @brief Get the thread-specific value for the provided thread specific storage key.
 *
 * @param key The key to get the value for.
 * @return The thread-specific value.
 *
 * @retval NULL if the key is invalid or there is no thread-specific value set for the key.
 */
CELIX_UTILS_EXPORT void* celix_tss_get(celix_tss_key_t key);

#ifdef __cplusplus
}
#endif
#endif /* CELIX_THREADS_H_ */
