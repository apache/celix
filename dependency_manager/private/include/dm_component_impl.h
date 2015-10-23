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
 * dm_component_impl.h
 *
 *  \date       22 Feb 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef COMPONENT_IMPL_H_
#define COMPONENT_IMPL_H_

#include "dm_component.h"

#include "dm_service_dependency_impl.h"

#include "dm_event.h"

typedef enum dm_component_state {
    DM_CMP_STATE_INACTIVE = 1,
    DM_CMP_STATE_WAITING_FOR_REQUIRED = 2,
    DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED = 3,
    DM_CMP_STATE_TRACKING_OPTIONAL = 4,
} dm_component_state_t;

typedef struct dm_executor * dm_executor_pt;

typedef struct dm_interface_struct {
    char *serviceName;
    void *service;
    properties_pt properties;
    service_registration_pt registration;
} dm_interface;

struct dm_component {
    char id[DM_COMPONENT_MAX_ID_LENGTH];
    char name[DM_COMPONENT_MAX_NAME_LENGTH];
    bundle_context_pt context;
    array_list_pt dm_interface;

    void *implementation;

    init_fpt callbackInit;
    start_fpt callbackStart;
    stop_fpt callbackStop;
    deinit_fpt callbackDeinit;

    array_list_pt dependencies;
    pthread_mutex_t mutex;

    dm_component_state_t state;
    bool isStarted;
    bool active;

    hash_map_pt dependencyEvents;

    dm_executor_pt executor;
};

celix_status_t component_start(dm_component_pt component);
celix_status_t component_stop(dm_component_pt component);

celix_status_t component_getBundleContext(dm_component_pt component, bundle_context_pt *context);

celix_status_t component_handleEvent(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event);

#endif /* COMPONENT_IMPL_H_ */
