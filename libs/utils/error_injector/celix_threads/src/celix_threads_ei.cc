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
    CELIX_EI_IMPL(celixThreadMutex_create);
    return __real_celixThreadMutex_create(__mutex, __attr);
}

celix_status_t
__real_celixThread_create(celix_thread_t *__new_thread, celix_thread_attr_t *__attr, celix_thread_start_t __func,
                          void *__data);
CELIX_EI_DEFINE(celixThread_create, celix_status_t)
celix_status_t
__wrap_celixThread_create(celix_thread_t *__new_thread, celix_thread_attr_t *__attr, celix_thread_start_t __func,
                          void *__data) {
    CELIX_EI_IMPL(celixThread_create);
    return __real_celixThread_create(__new_thread, __attr, __func, __data);
}

celix_status_t __real_celixThreadCondition_signal(celix_thread_cond_t* cond);
CELIX_EI_DEFINE(celixThreadCondition_signal, celix_status_t)
celix_status_t __wrap_celixThreadCondition_signal(celix_thread_cond_t* cond) {
    CELIX_EI_IMPL(celixThreadCondition_signal);
    return __real_celixThreadCondition_signal(cond);
}

celix_status_t __real_celixThreadCondition_init(celix_thread_cond_t *__condition, celix_thread_condattr_t *__attr);
CELIX_EI_DEFINE(celixThreadCondition_init, celix_status_t)
celix_status_t __wrap_celixThreadCondition_init(celix_thread_cond_t *__condition, celix_thread_condattr_t *__attr) {
    CELIX_EI_IMPL(celixThreadCondition_init);
    return __real_celixThreadCondition_init(__condition, __attr);
}

celix_status_t __real_celixThreadRwlock_create(celix_thread_rwlock_t *__rwlock, celix_thread_rwlockattr_t *__attr);
CELIX_EI_DEFINE(celixThreadRwlock_create, celix_status_t)
celix_status_t __wrap_celixThreadRwlock_create(celix_thread_rwlock_t *__rwlock, celix_thread_rwlockattr_t *__attr) {
    CELIX_EI_IMPL(celixThreadRwlock_create);
    return __real_celixThreadRwlock_create(__rwlock, __attr);
}


celix_status_t __real_celix_tss_create(celix_tss_key_t* __key, void (*__destroyFunction)(void*));
CELIX_EI_DEFINE(celix_tss_create, celix_status_t)
celix_status_t __wrap_celix_tss_create(celix_tss_key_t* __key, void (*__destroyFunction)(void*)) {
    CELIX_EI_IMPL(celix_tss_create);
    return __real_celix_tss_create(__key, __destroyFunction);
}

celix_status_t __real_celix_tss_delete(celix_tss_key_t __key);
CELIX_EI_DEFINE(celix_tss_delete, celix_status_t)
celix_status_t __wrap_celix_tss_delete(celix_tss_key_t __key) {
    CELIX_EI_IMPL(celix_tss_delete);
    return __real_celix_tss_delete(__key);
}

celix_status_t __real_celix_tss_set(celix_tss_key_t __key, void* __value);
CELIX_EI_DEFINE(celix_tss_set, celix_status_t)
celix_status_t __wrap_celix_tss_set(celix_tss_key_t __key, void* __value) {
    CELIX_EI_IMPL(celix_tss_set);
    return __real_celix_tss_set(__key, __value);
}

void* __real_celix_tss_get(celix_tss_key_t __key);
CELIX_EI_DEFINE(celix_tss_get, void*)
void* __wrap_celix_tss_get(celix_tss_key_t __key) {
    CELIX_EI_IMPL(celix_tss_get);
    return __real_celix_tss_get(__key);
}

}
