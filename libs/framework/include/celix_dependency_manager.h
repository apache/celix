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
 * The `celix_dependencyManager_add`, `celix_dependencyManager_remove` and `celix_dependencyManager_removeAllComponents`
 * funcions for celix_dependency_manager_t should be called outside the Celix event thread.
 * Note that bundle activators are started and stopped outside the Celix event thread and thus these
 * functions can be used in bundle activators.
 *
 * Inside the Celix event thread - i.e. during a useService callback or add/rem/set call from a service tracker -
 * the async versions should be used. After a async function has returned, service registration and opening
 * service trackers are (possibly) still in progress.
 */


/**
 * Adds a DM component to the dependency manager
 *
 * After this call the components will be build and if the components can be started, they
 * will be started and the services will be registered.
 *
 * After this call the dependency manager has ownership of the component and the dependency manager will also
 * de-active and destroy the component when the dependency manager is destroyed.
 *
 * Should not be called from the Celix event thread.
 */
celix_status_t celix_dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Same as celix_dependencyManager_add, but this call will not wait until all service registrations and
 * tracker are registered/opened on the Celix event thread.
 * Can be called on the Celix event thread.
 */
celix_status_t celix_dependencyManager_addAsync(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Removes a DM component from the dependency manager and destroys it
 *
 * After this call - and if the component was found - the component will be destroyed and if the component was started,
 * the component will have been stopped and de-initialized.
 *
 * Should not be called from the Celix event thread.
 */
celix_status_t celix_dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component);

/**
 * Same as celix_dependencyManager_remove, but this call will not wait until component is deactivated.
 * Can be called on the Celix event thread.
 *
 * The doneCallback will be called (if not NULL) with doneData as argument when the component is removed
 * and destroyed.
 *
 */
celix_status_t celix_dependencyManager_removeAsync(
        celix_dependency_manager_t *manager,
        celix_dm_component_t *component,
        void* doneData,
        void (*doneCallback)(void* data));

/**
 * Removes all DM components from the dependency manager.
 *
 * Should not be called from the Celix event thread.
 */
celix_status_t celix_dependencyManager_removeAllComponents(celix_dependency_manager_t *manager);

/**
 * Same as celix_dependencyManager_removeAllComponents, but this call will not wait til all
 * service registration and service trackers are unregistered/closed.
 *
 * The doneCallback will be called (if not NULL) with doneData as argument when the all component are removed
 * and destroyed.
 *
 * Can be called on the Celix event thread.
 */
celix_status_t celix_dependencyManager_removeAllComponentsAsync(celix_dependency_manager_t *manager, void *doneData, void (*doneCallback)(void *data));

/**
 * Check if all components for the bundle of the dependency manager are active (all required dependencies resolved).
 */
bool celix_dependencyManager_areComponentsActive(celix_dependency_manager_t *manager);

/**
 * Check if all components - for all bundles - are active (all required dependencies resolved).
 */
bool celix_dependencyManager_allComponentsActive(celix_dependency_manager_t *manager);

/**
 * Return the nr of components for this dependency manager
 */
size_t celix_dependencyManager_nrOfComponents(celix_dependency_manager_t *manager);

/**
 * Wait for an empty Celix event queue.
 * Should not be called on the Celix event queue thread.
 *
 * Can be used to ensure that all created/updated components are completely processed (services registered
 * and/or service trackers are created).
 */
void celix_dependencyManager_wait(celix_dependency_manager_t* manager);

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
 * Create and returns a dependency manager info struct for all started bundles.
 * The dependency manager info contains information about the state of the dependency manager components
 *
 * Caller has ownership of the return values (use celix_arrayList_destroy to free the memory).
 *
 * @param manager The dependency manager
 * @returns A Celix array of dependency manager infos (celix_dependency_manager_info_t*)
 */
celix_array_list_t * /*celix_dependency_manager_info_t* entries*/ celix_dependencyManager_createInfos(celix_dependency_manager_t *manager);

/**
 * Destroys a DM info struct.
 */
void celix_dependencyManager_destroyInfo(celix_dependency_manager_t *manager, celix_dependency_manager_info_t *info);

/**
 * Destroys a celix array list of  DM info structs.
 * @deprecated use celix_arrayList_destroy instead.
 */
void celix_dependencyManager_destroyInfos(celix_dependency_manager_t *manager, celix_array_list_t * infos /*entries celix_dependency_manager_info_t*/);

/**
 * Print the dependency manager info for all bundles to the provided output stream.
 * @param manager The dependency manager.
 * @param fullInfo Whether to print the full info or summary.
 * @param useAnsiColors Whether to use ansi colors when printing info.
 * @param stream The output stream (e.g. stdout)
 */
void celix_dependencyManager_printInfo(celix_dependency_manager_t* manager, bool fullInfo, bool useAnsiColors, FILE* stream);

/**
 * Print the dependency manager info for the provided bundle id to the provided output stream.
 * @param manager The dependency manager.
 * @param fullInfo whether to print the full info or summary.
 * @param useAnsiColors Whether to use ansi colors when printing info.
 * @param bundleId The bundle id for which the dependency manager info should be printed.
 * @param stream The output stream (e.g. stdout)
 */
void celix_dependencyManager_printInfoForBundle(celix_dependency_manager_t* manager, bool fullInfo, bool useAnsiColors, long bundleId, FILE* stream);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_DEPENDENCY_MANAGER_H_ */
