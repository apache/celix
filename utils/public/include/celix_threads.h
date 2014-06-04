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
 * celix_threads.h
 *
 *  \date       4 Jun 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_THREADS_H_
#define CELIX_THREADS_H_

#include <pthread.h>

#include "celix_errno.h"

typedef pthread_t celix_thread_t;
typedef pthread_attr_t celix_thread_attr_t;

typedef void *(*celix_thread_start_t)(void*);

celix_status_t celixThread_create(celix_thread_t *new_thread, celix_thread_attr_t *attr, celix_thread_start_t func, void *data);
celix_status_t celixThread_exit(void *exitStatus);
celix_status_t celixThread_detach(celix_thread_t thread);
celix_status_t celixThread_join(celix_thread_t thread, void **status);
celix_thread_t celixThread_self();
int celixThread_equals(celix_thread_t thread1, celix_thread_t thread2);

typedef pthread_mutex_t celix_thread_mutex_t;
typedef pthread_mutexattr_t celix_thread_mutexattr_t;

celix_status_t celixThreadMutex_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr);
celix_status_t celixThreadMutex_destroy(celix_thread_mutex_t *mutex);
celix_status_t celixThreadMutex_lock(celix_thread_mutex_t *mutex);
celix_status_t celixThreadMutex_unlock(celix_thread_mutex_t *mutex);

typedef pthread_cond_t celix_thread_cond_t;
typedef pthread_condattr_t celix_thread_condattr_t;

celix_status_t celixThreadCondition_init(celix_thread_cond_t *condition, celix_thread_condattr_t *attr);
celix_status_t celixThreadCondition_wait(celix_thread_cond_t *cond, celix_thread_mutex_t *mutex);
celix_status_t celixThreadCondition_broadcast(celix_thread_cond_t *cond);
celix_status_t celixThreadCondition_signal(celix_thread_cond_t *cond);
celix_status_t celixThreadCondition_signalThreadNp(celix_thread_cond_t *cond, celix_thread_t thread);

#endif /* CELIX_THREADS_H_ */
