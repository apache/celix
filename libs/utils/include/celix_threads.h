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
/**
 * celix_threads.h
 *
 *  \date       4 Jun 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_THREADS_H_
#define CELIX_THREADS_H_

#include <pthread.h>
#include <stdbool.h>

#include "celix_errno.h"

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

celix_status_t
celixThread_create(celix_thread_t *new_thread, celix_thread_attr_t *attr, celix_thread_start_t func, void *data);

/**
 * If supported by the platform sets the name of the thread.
 */
void celixThread_setName(celix_thread_t *thread, const char *threadName);

void celixThread_exit(void *exitStatus);

celix_status_t celixThread_detach(celix_thread_t thread);

celix_status_t celixThread_join(celix_thread_t thread, void **status);

celix_status_t celixThread_kill(celix_thread_t thread, int sig);

celix_thread_t celixThread_self(void);

int celixThread_equals(celix_thread_t thread1, celix_thread_t thread2);

bool celixThread_initialized(celix_thread_t thread);


typedef pthread_mutex_t celix_thread_mutex_t;
typedef pthread_mutexattr_t celix_thread_mutexattr_t;

//MUTEX TYPES
enum {
    CELIX_THREAD_MUTEX_NORMAL,
    CELIX_THREAD_MUTEX_RECURSIVE,
    CELIX_THREAD_MUTEX_ERRORCHECK,
    CELIX_THREAD_MUTEX_DEFAULT
};


celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr);

celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex);

celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex);

celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex);

celix_status_t celixThreadMutexAttr_create(celix_thread_mutexattr_t *attr);

celix_status_t celixThreadMutexAttr_destroy(celix_thread_mutexattr_t *attr);

celix_status_t celixThreadMutexAttr_settype(celix_thread_mutexattr_t *attr, int type);

typedef pthread_rwlock_t celix_thread_rwlock_t;
typedef pthread_rwlockattr_t celix_thread_rwlockattr_t;

celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr);

celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock);

celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock);

celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock);

celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock);

celix_status_t celixThreadRwlockAttr_create(celix_thread_rwlockattr_t *attr);

celix_status_t celixThreadRwlockAttr_destroy(celix_thread_rwlockattr_t *attr);
//NOTE: No support yet for setting specific rw lock attributes


typedef pthread_cond_t celix_thread_cond_t;
typedef pthread_condattr_t celix_thread_condattr_t;

celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr);

celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition);

celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex);

celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds);

celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond);

celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond);

celix_status_t celixThread_once(celix_thread_once_t *once_control, void (*init_routine)(void));

#ifdef __cplusplus
}
#endif
#endif /* CELIX_THREADS_H_ */
