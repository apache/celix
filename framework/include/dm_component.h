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
/*
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
#include "dm_info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dm_component_state_enum {
    DM_CMP_STATE_INACTIVE = 1,
    DM_CMP_STATE_WAITING_FOR_REQUIRED = 2,
    DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED = 3,
    DM_CMP_STATE_TRACKING_OPTIONAL = 4,
} dm_component_state_t;

#define DM_COMPONENT_MAX_ID_LENGTH 64
#define DM_COMPONENT_MAX_NAME_LENGTH 128

typedef int (*init_fpt)(void *userData);
typedef int (*start_fpt)(void *userData);
typedef int (*stop_fpt)(void *userData);
typedef int (*deinit_fpt)(void *userData);

/**
 * Creates a DM Component
 * Caller has ownership.
 */
celix_status_t component_create(bundle_context_t *context, const char* name, dm_component_t **component);

/**
 * Destroys a DM Component
 */
void component_destroy(dm_component_t *component);


/**
 * Specify if a default 'service.lang=C' should be added to the properties of interfaces if no 'service.lang' has been
 * provided. Default is false. Note that this should be set before using component_addInterface.
 */
celix_status_t component_setCLanguageProperty(dm_component_t *component, bool setCLangProp);


/**
 * Adds a C interface to provide as service to the Celix framework.
 *
 * @param serviceName the service name.
 * @param version The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 */
celix_status_t component_addInterface(dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, properties_t *properties);

/**
 * Sets the implementation of the component. e.g. the component handle/self/this pointer.
 */
celix_status_t component_setImplementation(dm_component_t *component, void* implementation);

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
celix_status_t component_getInterfaces(dm_component_t *component, array_list_t **servicesNames);

/**
 * Adds a C service dependency to the component
 */
celix_status_t component_addServiceDependency(dm_component_t *component, dm_service_dependency_t *dep);

/**
 * Removes a C service dependency to the component
 */
celix_status_t component_removeServiceDependency(dm_component_t *component, dm_service_dependency_t *dependency);

/**
 * Returns the current state of the component.
 */
dm_component_state_t component_currentState(dm_component_t *cmp);

/**
 * Returns the implementation of the component. e.g. the component handle/self/this pointer.
 */
void * component_getImplementation(dm_component_t *cmp);

/**
 * Returns the DM component name. This is used when printing information about the component.
 */
const char * component_getName(dm_component_t *cmp);

/**
 * Returns bundle context for the bundle where this DM component is part of.
 */
celix_status_t component_getBundleContext(dm_component_t *component, bundle_context_t **out);

/**
 * Set the component life cycle callbacks.
 * The first argument will be the component implementation (@see component_getImplementation)
 */
celix_status_t component_setCallbacks(dm_component_t *component, init_fpt init, start_fpt start, stop_fpt stop, deinit_fpt deinit);

/**
 * Set the component life cycle callbacks using a MACRO for improving the type safety.
 */
#define component_setCallbacksSafe(dmCmp, type, init, start, stop, deinit) \
    do {  \
        int (*tmp_init)(type)   = (init); \
        int (*tmp_start)(type)  = (start); \
        int (*tmp_stop)(type)   = (stop); \
        int (*tmp_deinit)(type) = (deinit); \
        component_setCallbacks((dmCmp), (init_fpt)tmp_init, (start_fpt)tmp_start, (stop_fpt)tmp_stop, (deinit_fpt)tmp_deinit); \
    } while(0)

/**
 * Create a DM Component info struct. Containing information about the component.
 * Caller has ownership.
 */
celix_status_t component_getComponentInfo(dm_component_t *component, dm_component_info_pt *info);

/**
 * Destroys a DM Component info struct.
 */
void component_destroyComponentInfo(dm_component_info_pt info);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_H_ */
