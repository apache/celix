/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * celix_threads.c
 *
 *  \date       4 Jun 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include "celix_threads.h"


celix_status_t celixThread_create(celix_thread_t *new_thread, celix_thread_attr_t *attr, celix_thread_start_t func, void *data) {
    celix_status_t status = CELIX_SUCCESS;

	if (pthread_create(&(*new_thread).thread, attr, func, data) != 0) {
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else {
		(*new_thread).threadInitialized = true;
	}

	return status;
}

celix_status_t celixThread_exit(void *exitStatus) {
    celix_status_t status = CELIX_SUCCESS;
    pthread_exit(exitStatus);
    return status;
}

celix_status_t celixThread_detach(celix_thread_t thread) {
    return pthread_detach(thread.thread);
}

celix_status_t celixThread_join(celix_thread_t thread, void **retVal) {
	celix_status_t status = CELIX_SUCCESS;

	if (pthread_join(thread.thread, retVal) != 0) {
		status = CELIX_BUNDLE_EXCEPTION;
	}

	thread.threadInitialized = false;

    return status;
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

bool celixThread_initalized(celix_thread_t thread) {
    return thread.threadInitialized;
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
	celix_status_t status = CELIX_SUCCESS;
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
    return pthread_cond_init(condition, attr);
}

celix_status_t celixThreadCondition_destroy(celix_thread_cond_t *condition) {
    return pthread_cond_destroy(condition);
}

celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex) {
    return pthread_cond_wait(cond, mutex);
}

celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond) {
    return pthread_cond_broadcast(cond);
}

celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond) {
    return pthread_cond_signal(cond);
}
