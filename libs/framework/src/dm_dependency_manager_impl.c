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


#include <pthread.h>
#include <stdlib.h>
#include <dm_dependency_manager.h>
#include <memory.h>

#include "bundle_context.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"
#include "celix_dependency_manager.h"
#include "celix_bundle.h"


celix_dependency_manager_t* celix_private_dependencyManager_create(celix_bundle_context_t *context) {
	celix_dependency_manager_t *manager = calloc(1, sizeof(*manager));
	if (manager != NULL) {
		manager->ctx = context;
		manager->components = celix_arrayList_create();
		pthread_mutex_init(&manager->mutex, NULL);
	}
	return manager;
}

void celix_private_dependencyManager_destroy(celix_dependency_manager_t *manager) {
	if (manager != NULL) {
		celix_arrayList_destroy(manager->components);
		pthread_mutex_destroy(&manager->mutex);
		free(manager);
	}
}


celix_status_t celix_dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
	celix_status_t status;
	celix_arrayList_add(manager->components, component);
	status = celix_private_dmComponent_start(component);
	return status;
}


celix_status_t celix_dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
	celix_status_t status;

	celix_array_list_entry_t entry;
	memset(&entry, 0, sizeof(entry));
	entry.voidPtrVal = component;
	int index = celix_arrayList_indexOf(manager->components, entry);

	if (index >= 0) {
        celix_arrayList_removeAt(manager->components, index);
        status = celix_private_dmComponent_stop(component);
        component_destroy(component);
	} else {
	    fprintf(stderr, "Cannot find component with pointer %p\n", component);
	    status = CELIX_BUNDLE_EXCEPTION;
	}


	return status;
}


celix_status_t celix_dependencyManager_removeAllComponents(celix_dependency_manager_t *manager) {
	celix_status_t status = CELIX_SUCCESS;

	int i=0;
	int size = celix_arrayList_size(manager->components);

	for(;i<size;i++){
		celix_dm_component_t *cmp = celix_arrayList_get(manager->components, i);
//		printf("Stopping comp %s\n", component_getName(cmp));
		celix_private_dmComponent_stop(cmp);
	}

	while (!arrayList_isEmpty(manager->components)) {
		celix_dm_component_t *cmp = celix_arrayList_get(manager->components, 0);
		celix_arrayList_removeAt(manager->components, 0);
//        printf("Removing comp %s\n", component_getName(cmp));
		component_destroy(cmp);
	}

	return status;
}


static void celix_dm_getInfoCallback(void *handle, const celix_bundle_t *bnd) {
	celix_dependency_manager_info_t **out = handle;

	celix_bundle_context_t *context = NULL;
	bundle_getContext((celix_bundle_t*)bnd, &context);
	celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(context);

	celix_dependency_manager_info_t *info = calloc(1, sizeof(*info));
	celixThreadMutex_lock(&mng->mutex);
	if (info != NULL) {
		info->components = celix_arrayList_create();
		int size = celix_arrayList_size(mng->components);
		for (int i = 0; i < size; i += 1) {
			celix_dm_component_t *cmp = celix_arrayList_get(mng->components, i);
			celix_dm_component_info_t *cmpInfo = NULL;
			component_getComponentInfo(cmp, &cmpInfo);
			celix_arrayList_add(info->components, cmpInfo);
		}
	}
	celixThreadMutex_unlock(&mng->mutex);

	*out = info;
}

celix_dependency_manager_info_t* celix_dependencyManager_createInfo(celix_dependency_manager_t *manager, long bndId) {
	celix_dependency_manager_info_t *info = NULL;
	celix_bundleContext_useBundle(manager->ctx, bndId, &info, celix_dm_getInfoCallback);
	return info;
}


static void celix_dm_getInfosCallback(void *handle, const celix_bundle_t *bnd) {
	celix_array_list_t *infos = handle;

	celix_bundle_context_t *context = NULL;
	bundle_getContext((celix_bundle_t*)bnd, &context);
	celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(context);

	celix_dependency_manager_info_t *info = calloc(1, sizeof(*info));
	celixThreadMutex_lock(&mng->mutex);
	if (info != NULL) {
		info->bndId = celix_bundle_getId(bnd);
		info->components = celix_arrayList_create();
		int size = celix_arrayList_size(mng->components);
		for (int i = 0; i < size; i += 1) {
			celix_dm_component_t *cmp = celix_arrayList_get(mng->components, i);
			celix_dm_component_info_t *cmpInfo = NULL;
			component_getComponentInfo(cmp, &cmpInfo);
			celix_arrayList_add(info->components, cmpInfo);
		}

		celix_arrayList_add(infos, info);
	}
	celixThreadMutex_unlock(&mng->mutex);
}

celix_array_list_t * celix_dependencyManager_createInfos(celix_dependency_manager_t *manager) {
	celix_array_list_t *infos = celix_arrayList_create();
	celix_bundleContext_useBundles(manager->ctx, infos, celix_dm_getInfosCallback);
	return infos;
}


void celix_dependencyManager_destroyInfo(celix_dependency_manager_t *manager __attribute__((unused)), celix_dependency_manager_info_t *info) {
	for (int i = 0; i < celix_arrayList_size(info->components); i += 1) {
		celix_dm_component_info_t *cmpinfo = (dm_component_info_pt)arrayList_get(info->components, i);
		component_destroyComponentInfo(cmpinfo);
	}
	arrayList_destroy(info->components);
	free(info);
}


void celix_dependencyManager_destroyInfos(celix_dependency_manager_t *manager, celix_array_list_t * infos /*entries celix_dm_dependency_manager_info_t*/) {
	for (int i = 0; i < celix_arrayList_size(infos); ++i) {
		celix_dependencyManager_destroyInfo(manager, celix_arrayList_get(infos, i));
	}
	celix_arrayList_destroy(infos);
}




/************************ DEPRECATED API ************************************************/


celix_status_t dependencyManager_create(bundle_context_pt context, celix_dependency_manager_t **out) {
	celix_status_t status = CELIX_SUCCESS;
	celix_dependency_manager_t *manager = celix_private_dependencyManager_create(context);
	if (manager != NULL) {
        *out = manager;
	} else {
		status = CELIX_BUNDLE_EXCEPTION;
	}
	return status;
}

void dependencyManager_destroy(celix_dependency_manager_t *manager) {
	celix_private_dependencyManager_destroy(manager);
}

celix_status_t dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
	return celix_dependencyManager_add(manager, component);
}

celix_status_t dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
    return celix_dependencyManager_remove(manager, component);
}

celix_status_t dependencyManager_removeAllComponents(celix_dependency_manager_t *manager) {
	return celix_dependencyManager_removeAllComponents(manager);
}

celix_status_t dependencyManager_getInfo(celix_dependency_manager_t *manager, dm_dependency_manager_info_pt *out) {
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
			celix_dm_component_t *cmp = arrayList_get(manager->components, i);
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


void dependencyManager_destroyInfo(celix_dependency_manager_t *manager, celix_dependency_manager_info_t *info) {
    celix_dependencyManager_destroyInfo(manager, info);
}