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

#ifndef CELIX_COMPONENT_H_
#define CELIX_COMPONENT_H_

#include <stdbool.h>

#include "celix_types.h"
#include "celix_errno.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_dm_info.h"
#include "celix_framework_export.h"
#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_DM_COMPONENT_UUID     "component.uuid"

typedef enum celix_dm_component_state_enum {
    CELIX_DM_CMP_STATE_INACTIVE =                               1,
    CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED =                   2,
    CELIX_DM_CMP_STATE_INITIALIZING =                           3,
    CELIX_DM_CMP_STATE_DEINITIALIZING =                         4,
    CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED =   5,
    CELIX_DM_CMP_STATE_STARTING =                               6,
    CELIX_DM_CMP_STATE_STOPPING =                               7,
    CELIX_DM_CMP_STATE_TRACKING_OPTIONAL =                      8,
    CELIX_DM_CMP_STATE_SUSPENDING =                             9,
    CELIX_DM_CMP_STATE_SUSPENDED =                              10,
    CELIX_DM_CMP_STATE_RESUMING =                               11,

    /**
     * Note this dm state enums are deprecated, but for
     * now still supported.
     */
    DM_CMP_STATE_INACTIVE =                                     1,
    DM_CMP_STATE_WAITING_FOR_REQUIRED =                         2,
    DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED =        5,
    DM_CMP_STATE_TRACKING_OPTIONAL =                            8,
} celix_dm_component_state_t;

#define CELIX_DM_COMPONENT_MAX_ID_LENGTH 64
#define CELIX_DM_COMPONENT_MAX_NAME_LENGTH 128

typedef int (*celix_dm_cmp_lifecycle_fpt)(void *userData);
typedef void (*celix_dm_cmp_impl_destroy_fpt)(void*);

/**
 * Creates a DM Component with a random generated UUID.
 * Caller has ownership.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_component_t* celix_dmComponent_create(celix_bundle_context_t *context, const char* name);

/**
 * Creates a DM Component with a provided UUID.
 * Caller has ownership.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_component_t* celix_dmComponent_createWithUUID(celix_bundle_context_t *context, const char* name, const char* UUID);

/**
 * Get the UUID of the component.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_dmComponent_getUUID(celix_dm_component_t* cmp);

/**
 * Destroys a DM Component
 */
CELIX_FRAMEWORK_EXPORT void celix_dmComponent_destroy(celix_dm_component_t* cmp);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_dm_component_t, celix_dmComponent_destroy);

/**
 * Destroys a DM Component on the event thread.
 * Will call doneCallback (if not NULL) when done.
 *
 */
CELIX_FRAMEWORK_EXPORT void celix_dmComponent_destroyAsync(celix_dm_component_t* cmp, void *doneData, void (*doneCallback)(void*));

/**
 * Specify if a default 'service.lang=C' should be added to the properties of interfaces if no 'service.lang' has been
 * provided. Default is false. Note that this should be set before using component_addInterface.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t celix_dmComponent_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp);


/**
 * Adds a C interface to provide as service to the Celix framework.
 *
 * @param serviceName the service name.
 * @param version The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, celix_properties_t *properties);

/**
 * Removed  a C interface from a component the Celix framework.
 *
 * @param serviceName the service name.
 * @return CELIX_SUCCESS when removed, CELIX_ILLEGAL_ARGUMENT when the component does not provide the interface
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_removeInterface(celix_dm_component_t *component, const void* service);
/**
 * Sets the implementation of the component. e.g. the component handle/self/this pointer.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_setImplementation(celix_dm_component_t *component, void* implementation);

/**
 * Configures the destroy function for the component implementation.
 *
 * If a destroy function for the component implementation is configured, this will be used
 * when the component is removed from the dependency manager and component is successfully de-activated.
 *
 * The destroy function will not be called if the component implementation is not set. e.g. if the
 * celix_dmComponent_setImplementation is not called with a non NULL value.
 */
CELIX_FRAMEWORK_EXPORT void celix_dmComponent_setImplementationDestroyFunction(celix_dm_component_t* cmp, celix_dm_cmp_impl_destroy_fpt destroyFn);

