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

#include "dm_activator.h"

#include <stdlib.h>


celix_status_t bundleActivator_create(celix_bundle_context_t *ctx, void **userData) {
    return dm_create(ctx, userData);
}

celix_status_t bundleActivator_start(void *userData, celix_bundle_context_t *ctx) {
    dm_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    return dm_init(userData, ctx, mng);
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *ctx) {
    dm_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    return dm_destroy(userData, ctx, mng);
}

celix_status_t bundleActivator_destroy(void * userData  __attribute__((unused)), celix_bundle_context_t *ctx __attribute__((unused))) {
    return CELIX_SUCCESS; //nothing to do
}
