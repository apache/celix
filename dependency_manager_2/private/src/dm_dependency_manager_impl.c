/**
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

/*
 * dm_dependency_manager_impl.c
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#include <pthread.h>
#include <stdlib.h>

#include "bundle_context.h"
#include "dm_component_impl.h"

struct dm_dependency_manager {
	array_list_pt components;

	pthread_mutex_t mutex;
};

celix_status_t dependencyManager_create(bundle_context_pt __unused context, dm_dependency_manager_pt *manager) {
	celix_status_t status = CELIX_ENOMEM;

	(*manager) = calloc(1, sizeof(**manager));

	if (*manager) {
		arrayList_create(&(*manager)->components);

		status = CELIX_SUCCESS;
	}

	return status;

}

celix_status_t dependencyManager_destroy(dm_dependency_manager_pt *manager) {
	celix_status_t status = CELIX_SUCCESS;

	arrayList_destroy((*manager)->components);

	free(*manager);
	(*manager) = NULL;

	return status;
}

celix_status_t dependencyManager_add(dm_dependency_manager_pt manager, dm_component_pt component) {
	celix_status_t status;

	arrayList_add(manager->components, component);
	status = component_start(component);

	return status;
}

celix_status_t dependencyManager_remove(dm_dependency_manager_pt manager, dm_component_pt component) {
	celix_status_t status;

	arrayList_removeElement(manager->components, component);
	status = component_stop(component);

	return status;
}

celix_status_t dependencyManager_getComponents(dm_dependency_manager_pt manager, array_list_pt* components) {
	celix_status_t status = CELIX_SUCCESS;

	(*components) = manager->components;

	return status;
}
