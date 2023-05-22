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

/**
 * @file celix_threads.h
 * @brief Abstraction of thread utilities.
 * 
 * This file contains the abstraction of thread utilities. Currently only pthread is supported.
 */


struct celix_thread {
    bool threadInitialized; /**< Indicates if the thread is initialized. */
    pthread_t thread; /**< The actual thread.*/
};

typedef struct celix_thread celix_thread_t;
typedef pthread_attr_t celix_thread_attr_t;

typedef void *(*celix_thread_start_t)(void *);

static const celix_thread_t celix_thread_default = {0, 0};

/**
 * @brief Create a new thread.
 * 
 * Note will not check the initialized field when creating.
 * 
 * @section errors Errors
 * If an error occurs, the thread is not created and the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN Insufficient resources to create another thread.
 * - EAGAIN A system-imposed limit on the number of threads was encountered. 
 * - EINVAL Invalid settings in attr.
 * - EPERM  No permission to set the scheduling policy and parameters specified in attr.
 * @endsection
 * 
 * @param[in,out] new_thread The created thread.
 * @param[in] attr The thread attributes. Can be NULL for default attributes.
 * @param[in] func The function to execute in the thread.
 * @param[in] data The data passed to the function.
 * @return CELIX_SUCCESS if the thread is created.
 */
CELIX_UTILS_EXPORT celix_status_t
celixThread_create(celix_thread_t *new_thread, const celix_thread_attr_t *attr, celix_thread_start_t func, void *data);


/**
 * @brief If supported by the platform sets the name of the thread.
 */
CELIX_UTILS_EXPORT void celixThread_setName(celix_thread_t *thread, const char *threadName);

/**
 * @brief Exit the current thread.
 * 
 * @param[in] exitStatus The exit status of the thread. This is output status for the celixThread_join function.
 */
CELIX_UTILS_EXPORT void celixThread_exit(void *exitStatus);

/**
 * @brief Detach the thread.
 * 
 * Will silently ignore if the thread is not initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EINVAL thread is not a joinable thread.
 * - ESRCH  No thread with the ID thread could be found.
 * 
 * @param[in] thread The thread to detach.
 * @return CELIX_SUCCESS if the thread is detached.
 */
CELIX_UTILS_EXPORT celix_status_t celixThread_detach(celix_thread_t thread);

/**
 * @brief Join the thread.
 * 
 * Will silently ignore if the thread is not initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 *  - EDEADLK A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.
 *  - EINVAL thread is not a joinable thread.
 *  - EINVAL Another thread is already waiting to join with this thread.
 *  - ESRCH  No thread with the ID thread could be found.
 * @endsection
 * 
 * @param[in] thread The thread to join.
 * @param[out] status The exit status of the thread.
 * @return CELIX_SUCCESS if the thread is joined.
 */
CELIX_UTILS_EXPORT celix_status_t celixThread_join(celix_thread_t thread, void** status);

/**
 * @brief Kill the thread.
 * 
 * Will silently ignore if the thread is not initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EINVAL An invalid signal was specified.
 * @endsection
 * 
 * @param[in] thread The thread to kill.
 * @param[in] sig The signal to send to the thread.
 * @return CELIX_SUCCESS if the thread is killed.
 */
CELIX_UTILS_EXPORT celix_status_t celixThread_kill(celix_thread_t thread, int sig);

/**
 * Return the current thread.
 */
CELIX_UTILS_EXPORT celix_thread_t celixThread_self(void);

/**
 * @brief Return true - as int - if the threads are equals
 * @param[in] thread1
 * @param[in] thread2
 * @return non-zero if the thread IDs t1 and t2 correspond to the same thread, otherwise it will return zero.
 */
CELIX_UTILS_EXPORT int celixThread_equals(celix_thread_t thread1, celix_thread_t thread2);

/**
 * @brief Check if the thread is initialized.
 * 
 * @param[in] thread
 * @return true if the thread is initialized.
 */
CELIX_UTILS_EXPORT bool celixThread_initialized(celix_thread_t thread);


typedef pthread_once_t celix_thread_once_t;
#define CELIX_THREAD_ONCE_INIT PTHREAD_ONCE_INIT

CELIX_UTILS_EXPORT celix_status_t celixThread_once(celix_thread_once_t *once_control, void (*init_routine)(void));

typedef pthread_mutexattr_t celix_thread_mutexattr_t;

typedef struct celix_thread_mutex {
    bool initialized; /**< Indicates if the thread is initialized. */
    pthread_mutex_t pthreadMutex; /**< The actual mutex. */
} celix_thread_mutex_t;

#define CELIX_THREAD_MUTEX_INITIALIZER {true, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEXATTR_INITIALIZER}

//MUTEX TYPES
enum {
    CELIX_THREAD_MUTEX_NORMAL,
    CELIX_THREAD_MUTEX_RECURSIVE,
    CELIX_THREAD_MUTEX_ERRORCHECK,
    CELIX_THREAD_MUTEX_DEFAULT
};

