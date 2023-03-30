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

#include "celix_threads_ei.h"
#include "celix_error_injector.h"
#include "celix_threads.h"
#include "celix_errno.h"

extern "C" {
celix_status_t __real_celixThreadMutex_create(celix_thread_mutex_t *__mutex, celix_thread_mutexattr_t *__attr);
CELIX_EI_DEFINE(celixThreadMutex_create, celix_status_t)
celix_status_t __wrap_celixThreadMutex_create(celix_thread_mutex_t *__mutex, celix_thread_mutexattr_t *__attr) {
    CELIX_EI_IMPL_POSITIVE(celixThreadMutex_create);
    return __real_celixThreadMutex_create(__mutex, __attr);
}

celix_status_t __real_celixThread_create(celix_thread_t *__new_thread, celix_thread_attr_t *__attr, celix_thread_start_t __func, void *__data);
CELIX_EI_DEFINE(celixThread_create, celix_status_t)
celix_status_t __wrap_celixThread_create(celix_thread_t *__new_thread, celix_thread_attr_t *__attr, celix_thread_start_t __func, void *__data) {
    CELIX_EI_IMPL_POSITIVE(celixThread_create);
    return __real_celixThread_create(__new_thread, __attr, __func, __data);
}
}
