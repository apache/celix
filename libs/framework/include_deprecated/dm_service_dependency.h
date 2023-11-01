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
 * dm_service_dependency.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_SERVICE_DEPENDENCY_H_
#define DM_SERVICE_DEPENDENCY_H_

#include "celix_types.h"
#include "celix_errno.h"
#include "celix_threads.h"
#include "service_reference.h"

#include "celix_dm_info.h"
#include "celix_dm_service_dependency.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum celix_dm_service_dependency_strategy_enum dm_service_dependency_strategy_t;

typedef int (*service_set_fpt)(void *handle, const void* service);
typedef int (*service_add_fpt)(void *handle, const void* service);
typedef int (*service_change_fpt)(void *handle, const void* service);
typedef int (*service_remove_fpt)(void *handle, const void* service);
typedef int (*service_swap_fpt)(void *handle, const void* oldService, const void* newService);

typedef celix_status_t (*service_set_with_ref_fpt)(void *handle, service_reference_pt reference, const void* service);
typedef celix_status_t (*service_add_with_ref_fpt)(void *handle, service_reference_pt reference, const void* service);
typedef celix_status_t (*service_change_with_ref_fpt)(void *handle, service_reference_pt reference, const void* service);
typedef celix_status_t (*service_remove_with_ref_fpt)(void *handle, service_reference_pt reference, const void* service);
typedef celix_status_t (*service_swap_with_ref_fpt)(void *handle, service_reference_pt oldReference, const void* oldService, service_reference_pt newReference, const void* newService);

/**
 * Create a service dependency.
 * Caller has ownership.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_create(celix_dm_service_dependency_t **dep);

/**
 * Destroys a service dependency.
 * Will normally be done the by the DM Component.
 *
 * Can only be called if the serviceDependency is disabled (note that a service dependency not added to a
 * component is disabled).
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_destroy(celix_dm_service_dependency_t **dep);

/**
 * Specify if the service dependency is required. default is false
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required);

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
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_setStrategy(celix_dm_service_dependency_t *dependency, celix_dm_service_dependency_strategy_t strategy);

/**
 * Return the service dependency update strategy.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_getStrategy(celix_dm_service_dependency_t *dependency, celix_dm_service_dependency_strategy_t* strategy);

/**
 * Set the service name, version range and filter.
 *
 * @param serviceName The service name. Must have a value.
 * @param serviceVersionRange The service version range, can be a NULL pointer.
 * @param filter The (additional) filter to use (e.g. "(location=front)"). Can be a NULL pointer.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter);

/**
 * Returns the service dependency filter.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_getFilter(celix_dm_service_dependency_t *dependency, const char** filter);

/**
 * Set the set, add, change, remove and swap function callbacks when services specified by the service dependency
 * are (respectively) set, added, changed, removed or swapped.
 * The first argument of the callbacks will be the component implement (@see component_getImplementation)
 * The second the argument a pointer to an instance of a service struct of the specified service dependency.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_setCallbacks(celix_dm_service_dependency_t *dependency, service_set_fpt set, service_add_fpt add, service_change_fpt change, service_remove_fpt remove, service_swap_fpt swap);

#define serviceDependency_setCallbacksSafe(dep, cmpType, servType, set, add, change, remove, swap) \
	do { \
		int (*tmpSet)(cmpType, servType) = set; \
		int (*tmpAdd)(cmpType, servType) = add; \
		int (*tmpChange)(cmpType, servType) = change; \
		int (*tmpRemove)(cmpType, servType) = remove; \
		int (*tmpSwap)(cmpType, servType, servType) = swap; \
		serviceDependency_setCallbacks((dep), (service_set_fpt)tmpSet, (service_add_fpt)tmpAdd, (service_change_fpt)tmpChange, (service_remove_fpt)tmpRemove, (service_swap_fpt)tmpSwap); \
	} while(0)

/**
 * Set the callback handle to be used in the callbacks. Note that this normally should not be set, because the
 * result of component_getImplementation() is used
 * This can be used in rare cases when the callbacks are actually interceptors. e.g. in the case of C++ support.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle);

/**
 * Creates a service dependency info. The service dependency info struct contains information about the service dependency.
 * The caller is the owner
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceDependency_getServiceDependencyInfo(celix_dm_service_dependency_t* dep, dm_service_dependency_info_t **info);

/**
 * Destroy a provided service dependency info struct.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT void dependency_destroyDependencyInfo(dm_service_dependency_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* DM_SERVICE_DEPENDENCY_H_ */
