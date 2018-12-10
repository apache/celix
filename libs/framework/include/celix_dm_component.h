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

#ifndef CELIX_COMPONENT_H_
#define CELIX_COMPONENT_H_

#include <stdbool.h>

#include "celix_types.h"
#include "celix_errno.h"
#include "properties.h"
#include "array_list.h"
#include "celix_dm_info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_dm_component_state_enum {
    DM_CMP_STATE_INACTIVE = 1,
    DM_CMP_STATE_WAITING_FOR_REQUIRED = 2,
    DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED = 3,
    DM_CMP_STATE_TRACKING_OPTIONAL = 4,
} celix_dm_component_state_t;

#define CELIX_DM_COMPONENT_MAX_ID_LENGTH 64
#define CELIX_DM_COMPONENT_MAX_NAME_LENGTH 128

typedef int (*celix_dm_cmp_lifecycle_fpt)(void *userData);

/**
 * Creates a DM Component
 * Caller has ownership.
 */
celix_dm_component_t* celix_dmComponent_create(celix_bundle_context_t *context, const char* name);

/**
 * Destroys a DM Component
 */
void celix_dmComponent_destroy(celix_dm_component_t *cmp);

/**
 * Specify if a default 'service.lang=C' should be added to the properties of interfaces if no 'service.lang' has been
 * provided. Default is false. Note that this should be set before using component_addInterface.
 */
celix_status_t celix_dmComponent_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp);


/**
 * Adds a C interface to provide as service to the Celix framework.
 *
 * @param serviceName the service name.
 * @param version The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 */
celix_status_t celix_dmComponent_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, celix_properties_t *properties);

/**
 * Removed  a C interface from a component the Celix framework.
 *
 * @param serviceName the service name.
 * @return CELIX_SUCCESS when removed, CELIX_ILLEGAL_ARGUMENT when the component does not provide the interface
 */
celix_status_t celix_dmComponent_removeInterface(celix_dm_component_t *component, const void* service);
/**
 * Sets the implementation of the component. e.g. the component handle/self/this pointer.
 */
celix_status_t celix_dmComponent_setImplementation(celix_dm_component_t *component, void* implementation);

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
celix_status_t celix_dmComponent_getInterfaces(celix_dm_component_t *component, celix_array_list_t **servicesNames);

/**
 * Adds a C service dependency to the component
 */
celix_status_t celix_dmComponent_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep);

/**
 * Removes a C service dependency to the component
 */
celix_status_t celix_dmComponent_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);

/**
 * Returns the current state of the component.
 */
celix_dm_component_state_t celix_dmComponent_currentState(celix_dm_component_t *cmp);

/**
 * Returns the implementation of the component. e.g. the component handle/self/this pointer.
 */
void * celix_dmComponent_getImplementation(celix_dm_component_t *cmp);

/**
 * Returns the DM component name. This is used when printing information about the component.
 */
const char* celix_dmComponent_getName(celix_dm_component_t *cmp);

/**
 * Returns bundle context for the bundle where this DM component is part of.
 */
celix_bundle_context_t* celix_dmComponent_getBundleContext(celix_dm_component_t *component);

/**
 * Set the component life cycle callbacks.
 * The first argument will be the component implementation (@see component_getImplementation)
 */
celix_status_t celix_dmComponent_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit);

/**
 * Set the component life cycle callbacks using a MACRO for improving the type safety.
 */
#define CELIX_DMCOMPONENT_SETCALLBACKS(dmCmp, type, init, start, stop, deinit) \
    do {  \
        int (*tmp_init)(type)   = (init); \
        int (*tmp_start)(type)  = (start); \
        int (*tmp_stop)(type)   = (stop); \
        int (*tmp_deinit)(type) = (deinit); \
        celix_dmComponent_setCallbacks((dmCmp), (celix_dm_cmp_lifecycle_fpt)tmp_init, (celix_dm_cmp_lifecycle_fpt)tmp_start, (celix_dm_cmp_lifecycle_fpt)tmp_stop, (celix_dm_cmp_lifecycle_fpt)tmp_deinit); \
    } while(0)

/**
 * Create a DM Component info struct. Containing information about the component.
 * Caller has ownership.
 */
celix_status_t celix_dmComponent_getComponentInfo(celix_dm_component_t *component, dm_component_info_pt *info);

/**
 * Destroys a DM Component info struct.
 */
void celix_dmComponent_destroyComponentInfo(dm_component_info_pt info);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_H_ */
