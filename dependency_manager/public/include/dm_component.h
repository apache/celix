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

#include <bundle_context.h>
#include <celix_errno.h>

#include "dm_service_dependency.h"

typedef struct dm_component_struct *dm_component_pt;

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

celix_status_t component_create(bundle_context_pt context, const char *name, dm_component_pt *component);
void component_destroy(dm_component_pt component);

celix_status_t component_addInterface(dm_component_pt component, char *serviceName, void *service, properties_pt properties);
celix_status_t component_setImplementation(dm_component_pt component, void *implementation);

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
celix_status_t component_getInterfaces(dm_component_pt component, array_list_pt *servicesNames);

celix_status_t component_addServiceDependency(dm_component_pt component, dm_service_dependency_pt dep);
celix_status_t component_removeServiceDependency(dm_component_pt component, dm_service_dependency_pt dependency);

dm_component_state_t component_currentState(dm_component_pt cmp);
void * component_getImplementation(dm_component_pt cmp);
const char * component_getName(dm_component_pt cmp);

#define component_setCallbacksSafe(dmCmp, type, init, start, stop, deinit) \
    do {  \
        int (*tmp_init)(type)   = (init); \
        int (*tmp_start)(type)  = (start); \
        int (*tmp_stop)(type)   = (stop); \
        int (*tmp_deinit)(type) = (deinit); \
        component_setCallbacks((dmCmp), (init_fpt)tmp_init, (start_fpt)tmp_start, (stop_fpt)tmp_stop, (deinit_fpt)tmp_deinit); \
    } while(0)

celix_status_t component_setCallbacks(dm_component_pt component, init_fpt init, start_fpt start, stop_fpt stop, deinit_fpt deinit);

/**
 * returns a dm_component_info_pt. Caller has ownership.
 */
celix_status_t component_getComponentInfo(dm_component_pt component, dm_component_info_pt *info);
void component_destroyComponentInfo(dm_component_info_pt info);


#endif /* COMPONENT_H_ */
