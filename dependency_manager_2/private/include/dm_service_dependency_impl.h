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
 * dm_service_dependency.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_SERVICE_DEPENDENCY_IMPL_H_
#define DM_SERVICE_DEPENDENCY_IMPL_H_

#include <stdbool.h>

#include "dm_event.h"
#include "service_tracker.h"
#include "service_tracker_customizer.h"

#include "dm_service_dependency.h"

struct dm_service_dependency {
	dm_component_pt component;
	bool available;
	bool instanceBound;
	bool required;

	service_add_fpt add;
	service_change_fpt change;
	service_remove_fpt remove;
	service_swap_fpt swap;

	void **autoConfigure;

	bool isStarted;

	char *tracked_service_name;
	char *tracked_filter_unmodified;
	char *tracked_filter;

	service_tracker_pt tracker;
	service_tracker_customizer_pt tracker_customizer;
};

celix_status_t serviceDependency_start(dm_service_dependency_pt dependency);
celix_status_t serviceDependency_stop(dm_service_dependency_pt dependency);
celix_status_t serviceDependency_setInstanceBound(dm_service_dependency_pt dependency, bool instanceBound);
celix_status_t serviceDependency_setAvailable(dm_service_dependency_pt dependency, bool available);

celix_status_t serviceDependency_setComponent(dm_service_dependency_pt dependency, dm_component_pt component);
//celix_status_t serviceDependency_removeComponent(dm_service_dependency_pt dependency, dm_component_pt component);

celix_status_t serviceDependency_invokeAdd(dm_service_dependency_pt dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeChange(dm_service_dependency_pt dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeRemove(dm_service_dependency_pt dependency, dm_event_pt event);
celix_status_t serviceDependency_invokeSwap(dm_service_dependency_pt dependency, dm_event_pt event, dm_event_pt newEvent);
celix_status_t serviceDependency_isAvailable(dm_service_dependency_pt dependency, bool *available);
celix_status_t serviceDependency_isRequired(dm_service_dependency_pt dependency, bool *required);
celix_status_t serviceDependency_isInstanceBound(dm_service_dependency_pt dependency, bool *instanceBound);


#endif /* DM_SERVICE_DEPENDENCY_IMPL_H_ */
