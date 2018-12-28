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

#ifndef CELIX_DEPENDENCY_MANAGER_H_
#define CELIX_DEPENDENCY_MANAGER_H_

#include "celix_types.h"

#include "celix_errno.h"
#include "celix_array_list.h"
#include "celix_dm_info.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Adds a DM component to the dependency manager
 */
celix_status_t celix_dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Removes a DM component from the dependency manager and destroys it
 */
celix_status_t celix_dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Removes all DM components from the dependency manager
 */
celix_status_t celix_dependencyManager_removeAllComponents(celix_dependency_manager_t *manager);

/**
 * Create and returns a dependency manager info struct for the specified bundle.
 * The dependency manager info contains information about the state of the dependency manager components
 *
 * Caller has ownership of the return value (use celix_dependencyManager_destroyInfo to free the memory).
 *
 * @param manager The dependency manager
 * @param bndId The bundle id to get the info from.
 * @returns The dependency manager info for the provided bundle id or NULL if the bundle id is invalid.
 */
celix_dependency_manager_info_t* celix_dependencyManager_createInfo(celix_dependency_manager_t *manager, long bndId);

/**
 * Create and returns a dependency manager info structd for all started bundles.
 * The dependency manager info contains information about the state of the dependency manager components
 *
 * Caller has ownership of the return values (use celix_dependencyManager_destroyInfos to free the memory).
 *
 * @param manager The dependency manager
 * @returns A celix array of dependency manager infos for the provided bundle id or NULL if the bundle id is invalid.
 */
celix_array_list_t * /*celix_dependency_manager_info_t entries*/ celix_dependencyManager_createInfos(celix_dependency_manager_t *manager);

/**
 * Destroys a DM info struct.
 */
void celix_dependencyManager_destroyInfo(celix_dependency_manager_t *manager, celix_dependency_manager_info_t *info);

/**
 * Destroys a celix array list of  DM info structs.
 */
void celix_dependencyManager_destroyInfos(celix_dependency_manager_t *manager, celix_array_list_t * infos /*entries celix_dependency_manager_info_t*/);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_DEPENDENCY_MANAGER_H_ */