/**
 * Configures the destroy function for the component implementation using a MACRO for improving the type safety.
 */
#define CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(dmCmp, type, destroy) \
    do {  \
        void (*_destroyFunction)(type*) = (destroy); \
        celix_dmComponent_setImplementationDestroyFunction((dmCmp), (void(*)(void*))_destroyFunction); \
    } while(0)

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_getInterfaces(celix_dm_component_t *component, celix_array_list_t **servicesNames);

/**
 * Adds a C service dependency to the component
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep);

/**
 * Removes a C service dependency to the component
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);

/**
 * Returns the current state of the component.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_component_state_t celix_dmComponent_currentState(celix_dm_component_t* cmp);

/**
 * Returns the implementation of the component. e.g. the component handle/self/this pointer.
 */
CELIX_FRAMEWORK_EXPORT void * celix_dmComponent_getImplementation(celix_dm_component_t* cmp);

/**
 * Returns the configured component implementation destroy function.
 */
CELIX_FRAMEWORK_EXPORT celix_dm_cmp_impl_destroy_fpt celix_dmComponent_getImplementationDestroyFunction(celix_dm_component_t* cmp);

/**
 * Returns the DM component name. This is used when printing information about the component.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_dmComponent_getName(celix_dm_component_t* cmp);

/**
 * Returns bundle context for the bundle where this DM component is part of.
 */
CELIX_FRAMEWORK_EXPORT celix_bundle_context_t* celix_dmComponent_getBundleContext(celix_dm_component_t *component);

/**
 * Set the component life cycle callbacks.
 * The first argument will be the component implementation (@see component_getImplementation)
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit);

/**
 * Set the component life cycle callbacks using a MACRO for improving the type safety.
 */
#define CELIX_DM_COMPONENT_SET_CALLBACKS(dmCmp, type, init, start, stop, deinit) \
    do {  \
        int (*_tmp_init)(type*)   = (init); \
        int (*_tmp_start)(type*)  = (start); \
        int (*_tmp_stop)(type*)   = (stop); \
        int (*_tmp_deinit)(type*) = (deinit); \
        celix_dmComponent_setCallbacks((dmCmp), (celix_dm_cmp_lifecycle_fpt)_tmp_init, (celix_dm_cmp_lifecycle_fpt)_tmp_start, (celix_dm_cmp_lifecycle_fpt)_tmp_stop, (celix_dm_cmp_lifecycle_fpt)_tmp_deinit); \
    } while(0)


CELIX_FRAMEWORK_EXPORT bool celix_dmComponent_isActive(celix_dm_component_t *component);

/**
 * Returns the string value of a provided state
 */
CELIX_FRAMEWORK_EXPORT const char* celix_dmComponent_stateToString(celix_dm_component_state_t state);

/**
 * Deprecated, use CELIX_DM_COMPONENT_SET_CALLBACKS instead.
 */

#define CELIX_DMCOMPONENT_SETCALLBACKS(dmCmp, type, init, start, stop, deinit) \
    CELIX_DM_COMPONENT_SET_CALLBACKS(dmCmp, type*, init, start, stop, deinit)

/**
 * Create a DM Component info struct. Containing information about the component.
 * Caller has ownership.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_dmComponent_getComponentInfo(celix_dm_component_t *component, celix_dm_component_info_t** infoOut);

/**
 * Print the component info to the provided stream.
 * @param info The component info to print.
 * @param printFullInfo Whether to print the full info or summary.
 * @param useAnsiColors Whether to use ansi colors when printing the component info.
 * @param stream The stream to print to (e..g stdout).
 */
CELIX_FRAMEWORK_EXPORT void celix_dmComponent_printComponentInfo(celix_dm_component_info_t* info, bool printFullInfo, bool useAnsiColors, FILE* stream);

/**
 * Destroys a DM Component info struct.
 */
CELIX_FRAMEWORK_EXPORT void celix_dmComponent_destroyComponentInfo(dm_component_info_pt info);


#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_H_ */
