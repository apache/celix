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
 * dm_component.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <stdbool.h>

#include "celix_types.h"
#include "celix_errno.h"
#include "properties.h"
#include "array_list.h"
#include "celix_dm_info.h"
#include "celix_dm_component.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_dm_component_state_enum dm_component_state_t CELIX_DEPRECATED_ATTR; //deprecated

#define DM_COMPONENT_MAX_ID_LENGTH 64
#define DM_COMPONENT_MAX_NAME_LENGTH 128

typedef int (*init_fpt)(void *userData) CELIX_DEPRECATED_ATTR;
typedef int (*start_fpt)(void *userData) CELIX_DEPRECATED_ATTR;
typedef int (*stop_fpt)(void *userData) CELIX_DEPRECATED_ATTR;
typedef int (*deinit_fpt)(void *userData) CELIX_DEPRECATED_ATTR;

/**
 * Creates a DM Component
 * Caller has ownership.
 */
celix_status_t component_create(celix_bundle_context_t *context, const char* name, celix_dm_component_t **component) CELIX_DEPRECATED_ATTR;

/**
 * Destroys a DM Component
 */
void component_destroy(celix_dm_component_t *component) CELIX_DEPRECATED_ATTR;


/**
 * @deprecated Deprecated. not used ny mowre
 */
celix_status_t component_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp) CELIX_DEPRECATED_ATTR;


/**
 * Adds a C interface to provide as service to the Celix framework.
 *
 * @param serviceName the service name.
 * @param version The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 */
celix_status_t component_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, celix_properties_t *properties) CELIX_DEPRECATED_ATTR;

/**
 * Removed  a C interface from a component the Celix framework.
 *
 * @param serviceName the service name.
 * @return CELIX_SUCCESS when removed, CELIX_ILLEGAL_ARGUMENT when the component does not provide the interface
 */
celix_status_t component_removeInterface(celix_dm_component_t *component, const void* service) CELIX_DEPRECATED_ATTR;
/**
 * Sets the implementation of the component. e.g. the component handle/self/this pointer.
 */
celix_status_t component_setImplementation(celix_dm_component_t *component, void* implementation) CELIX_DEPRECATED_ATTR;

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
celix_status_t component_getInterfaces(celix_dm_component_t *component, celix_array_list_t **servicesNames) CELIX_DEPRECATED_ATTR;

/**
 * Adds a C service dependency to the component
 */
celix_status_t component_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) CELIX_DEPRECATED_ATTR;

/**
 * Removes a C service dependency to the component
 */
celix_status_t component_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) CELIX_DEPRECATED_ATTR;

/**
 * Returns the current state of the component.
 */
celix_dm_component_state_t component_currentState(celix_dm_component_t *cmp) CELIX_DEPRECATED_ATTR;

/**
 * Returns the implementation of the component. e.g. the component handle/self/this pointer.
 */
void * component_getImplementation(celix_dm_component_t *cmp) CELIX_DEPRECATED_ATTR;

/**
 * Returns the DM component name. This is used when printing information about the component.
 */
const char * component_getName(celix_dm_component_t *cmp) CELIX_DEPRECATED_ATTR;

/**
 * Returns bundle context for the bundle where this DM component is part of.
 */
celix_status_t component_getBundleContext(celix_dm_component_t *component, celix_bundle_context_t **out) CELIX_DEPRECATED_ATTR;

/**
 * Set the component life cycle callbacks.
 * The first argument will be the component implementation (@see component_getImplementation)
 */
celix_status_t component_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit) CELIX_DEPRECATED_ATTR;

/**
 * Set the component life cycle callbacks using a MACRO for improving the type safety.
 */
#define component_setCallbacksSafe(dmCmp, type, init, start, stop, deinit) \
    do {  \
        int (*tmp_init)(type)   = (init); \
        int (*tmp_start)(type)  = (start); \
        int (*tmp_stop)(type)   = (stop); \
        int (*tmp_deinit)(type) = (deinit); \
        component_setCallbacks((dmCmp), (celix_dm_cmp_lifecycle_fpt)tmp_init, (celix_dm_cmp_lifecycle_fpt)tmp_start, (celix_dm_cmp_lifecycle_fpt)tmp_stop, (celix_dm_cmp_lifecycle_fpt)tmp_deinit); \
    } while(0)

/**
 * Create a DM Component info struct. Containing information about the component.
 * Caller has ownership.
 */
celix_status_t component_getComponentInfo(celix_dm_component_t *component, dm_component_info_pt *info) CELIX_DEPRECATED_ATTR;

/**
 * Destroys a DM Component info struct.
 */
void component_destroyComponentInfo(dm_component_info_pt info) CELIX_DEPRECATED_ATTR;

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_H_ */
