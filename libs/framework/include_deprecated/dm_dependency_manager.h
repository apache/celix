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
/**
 * dm_dependency_manager.h
 *
 *  \date       22 Feb 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_DEPENDENCY_MANAGER_H_
#define DM_DEPENDENCY_MANAGER_H_

#include "celix_types.h"

#include "celix_errno.h"
#include "celix_dm_info.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a dependency manager.
 * Caller has ownership.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t dependencyManager_create(celix_bundle_context_t *context, celix_dependency_manager_t **manager);

/**
 * Destroys the provided dependency manager
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT void dependencyManager_destroy(celix_dependency_manager_t *manager);

/**
 * Adds a DM component to the dependency manager
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component)CELIX_DEPRECATED_ATTR;

/**
 * Removes a DM component from the dependency manager and destroys it
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Removes all DM components from the dependency manager
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t dependencyManager_removeAllComponents(celix_dependency_manager_t *manager);

/**
 * Create and returns a DM Info struct. Which contains information about the state of the DM components
 * Caller has ownership.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t dependencyManager_getInfo(celix_dependency_manager_t *manager, dm_dependency_manager_info_pt *info);

/**
 * Destroys a DM info struct.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT void dependencyManager_destroyInfo(celix_dependency_manager_t *manager, dm_dependency_manager_info_pt info);

#ifdef __cplusplus
}
#endif

#endif /* DM_DEPENDENCY_MANAGER_H_ */