/**
 * @brief Create a mutex.
 * 
 * Note will not check the initialized field when creating.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The system lacked the necessary resources (other than memory) to initialize another mutex.
 * - ENOMEM Insufficient memory exists to initialize the mutex.
 * - EPERM  The caller does not have the privilege to perform the operation.
 * - EINVAL The attributes object referenced by attr has the robust mutex attribute set without the process-shared 
 *   attribute being set.
 * @endsection
 * 
 * @param[out] mutex The created mutex.
 * @param[in] attr The mutex attributes. Can be NULL.
 * @return CELIX_SUCCESS if the mutex is created.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr);

/**
 * @brief Destroy a mutex.
 * 
 * Will silently ignore if the mutex is not initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The system lacked the necessary resources (other than memory) to initialize another mutex.
 * - ENOMEM Insufficient memory exists to initialize the mutex.
 * - EPERM  The caller does not have the privilege to perform the operation.
 * @endsection
 * 
 * @param[in,out] mutex The mutex to destroy.
 * @return CELIX_SUCCESS if the mutex is destroyed.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex);

/**
 * @brief Check if the mutex is initialized.
 */
CELIX_UTILS_EXPORT bool celixThreadMutex_isInitialized(const celix_thread_mutex_t *mutex);

/**
 * @brief Lock a mutex.
 * 
 * Will not check if the mutex is initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.
 * - EINVAL The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling 
 *   thread's priority is higher than the mutex's current priority ceiling.
 * - ENOTRECOVERABLE The state protected by the mutex is not recoverable.
 * - EOWNERDEAD The mutex is a robust mutex and the process containing the previous owning thread terminated while 
 *   holding the mutex lock. The mutex lock shall be acquired by the calling thread and it is up to the new owner to 
 *   make the state consistent.
 * - EDEADLK The mutex type is PTHREAD_MUTEX_ERRORCHECK and the current thread already owns the mutex.
 * - EOWNERDEAD The mutex is a robust mutex and the previous owning thread terminated while holding the mutex lock. 
 *   The mutex lock shall be acquired by the calling thread and it is up to the new owner to make the state consistent.
 * - EDEADLK A deadlock condition was detected.
 * @endsection
 * 
 * @param[in] mutex The mutex to lock.
 * @return CELIX_SUCCESS if the mutex is locked.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 * 
 * Will not check if the mutex is initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.
 * - EINVAL The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling 
 *   thread's priority is higher than the mutex's current priority ceiling.
 * - ENOTRECOVERABLE The state protected by the mutex is not recoverable.
 * - EOWNERDEAD The mutex is a robust mutex and the process containing the previous owning thread terminated while 
 *   holding the mutex lock. The mutex lock shall be acquired by the calling thread and it is up to the new owner to 
 *   make the state consistent.
 * - EPERM  The mutex type is PTHREAD_MUTEX_ERRORCHECK or PTHREAD_MUTEX_RECURSIVE, or the mutex is a robust mutex, 
 *   and the current thread does not own the mutex.
 * @endsection
 * 
 * @param[in] mutex The mutex to unlock.
 * @return CELIX_SUCCESS if the mutex is unlocked.
 * 
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_create(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_destroy(celix_thread_mutexattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadMutexAttr_settype(celix_thread_mutexattr_t *attr, int type);

typedef pthread_rwlockattr_t celix_thread_rwlockattr_t;
typedef struct celix_thread_rwlock {
    bool initialized;
    pthread_rwlock_t pthreadRwLock;
} celix_thread_rwlock_t;


/**
 * @brief Create a read-write lock.
 * 
 * Note will not check the initialized field when creating.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The system lacked the necessary resources (other than memory) to initialize another read-write lock.
 * - ENOMEM Insufficient memory exists to initialize the read-write lock.
 * - EPERM The caller does not have the privilege to perform the operation.
 * @endsection
 * 
 * @param[out] lock The created read-write lock.
 * @param[in] attr The read-write lock attributes. Can be NULL.
 * @return CELIX_SUCCESS if the read-write lock is created.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_create(celix_thread_rwlock_t *lock, celix_thread_rwlockattr_t *attr);

/**
 * @brief Destroy a read-write lock.
 * 
 * Will silently ignore if the lock is not initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * - EAGAIN The system lacked the necessary resources (other than memory) to initialize another read-write lock.
 * - ENOMEM Insufficient memory exists to initialize the read-write lock.
 * - EPERM  The caller does not have the privilege to perform the operation
 * @endsection
 * 
 * @param[in] lock The read-write lock to destroy.
 * @return CELIX_SUCCESS if the read-write lock is destroyed.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_destroy(celix_thread_rwlock_t *lock);

/**
 * @brief Check if a read-write lock is initialized.
 */
CELIX_UTILS_EXPORT bool celixThreadRwlock_isInitialized(const celix_thread_rwlock_t *lock);

