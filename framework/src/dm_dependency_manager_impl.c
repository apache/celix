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
#include <dm_dependency_manager.h>

#include "bundle_context.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"


celix_status_t dependencyManager_create(bundle_context_pt context __attribute__((unused)), dm_dependency_manager_pt *manager) {
	celix_status_t status = CELIX_ENOMEM;

	(*manager) = calloc(1, sizeof(**manager));

	if (*manager) {
		arrayList_create(&(*manager)->components);

		status = CELIX_SUCCESS;
	}

	return status;

}

void dependencyManager_destroy(dm_dependency_manager_pt manager) {
	if (manager != NULL) {
		arrayList_destroy(manager->components);
		free(manager);
	}
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
    component_destroy(component);

    return status;
}

celix_status_t dependencyManager_removeAllComponents(dm_dependency_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	unsigned int i=0;
	unsigned int size = arrayList_size(manager->components);

	for(;i<size;i++){
		dm_component_pt cmp = arrayList_get(manager->components, i);
//		printf("Stopping comp %s\n", component_getName(cmp));
		component_stop(cmp);
	}

	while (!arrayList_isEmpty(manager->components)) {
		dm_component_pt cmp = arrayList_remove(manager->components, 0);
//        printf("Removing comp %s\n", component_getName(cmp));
        component_destroy(cmp);
	}

	return status;
}

celix_status_t dependencyManager_getInfo(dm_dependency_manager_pt manager, dm_dependency_manager_info_pt *out) {
	celix_status_t status = CELIX_SUCCESS;
	unsigned int i;
	int size;
	dm_component_info_pt cmpInfo = NULL;
	dm_dependency_manager_info_pt info = calloc(1, sizeof(*info));

	celixThreadMutex_lock(&manager->mutex);

	if (info != NULL) {
		arrayList_create(&info->components);
		size = arrayList_size(manager->components);
		for (i = 0; i < size; i += 1) {
			dm_component_pt cmp = arrayList_get(manager->components, i);
			cmpInfo = NULL;
			component_getComponentInfo(cmp, &cmpInfo);
			arrayList_add(info->components, cmpInfo);
		}
	} else {
		status = CELIX_ENOMEM;
	}

	celixThreadMutex_unlock(&manager->mutex);

	if (status == CELIX_SUCCESS) {
		*out = info;
	}

	return status;
}

void dependencyManager_destroyInfo(dm_dependency_manager_pt __attribute__((__unused__)) manager, dm_dependency_manager_info_pt info) {
    unsigned int i = 0;
    for (; i < arrayList_size(info->components); i += 1) {
        dm_component_info_pt cmpinfo = (dm_component_info_pt)arrayList_get(info->components, i);
        component_destroyComponentInfo(cmpinfo);
    }
    arrayList_destroy(info->components);
    free(info);
}
