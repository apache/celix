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

#ifndef DM_ACTIVATOR_BASE_H_
#define DM_ACTIVATOR_BASE_H_

#include "celix_bundle_context.h"
#include "dm_dependency_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Should be implemented by a bundle specific DM activator.
 * Should allocate and initialize a bundle specific activator struct.
 */
celix_status_t dm_create(celix_bundle_context_t *ctx, void ** userData);

/**
 * Should be implemented by a bundle specific DM activator.
 * Will be called after the dm_create function.
 * Can be used to specify with use of the provided dependency manager the bundle specific components.
 */
celix_status_t dm_init(void * userData, celix_bundle_context_t *ctx, dm_dependency_manager_t *mng);

/**
 * Should be implemented by a bundle specific DM activator.
 * Should deinitialize and deallocate the bundle specific activator struct.
 */
celix_status_t dm_destroy(void * userData, celix_bundle_context_t *ctx, dm_dependency_manager_t *mng);

#ifdef __cplusplus
}
#endif

#endif /* DM_ACTIVATOR_BASE_H_ */

