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

static const celix_thread_t celix_thread_default = {0, 0};

CELIX_UTILS_EXPORT celix_status_t
celixThread_create(celix_thread_t *new_thread, celix_thread_attr_t *attr, celix_thread_start_t func, void *data);

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

//MUTEX TYPES
enum {
    CELIX_THREAD_MUTEX_NORMAL,
    CELIX_THREAD_MUTEX_RECURSIVE,
    CELIX_THREAD_MUTEX_ERRORCHECK,
    CELIX_THREAD_MUTEX_DEFAULT
};


CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_create(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_destroy(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_settype(celix_thread_mutexattr_t *attr, int type);

typedef pthread_rwlock_t celix_thread_rwlock_t;
typedef pthread_rwlockattr_t celix_thread_rwlockattr_t;

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_create(celix_thread_rwlockattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_destroy(celix_thread_rwlockattr_t *attr);
//NOTE: No support yet for setting specific rw lock attributes


typedef pthread_cond_t celix_thread_cond_t;
typedef pthread_condattr_t celix_thread_condattr_t;

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond);

CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond);

CELIX_UTILS_EXPORT celix_status_t celixThread_once(celix_thread_once_t *once_control, void (*init_routine)(void));

#ifdef __cplusplus
}
#endif
#endif /* CELIX_THREADS_H_ */
