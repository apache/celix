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
#include "pthread_ei.h"
#include <pthread.h>

extern "C" {
int __real_pthread_mutexattr_init(pthread_mutexattr_t * __attr);
CELIX_EI_DEFINE(pthread_mutexattr_init, int)
int __wrap_pthread_mutexattr_init(pthread_mutexattr_t * __attr) {
    CELIX_EI_IMPL(pthread_mutexattr_init);
    return __real_pthread_mutexattr_init(__attr);
}

int __real_pthread_mutexattr_setpshared(pthread_mutexattr_t * __attr, int __pshared);
CELIX_EI_DEFINE(pthread_mutexattr_setpshared, int)
int __wrap_pthread_mutexattr_setpshared(pthread_mutexattr_t * __attr, int __pshared) {
    CELIX_EI_IMPL(pthread_mutexattr_setpshared);
    return __real_pthread_mutexattr_setpshared(__attr, __pshared);
}

int __real_pthread_mutex_init(pthread_mutex_t * __mutex, const pthread_mutexattr_t * __mutex_attr);
CELIX_EI_DEFINE(pthread_mutex_init, int)
int __wrap_pthread_mutex_init(pthread_mutex_t * __mutex, const pthread_mutexattr_t * __mutex_attr) {
    CELIX_EI_IMPL(pthread_mutex_init);
    return __real_pthread_mutex_init(__mutex, __mutex_attr);
}

int __real_pthread_condattr_init(pthread_condattr_t * __attr);
CELIX_EI_DEFINE(pthread_condattr_init, int)
int __wrap_pthread_condattr_init(pthread_condattr_t * __attr) {
    CELIX_EI_IMPL(pthread_condattr_init);
    return __real_pthread_condattr_init(__attr);
}

int __real_pthread_condattr_setclock(pthread_condattr_t * __attr, clockid_t __clock_id);
CELIX_EI_DEFINE(pthread_condattr_setclock, int)
int __wrap_pthread_condattr_setclock(pthread_condattr_t * __attr, clockid_t __clock_id) {
    CELIX_EI_IMPL(pthread_condattr_setclock);
    return __real_pthread_condattr_setclock(__attr, __clock_id);
}

int __real_pthread_condattr_setpshared(pthread_condattr_t * __attr, int __pshared);
CELIX_EI_DEFINE(pthread_condattr_setpshared, int)
int __wrap_pthread_condattr_setpshared(pthread_condattr_t * __attr, int __pshared) {
    CELIX_EI_IMPL(pthread_condattr_setpshared);
    return __real_pthread_condattr_setpshared(__attr, __pshared);
}

int __real_pthread_cond_init(pthread_cond_t * __cond, const pthread_condattr_t * __cond_attr);
CELIX_EI_DEFINE(pthread_cond_init, int)
int __wrap_pthread_cond_init(pthread_cond_t * __cond, const pthread_condattr_t * __cond_attr) {
    CELIX_EI_IMPL(pthread_cond_init);
    return __real_pthread_cond_init(__cond, __cond_attr);
}

int __real_pthread_cond_timedwait(pthread_cond_t * __cond, pthread_mutex_t * __mutex, const struct timespec * __abstime);
CELIX_EI_DEFINE(pthread_cond_timedwait, int)
int __wrap_pthread_cond_timedwait(pthread_cond_t * __cond, pthread_mutex_t * __mutex, const struct timespec * __abstime) {
    CELIX_EI_IMPL(pthread_cond_timedwait);
    return __real_pthread_cond_timedwait(__cond, __mutex, __abstime);
}

}