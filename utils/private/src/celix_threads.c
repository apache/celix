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

#include "celix_threads.h"

celix_status_t celixThread_create(celix_thread_t *new_thread, celix_thread_attr_t *attr, celix_thread_start_t func, void *data) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_create(new_thread, attr, func, data);

    return status;
}

celix_status_t celixThread_exit(void *exitStatus) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_exit(exitStatus);

    return status;
}


celix_status_t celixThreadMutext_create(celix_thread_mutex_t *mutex, celix_thread_mutexattr_t *attr) {
    pthread_mutex_init(mutex, attr);
    return CELIX_SUCCESS;
}
