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


#ifndef DM_SERVICE_DEPENDENCY_IMPL_H_
#define DM_SERVICE_DEPENDENCY_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "celix_dm_event.h"
#include "service_tracker.h"
#include "service_tracker_customizer.h"

#include "dm_service_dependency.h"
#include "dm_component.h"

struct celix_dm_service_dependency_svc_entry {
    void* svc;
    const celix_properties_t* props;
};

struct celix_dm_service_dependency {
    celix_dm_service_update_fp set;
    celix_dm_service_update_fp add;
    celix_dm_service_update_fp remove;

    celix_dm_service_update_with_props_fp setWithProperties;
    celix_dm_service_update_with_props_fp addWithProperties;
    celix_dm_service_update_with_props_fp remWithProperties;

    char* serviceName;
    char* filter;
    char* versionRange;
    bool required;
    dm_service_dependency_strategy_t strategy;
    celix_dm_component_t* component;

    celix_thread_mutex_t mutex;        // protects below
    long svcTrackerId;                 // active tracker id
    size_t nrOfActiveStoppingTrackers; // nr of async stop tracker still active (should be 0 or 1)
    size_t trackedSvcCount;
    void* callbackHandle; // This handle can be set to be used instead of the component implementation
};

celix_status_t celix_dmServiceDependency_enable(celix_dm_service_dependency_t *dependency);
celix_status_t celix_dmServiceDependency_disable(celix_dm_service_dependency_t *dependency);

bool celix_dmServiceDependency_isDisabled(celix_dm_service_dependency_t *dependency);

celix_status_t celix_dmServiceDependency_setComponent(celix_dm_service_dependency_t *dependency, celix_dm_component_t *component);

celix_status_t celix_dmServiceDependency_invokeSet(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props);
celix_status_t celix_dmServiceDependency_invokeAdd(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props);
celix_status_t celix_dmServiceDependency_invokeRemove(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props);

bool celix_dmServiceDependency_isAvailable(celix_dm_service_dependency_t *dependency);
bool celix_dmServiceDependency_isRequired(const celix_dm_service_dependency_t* dependency);
bool celix_dmServiceDependency_isTrackerOpen(celix_dm_service_dependency_t* dependency);


bool celix_dmServiceDependency_isSetCallbackConfigured(celix_dm_service_dependency_t* dependency);
bool celix_dmServiceDependency_isAddRemCallbacksConfigured(celix_dm_service_dependency_t* dependency);

#ifdef __cplusplus
}
#endif

#endif /* DM_SERVICE_DEPENDENCY_IMPL_H_ */
