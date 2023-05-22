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
#include "signal.h"
#include "celix_threads.h"
#include "celix_utils.h"


celix_status_t celixThread_create(celix_thread_t *thread, const celix_thread_attr_t *attr, celix_thread_start_t func, void *data) {
    celix_status_t status = pthread_create(&thread->thread, attr, func, data);
    thread->threadInitialized = (status == CELIX_SUCCESS);
    return status;
}

#if defined(_GNU_SOURCE) && defined(__linux__) && !defined(__UCLIBC__)
void celixThread_setName(celix_thread_t *thread, const char *threadName) {
    pthread_setname_np(thread->thread, threadName);
}
#else
void celixThread_setName(celix_thread_t *thread __attribute__((unused)), const char *threadName  __attribute__((unused))) {
    //nop
}
#endif

// Returns void, since pthread_exit does exit the thread and never returns.
void celixThread_exit(void *exitStatus) {
    pthread_exit(exitStatus);
}

celix_status_t celixThread_detach(celix_thread_t thread) {
    if (celixThread_initialized(thread)) {
        return pthread_detach(thread.thread);
    }
    return CELIX_SUCCESS;
}

celix_status_t celixThread_join(celix_thread_t thread, void** retVal) {
    if (celixThread_initialized(thread)) {
        return pthread_join(thread.thread, retVal);   
    }
    return CELIX_SUCCESS;
}

celix_status_t celixThread_kill(celix_thread_t thread, int sig) {
    if (celixThread_initialized(thread)) {
        return pthread_kill(thread.thread, sig);
    }
    return CELIX_SUCCESS;
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
    celix_status_t status = pthread_mutex_init(&mutex->pthreadMutex, attr);
    if (status != CELIX_SUCCESS) {
        __atomic_store_n(&mutex->initialized, false, __ATOMIC_RELEASE);
    }
    return status;
}

celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex) {
    if (celixThreadMutex_isInitialized(mutex)) {
        return pthread_mutex_destroy(&mutex->pthreadMutex);
    } 
    return CELIX_SUCCESS; 
}

bool celixThreadMutex_isInitialized(const celix_thread_mutex_t *mutex) {
    return __atomic_load_n(&mutex->initialized, __ATOMIC_ACQUIRE);
}

celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex) {
    return pthread_mutex_lock(&mutex->pthreadMutex);
}

celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex) {
    return pthread_mutex_unlock(&mutex->pthreadMutex);
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
    celix_status_t status = CELIX_SUCCESS;
#ifdef __APPLE__
    status = pthread_cond_init(condition, attr);
#else
    if (attr) {
        status = pthread_condattr_setclock(attr, CLOCK_MONOTONIC);
        status = CELIX_DO_IF(status, pthread_cond_init(&condition->pthreadCond, attr));
    } else {
        celix_thread_condattr_t condattr;
        (void)pthread_condattr_init(&condattr); // always return 0
        status = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
        status = CELIX_DO_IF(status, pthread_cond_init(&condition->pthreadCond, &condattr));
        (void)pthread_condattr_destroy(&condattr); // always return 0
    }
#endif
    condition->initialized = (status == CELIX_SUCCESS);
    return status;
}

celix_status_t celixThreadCondition_destroy(celix_thread_cond_t* cond) {
    if (celixThreadCondition_isInitialized(cond)) {
        return pthread_cond_destroy(&cond->pthreadCond);
    }
    return CELIX_SUCCESS;
}        

bool celixThreadCondition_isInitialized(const celix_thread_cond_t* cond) {
    return __atomic_load_n(&cond->initialized, __ATOMIC_ACQUIRE);
}

celix_status_t celixThreadCondition_wait(celix_thread_cond_t* cond, celix_thread_mutex_t* mutex) {
    return pthread_cond_wait(&cond->pthreadCond, &mutex->pthreadMutex);
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
    struct timespec time;
    seconds = seconds >= 0 ? seconds : 0;
    time = celix_gettime(CLOCK_MONOTONIC);
    time.tv_sec += seconds;
    if(nanoseconds > 0) {
        time.tv_nsec += nanoseconds;
        while (time.tv_nsec > CELIX_NS_IN_SEC) {
            time.tv_sec++;
            time.tv_nsec -= CELIX_NS_IN_SEC;
        }
    }
    return pthread_cond_timedwait(&cond->pthreadCond, &mutex->pthreadMutex, &time);
}
#endif

celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond) {
    return pthread_cond_broadcast(&cond->pthreadCond);
}

celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond) {
    return pthread_cond_signal(&cond->pthreadCond);
}

celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr) {
    celix_status_t status = pthread_rwlock_init(&lock->pthreadRwLock, attr);
    lock->initialized = (status == CELIX_SUCCESS);
    return status;
}

celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock) {
    if (!celixThreadRwlock_isInitialized(lock)) {
        return pthread_rwlock_destroy(&lock->pthreadRwLock);
    }
    return CELIX_SUCCESS;
}

bool celixThreadRwlock_isInitialized(const celix_thread_rwlock_t *lock) {
    return __atomic_load_n(&lock->initialized, __ATOMIC_ACQUIRE);
}

celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_rdlock(&lock->pthreadRwLock);
}

celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_wrlock(&lock->pthreadRwLock);
}

celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock) {
    return pthread_rwlock_unlock(&lock->pthreadRwLock);
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

celix_status_t celix_tss_create(celix_tss_key_t* key, void (*destroyFunction)(void*)) {
    celix_status_t status = pthread_key_create(&key->pthreadKey, destroyFunction);
    key->initialized = (status == CELIX_SUCCESS);
    return status;
}

bool celix_tss_isInitialized(const celix_tss_key_t* key) {
    return __atomic_load_n(&key->initialized, __ATOMIC_ACQUIRE);
}

celix_status_t celix_tss_delete(celix_tss_key_t* key) {
    if (key && celix_tss_isInitialized(key)) {
        return pthread_key_delete(key->pthreadKey); 
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_tss_set(const celix_tss_key_t* key, void* value) {
    if (!celix_tss_isInitialized(key)) {
        return CELIX_ILLEGAL_STATE;
    }
    return pthread_setspecific(key->pthreadKey, value);
}

void* celix_tss_get(const celix_tss_key_t* key) {
    if (!celix_tss_isInitialized(key)) {
        return NULL;
    }
    return pthread_getspecific(key->pthreadKey);
}
