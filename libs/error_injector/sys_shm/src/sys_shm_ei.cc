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
#include "sys_shm_ei.h"
#include "celix_error_injector.h"
#include <sys/shm.h>

extern "C" {
int __real_shmget(key_t __key, size_t __size, int __shmflg);
CELIX_EI_DEFINE(shmget, int)
int __wrap_shmget(key_t __key, size_t __size, int __shmflg) {
    CELIX_EI_IMPL(shmget);
    return __real_shmget(__key, __size, __shmflg);
}

void *__real_shmat(int __shmid, const void *__shmaddr, int __shmflg);
CELIX_EI_DEFINE(shmat, void *)
void *__wrap_shmat(int __shmid, const void *__shmaddr, int __shmflg) {
    CELIX_EI_IMPL(shmat);
    return __real_shmat(__shmid, __shmaddr, __shmflg);
}

}