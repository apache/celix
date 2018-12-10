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
 * dm_service_dependency_impl.h
 *
 *  \date       16 Oct 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_SERVICE_DEPENDENCY_IMPL_H_
#define DM_SERVICE_DEPENDENCY_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "dm_event.h"
#include "service_tracker.h"
#include "service_tracker_customizer.h"

#include "dm_service_dependency.h"
#include "dm_component.h"

struct celix_dm_service_dependency {
	celix_dm_component_t *component;
	bool available;
	bool instanceBound;
	bool required;
	dm_service_dependency_strategy_t strategy;

	void* callbackHandle; //This handle can be set to be used instead of the component implementation
	celix_dm_service_update_fp set;
	celix_dm_service_update_fp add;
	celix_dm_service_update_fp change;
	celix_dm_service_update_fp remove;
	celix_dm_service_swap_fp swap;

	service_set_with_ref_fpt set_with_ref;
	service_add_with_ref_fpt add_with_ref;
	service_change_with_ref_fpt change_with_ref;
	service_remove_with_ref_fpt remove_with_ref;
	service_swap_with_ref_fpt swap_with_ref;

	celix_dm_service_update_with_props_fp set_with_props;
	celix_dm_service_update_with_props_fp add_with_props;
	celix_dm_service_update_with_props_fp rem_with_props;
	celix_dm_service_swap_with_props_fp swap_with_props;

	const void **autoConfigure;
	celix_thread_mutex_t lock;

	bool isStarted;

	bool addCLanguageFilter;
	char *tracked_service;
	char *tracked_filter_unmodified;
	char *tracked_filter;

	service_tracker_pt tracker;
	service_tracker_customizer_pt tracker_customizer;
};

celix_status_t serviceDependency_start(celix_dm_service_dependency_t *dependency);
celix_status_t serviceDependency_stop(celix_dm_service_dependency_t *dependency);
celix_status_t serviceDependency_setInstanceBound(celix_dm_service_dependency_t *dependency, bool instanceBound);
celix_status_t serviceDependency_setAutoConfig(celix_dm_service_dependency_t *dependency, void **autoConfigure);
celix_status_t serviceDependency_setAvailable(celix_dm_service_dependency_t *dependency, bool available);

celix_status_t serviceDependency_setComponent(celix_dm_service_dependency_t *dependency, celix_dm_component_t *component);
//celix_status_t serviceDependency_removeComponent(celix_dm_service_dependency_t *dependency, celix_dm_component_t *component);

celix_status_t serviceDependency_invokeSet(celix_dm_service_dependency_t *dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeAdd(celix_dm_service_dependency_t *dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeChange(celix_dm_service_dependency_t *dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeRemove(celix_dm_service_dependency_t *dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeSwap(celix_dm_service_dependency_t *dependency, dm_event_pt event, dm_event_pt newEvent);
celix_status_t serviceDependency_isAvailable(celix_dm_service_dependency_t *dependency, bool *available);
celix_status_t serviceDependency_isRequired(celix_dm_service_dependency_t *dependency, bool *required);
celix_status_t serviceDependency_isInstanceBound(celix_dm_service_dependency_t *dependency, bool *instanceBound);
celix_status_t serviceDependency_isAutoConfig(celix_dm_service_dependency_t *dependency, bool *autoConfig);

celix_status_t serviceDependency_getAutoConfig(celix_dm_service_dependency_t *dependency, const void*** autoConfigure);
celix_status_t serviceDependency_unlock(celix_dm_service_dependency_t *dependency);
celix_status_t serviceDependency_lock(celix_dm_service_dependency_t *dependency);

#ifdef __cplusplus
}
#endif

#endif /* DM_SERVICE_DEPENDENCY_IMPL_H_ */
