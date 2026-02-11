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

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include "celix_compiler.h"
#include "celix_threads.h"
#include "celix_utils.h"

#ifndef CELIX_UTILS_STATIC_DEFINE
const celix_thread_t celix_thread_default = {0, 0};
#endif

celix_status_t celixThread_create(celix_thread_t *new_thread, const celix_thread_attr_t *attr, celix_thread_start_t func, void *data) {
    celix_status_t status = CELIX_SUCCESS;

    if (pthread_create(&(*new_thread).thread, attr, func, data) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else {
        __atomic_store_n(&(*new_thread).threadInitialized, true, __ATOMIC_RELEASE);
    }

    return status;
}

#if defined(_GNU_SOURCE) && defined(__linux__) && !defined(__UCLIBC__)
void celixThread_setName(celix_thread_t *thread, const char *threadName) {
    pthread_setname_np(thread->thread, threadName);
}
#else
void celixThread_setName(celix_thread_t *thread CELIX_UNUSED, const char *threadName  CELIX_UNUSED) {
    //nop
}
#endif

// Returns void, since pthread_exit does exit the thread and never returns.
void celixThread_exit(void *exitStatus) {
    pthread_exit(exitStatus);
}

celix_status_t celixThread_detach(celix_thread_t thread) {
    return pthread_detach(thread.thread);
}

celix_status_t celixThread_join(celix_thread_t thread, void **retVal) {
    celix_status_t status = CELIX_SUCCESS;

    if (pthread_join(thread.thread, retVal) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    }

    // #TODO make thread a pointer? Now this statement has no effect
    // thread.threadInitialized = false;

    return status;
}

celix_status_t celixThread_kill(celix_thread_t thread, int sig) {
    return pthread_kill(thread.thread, sig);
}

celix_thread_t celixThread_self() {
    celix_thread_t thread;

    thread.thread = pthread_self();
    thread.threadInitialized = true;

    return thread;
}

int celixThread_equals(celix_thread_t thread1, celix_thread_t thread2) {
    return pthread_equal(thread1.thread, thread2.thread);
}

bool celixThread_initialized(celix_thread_t thread) {
    return __atomic_load_n(&thread.threadInitialized, __ATOMIC_ACQUIRE);
}


celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr) {
    return pthread_mutex_init(mutex, attr);
}

celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex) {
    return pthread_mutex_destroy(mutex);
}

celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex) {
    return pthread_mutex_lock(mutex);
}

celix_status_t celixThreadMutex_tryLock(celix_thread_mutex_t *mutex) {
    return pthread_mutex_trylock(mutex);
}

celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex) {
    return pthread_mutex_unlock(mutex);
}

celix_status_t celixThreadMutexAttr_create(celix_thread_mutexattr_t *attr) {
    return pthread_mutexattr_init(attr);
}

celix_status_t celixThreadMutexAttr_destroy(celix_thread_mutexattr_t *attr) {
    return pthread_mutexattr_destroy(attr);
}

celix_status_t celixThreadMutexAttr_settype(celix_thread_mutexattr_t *attr, int type) {
    celix_status_t status;
    switch(type) {
        case CELIX_THREAD_MUTEX_NORMAL :
            status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_NORMAL);
            break;
        case CELIX_THREAD_MUTEX_RECURSIVE :
            status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
            break;
        case CELIX_THREAD_MUTEX_ERRORCHECK :
            status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_ERRORCHECK);
            break;
        case CELIX_THREAD_MUTEX_DEFAULT :
            status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_DEFAULT);
            break;
        default:
            status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_DEFAULT);
            break;
    }
    return status;
}

celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr) {
#ifdef __APPLE__
    return pthread_cond_init(condition, attr);
#else
    celix_status_t status = CELIX_SUCCESS;
    if (attr) {
        status = pthread_condattr_setclock(attr, CLOCK_MONOTONIC);
        status = CELIX_DO_IF(status, pthread_cond_init(condition, attr));
    } else {
        celix_thread_condattr_t condattr;
        (void)pthread_condattr_init(&condattr); // always return 0
        status = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
        status = CELIX_DO_IF(status, pthread_cond_init(condition, &condattr));
        (void)pthread_condattr_destroy(&condattr); // always return 0
    }
    return status;
#endif
}

celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition) {
    return pthread_cond_destroy(condition);
}

celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex) {
    return pthread_cond_wait(cond, mutex);
}

#ifdef __APPLE__
celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds) {
    struct timespec time;
    time.tv_sec = seconds;
    time.tv_nsec = nanoseconds;
    return pthread_cond_timedwait_relative_np(cond, mutex, &time);
}
#else
celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds) {
    double delay = (double)seconds + ((double)nanoseconds / 1000000000);
    struct timespec time = celixThreadCondition_getDelayedTime(delay);
    return pthread_cond_timedwait(cond, mutex, &time);
}
#endif

struct timespec celixThreadCondition_getTime() {
    return celixThreadCondition_getDelayedTime(0);
}

struct timespec celixThreadCondition_getDelayedTime(double delayInSeconds) {
    struct timespec now = celix_gettime(CLOCK_MONOTONIC);
    if (delayInSeconds == 0) {
        return now;
    }
    return celix_delayedTimespec(&now, delayInSeconds);
}

celix_status_t
celixThreadCondition_waitUntil(celix_thread_cond_t* cond, celix_thread_mutex_t* mutex, const struct timespec* absTime) {
    if (absTime == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
#if __APPLE__
    struct timespec now = celix_gettime(CLOCK_MONOTONIC);
    double diff = celix_difftime(&now, absTime);
    if (diff <= 0) {
        return ETIMEDOUT;
    }
    long seconds = diff;
    long nanoseconds = (diff - seconds) * CELIX_NS_IN_SEC;
    return celixThreadCondition_timedwaitRelative(cond, mutex, seconds, nanoseconds);
#else
    return pthread_cond_timedwait(cond, mutex, absTime);
#endif
}

celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond) {
    return pthread_cond_broadcast(cond);
}

celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond) {
    return pthread_cond_signal(cond);
}

celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr) {
    return pthread_rwlock_init(lock, attr);
}

celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_destroy(lock);
}

celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_rdlock(lock);
}

celix_status_t celixThreadRwlock_tryReadLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_tryrdlock(lock);
}

celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_wrlock(lock);
}

celix_status_t celixThreadRwlock_tryWriteLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_trywrlock(lock);
}

celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_unlock(lock);
}

celix_status_t celixThreadRwlockAttr_create(celix_thread_rwlockattr_t *attr) {
    return pthread_rwlockattr_init(attr);
}

celix_status_t celixThreadRwlockAttr_destroy(celix_thread_rwlockattr_t *attr) {
    return pthread_rwlockattr_destroy(attr);
}

celix_status_t celixThread_once(celix_thread_once_t *once_control, void (*init_routine)(void)) {
    return pthread_once(once_control, init_routine);
}
