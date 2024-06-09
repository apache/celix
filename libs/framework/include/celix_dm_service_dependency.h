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

#ifndef CELIX_DM_SERVICE_DEPENDENCY_H_
#define CELIX_DM_SERVICE_DEPENDENCY_H_

#include "celix_types.h"
#include "celix_errno.h"
#include "celix_threads.h"
#include "celix_dm_info.h"
#include "celix_framework_export.h"
#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum celix_dm_service_dependency_strategy_enum {
	DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING,
	DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND
} celix_dm_service_dependency_strategy_t;

typedef int (*celix_dm_service_update_fp)(void *handle, void* service);
typedef int (*celix_dm_service_swap_fp)(void *handle, void* oldService, void* newService);

typedef int (*celix_dm_service_update_with_props_fp)(void *handle, void* service, const celix_properties_t *props);
typedef int (*celix_dm_service_swap_with_props_fp)(void *handle, void* oldService, void* newService, const celix_properties_t *newProps);

typedef struct celix_dm_service_dependency_callback_options {
	celix_dm_service_update_fp set;
	celix_dm_service_update_fp add;
	celix_dm_service_update_fp remove;
	celix_dm_service_swap_fp swap; //not used, deprecated

	celix_dm_service_update_with_props_fp setWithProps;
	celix_dm_service_update_with_props_fp addWithProps;
	celix_dm_service_update_with_props_fp removeWithProps;
	celix_dm_service_swap_with_props_fp swapWithProps; //not used, deprecated
} celix_dm_service_dependency_callback_options_t;

#define CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS { .set = NULL, \
    .add = NULL, \
    .remove = NULL, \
    .swap = NULL, \
    .setWithProps = NULL, \
    .addWithProps = NULL, \
    .removeWithProps = NULL, \
    .swapWithProps = NULL }

/**
 * Create a service dependency.
 * Caller has ownership.
 *
 * \warning The dmServiceDependency is not thread safe when constructing or modifying.
 *          The handling of service updates is thread safe.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_service_dependency_t* celix_dmServiceDependency_create(void);

/**
 * Destroys a service dependency.
 * Will normally be done the by the DM Component.
 *
 * Can only be called if the serviceDependency is disabled (note that a service dependency not added to a
 * component is disabled).
 */
CELIX_FRAMEWORK_EXPORT void celix_dmServiceDependency_destroy(celix_dm_service_dependency_t *dep);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_dm_service_dependency_t, celix_dmServiceDependency_destroy);

/**
 * Specify if the service dependency is required. default is false
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required);

/**
 * Specify if the service dependency update strategy.
 *
 * The DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING strategy notifies the component in case the dependencies set
 * changes (e.g. a dependency is added/removed): the component is responsible for protecting via locks
 * the dependencies list and check (always under lock) if the service he's depending on is still available.
 *
 * The DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND (default when no strategy is explicitly set) reliefs the programmer
 * from dealing with service dependencies' consistency issues: in case this strategy is adopted, the component
 * is stopped and restarted (i.e. temporarily suspended) upon service dependencies' changes.
 *
 * Default strategy is DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setStrategy(celix_dm_service_dependency_t *dependency, celix_dm_service_dependency_strategy_t strategy);

/**
 * Return the service dependency update strategy.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_service_dependency_strategy_t celix_dmServiceDependency_getStrategy(celix_dm_service_dependency_t *dependency);

/**
 * Set the service name, version range and filter.
 *
 * @param serviceName The service name. Must have a value.
 * @param serviceVersionRange The service version range, can be a NULL pointer.
 * @param filter The (additional) filter to use (e.g. "(location=front)"). Can be a NULL pointer.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter);

/**
 * Returns the service dependency filter.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_dmServiceDependency_getFilter(celix_dm_service_dependency_t *dependency);

/**
 * Set the set callbacks when services specified by the service dependency
 * The first argument of the callbacks will be the component implement (@see component_getImplementation)
 * The second the argument a pointer to an instance of a service struct of the specified service dependency.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setCallback(celix_dm_service_dependency_t *dependency, celix_dm_service_update_fp set);

/**
 * Set the set function callbacks when services specified by the service dependency
 * The first argument of the callbacks will be the component implement (@see component_getImplementation)
 * The second argument of th callbacks will be a pointer to an instance of a service struct of the specified service dependency.
 * The third argument of th callbacks will be a pointer to a service properties of the a service instance of the specified service dependency.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setCallbackWithProperties(celix_dm_service_dependency_t *dependency, celix_dm_service_update_with_props_fp set);

/**
 * Set the set, add, change, remove and swap function callbacks when services specified by the service dependency
 * are (respectively) set, added, changed, removed or swapped.
 *
 * The version with the WithProps suffix will be called with as third argument the service properties.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setCallbacksWithOptions(celix_dm_service_dependency_t *dependency, const celix_dm_service_dependency_callback_options_t *opts);

/**
 * Set the callback handle to be used in the callbacks. Note that this normally should not be set, because the
 * result of component_getImplementation() is used
 * This can be used in rare cases when the callbacks are actually interceptors. e.g. in the case of C++ support.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmServiceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle);

/**
 * Creates a service dependency info. The service dependency info struct contains information about the service dependency.
 * The caller is the owner
 */
CELIX_FRAMEWORK_EXPORT dm_service_dependency_info_t* celix_dmServiceDependency_createInfo(celix_dm_service_dependency_t* dep);

/**
 * Destroy a provided service dependency info struct.
 */
CELIX_FRAMEWORK_EXPORT void celix_dmServiceDependency_destroyInfo(celix_dm_service_dependency_t *dep, dm_service_dependency_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_DM_SERVICE_DEPENDENCY_H_ */
