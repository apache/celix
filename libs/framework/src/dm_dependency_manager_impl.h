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

#ifndef CELIX_DM_DEPENDENCY_MANAGER_IMPL_H
#define CELIX_DM_DEPENDENCY_MANAGER_IMPL_H

#include "celix_array_list.h"
#include "celix_bundle_context.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct celix_dependency_manager {
    celix_bundle_context_t *ctx;
    celix_array_list_t *components;
    pthread_mutex_t mutex;
};

celix_dependency_manager_t* celix_private_dependencyManager_create(celix_bundle_context_t *context);
void celix_private_dependencyManager_destroy(celix_dependency_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif //CELIX_DM_DEPENDENCY_MANAGER_IMPL_H