/**
 * @brief Lock a read-write lock for reading.
 * 
 * Will not check if the lock is initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * - EAGAIN The read lock could not be acquired because the maximum number of read locks for rwlock has been exceeded.
 * - EDEADLK A deadlock condition was detected or the current thread already owns the read-write lock for writing.
 * @endsection
 * 
 * @param[in] lock The read-write lock to lock.
 * @return CELIX_SUCCESS if the read-write lock is locked.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_readLock(celix_thread_rwlock_t *lock);

/**
 * @brief Lock a read-write lock for writing.
 * 
 * Will not check if the lock is initialized.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * -  EDEADLK A deadlock condition was detected or the current thread already owns the read-write lock for writing 
 *    or reading.
 * 
 * @param[in] lock The read-write lock to lock.
 * @return CELIX_SUCCESS if the read-write lock is locked.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_writeLock(celix_thread_rwlock_t *lock);

/**
 * @brief Unlock a read-write lock.
 * 
 * Will not check if the lock is initialized.
 * 
 * @param[in] lock The read-write lock to unlock.
 * @return CELIX_SUCCESS if the read-write lock is unlocked.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadRwlock_unlock(celix_thread_rwlock_t *lock);


CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_create(celix_thread_rwlockattr_t *attr);

CELIX_UTILS_EXPORT celix_status_t celixThreadRwlockAttr_destroy(celix_thread_rwlockattr_t *attr);

typedef pthread_condattr_t celix_thread_condattr_t;
typedef struct celix_thread_cond {
    bool initialized; /**< Indicates if the condition is initialized. */
    pthread_cond_t pthreadCond; /**< The actual condition variable. */
} celix_thread_cond_t;

/**
 * @brief Initialize a condition variable.
 * 
 * Note will not check the initialized field when initializing.
 * 
 * @section errors Errors
 * If an error occurs, the function returns an error code.
 * 
 * And if pthread is the underlying implementation:
 * - EAGAIN The system lacked the necessary resources (other than memory) to initialize another condition variable.
 * - ENOMEM Insufficient memory exists to initialize the condition variable.
 * @endsection 
 * 
 * @param[in,out] condition The condition variable to initialize.
 * @param[in] attr The condition variable attributes to use. Can be NULL.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr);

/**
 * @brief Destroy a condition variable.
 * 
 * Will silently ignore if the condition variable is not initialized.
 * 
 * @oarnam[in] condition The condition variable to destroy.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition);

/**
 * @brief Check if a condition variable is initialized.
 */
CELIX_UTILS_EXPORT bool celixThreadCondition_isInitialized(const celix_thread_cond_t *condition);

/**
 * @brief Wait for a condition variable to be signaled.
 * 
 * Will not check if the condition variable is initialized.
 * 
 * @param[in] cond The condition to wait for.
 * @param[in] mutex The mutex to use.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex);

/**
 * @brief Wait for a condition variable to be signaled or a timeout to occur.
 * 
 * Will not check if the condition variable is initialized.
 * 
 * @param[in] cond The condition to wait for.
 * @param[in] mutex The mutex to use.
 * @param[in] seconds The seconds to wait.
 * @param[in] nanoseconds The nanoseconds to wait.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_timedwaitRelative(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex, long seconds, long nanoseconds);

/**
 * @brief Broadcast a condition.
 * 
 * @param[in] cond The condition to broadcast.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond);

/**
 * @brief Signal a condition.
 * 
 * @param[in] cond The condition to signal.
 * @return CELIX_SUCCESS if no errors are encountered.
 */
CELIX_UTILS_EXPORT celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond);

//Thread Specific Storage (TSS) Abstraction
typedef struct celix_tss_key {
    bool initialized;
    pthread_key_t pthreadKey;
} celix_tss_key_t;

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
 * Note will not check the initialized field when creating.
 *
 * @param key The key to create.
 * @param destroyFunction The function to call when the key is destroyed.
 * @return CELIX_SUCCESS if the key is created successfully; otherwise, an error code is returned.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_create(celix_tss_key_t* key, void (*destroyFunction)(void*));

/**
 * @brief Check if a thread specific storage key is initialized.
 */
CELIX_UTILS_EXPORT bool celix_tss_isInitialized(const celix_tss_key_t* key);

/**
 * @brief Delete a thread specific storage key previously created by celix_tss_create.
 * 
 * Will silently ignore if the key is NULL or key is not initialized.
 *
 * @param key The key to delete.
 * @return CELIX_SUCCESS if the key is deleted successfully; otherwise, an error code is returned.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_delete(celix_tss_key_t* key);

/**
 * @brief Set a thread-specific value for the provide thread specific storage key.
 *
 * @param key The key to set the value for.
 * @param value The thread-specific value to set.
 * @return CELIX_SUCCESS if the value is set successfully; otherwise, an error code is returned.
 * @retval CELIX_ILLEGAL_STATE if the key is not initialized.
 */
CELIX_UTILS_EXPORT celix_status_t celix_tss_set(const celix_tss_key_t* key, void* value);

/**
 * @brief Get the thread-specific value for the provided thread specific storage key.
 *
 * @param key The key to get the value for.
 * @return The thread-specific value.
 *
 * @retval NULL if the key is invalid or there is no thread-specific value set for the key.
 */
CELIX_UTILS_EXPORT void* celix_tss_get(const celix_tss_key_t* key);

#ifdef __cplusplus
}
#endif
#endif /* CELIX_THREADS_H_ */
