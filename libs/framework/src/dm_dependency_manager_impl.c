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

#include <pthread.h>
#include <stdlib.h>
#include <dm_dependency_manager.h>
#include <memory.h>
#include <celix_utils.h>

#include "celix_bundle_context.h"
#include "celix_array_list.h"
#include "celix_bundle.h"
#include "celix_bundle_private.h"
#include "celix_compiler.h"
#include "celix_dependency_manager.h"
#include "celix_framework.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"

celix_dependency_manager_t* celix_private_dependencyManager_create(celix_bundle_context_t *context) {
	celix_dependency_manager_t *manager = calloc(1, sizeof(*manager));
	if (manager != NULL) {
		manager->ctx = context;
		manager->components = celix_arrayList_createPointerArray();
		pthread_mutex_init(&manager->mutex, NULL);
	}
	return manager;
}

void celix_private_dependencyManager_destroy(celix_dependency_manager_t *manager) {
	if (manager != NULL) {
        celixThreadMutex_lock(&manager->mutex);
		celix_arrayList_destroy(manager->components);
        celixThreadMutex_unlock(&manager->mutex);

		pthread_mutex_destroy(&manager->mutex);
		free(manager);
	}
}

celix_status_t celix_dependencyManager_add(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
	celix_status_t status = celix_dependencyManager_addAsync(manager, component);
	celix_dependencyManager_wait(manager);
	return status;
}

celix_status_t celix_dependencyManager_addAsync(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
    celixThreadMutex_lock(&manager->mutex);
	celix_arrayList_add(manager->components, component);
    celixThreadMutex_unlock(&manager->mutex);

	return celix_private_dmComponent_enable(component);
}

static celix_status_t celix_dependencyManager_removeWithoutDestroy(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&manager->mutex);
    bool found = false;
    for (int i = 0; i < celix_arrayList_size(manager->components); ++i) {
        celix_dm_component_t* visit = celix_arrayList_get(manager->components, i);
        if (visit == component) {
            celix_arrayList_removeAt(manager->components, i);
            found = true;
            break;
        }
    }
    celixThreadMutex_unlock(&manager->mutex);

    if (!found) {
        celix_bundleContext_log(
                manager->ctx,
                CELIX_LOG_LEVEL_ERROR,
                "Cannot find component with ptr %p",
                component);
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t celix_dependencyManager_remove(celix_dependency_manager_t *manager, celix_dm_component_t *component) {
	celix_status_t  status = celix_dependencyManager_removeAsync(manager, component, NULL, NULL);
	celix_dependencyManager_wait(manager);
	return status;
}

celix_status_t celix_dependencyManager_removeAsync(
        celix_dependency_manager_t *manager,
        celix_dm_component_t *component,
        void* doneData,
        void (*doneCallback)(void* data)) {
    celix_status_t  status = celix_dependencyManager_removeWithoutDestroy(manager, component);
    if (status == CELIX_SUCCESS) {
        celix_dmComponent_destroyAsync(component, doneData, doneCallback);
    }
	return status;
}

celix_status_t celix_dependencyManager_removeAllComponents(celix_dependency_manager_t *manager) {
    celix_status_t status = celix_dependencyManager_removeAllComponentsAsync(manager, NULL, NULL);
    celix_dependencyManager_wait(manager);
	return status;
}

struct celix_dependency_manager_removeall_data {
    size_t destroysInProgress;
    void *doneData;
    void (*doneCallback)(void* data);
};

static void celix_dependencyManager_removeAllComponentsAsyncCallback(void *data) {
    struct celix_dependency_manager_removeall_data* callbackData = data;
    callbackData->destroysInProgress -= 1;
    if (callbackData->destroysInProgress == 0) {
        callbackData->doneCallback(callbackData->doneData);
        free(callbackData);
    }
}

celix_status_t celix_dependencyManager_removeAllComponentsAsync(celix_dependency_manager_t *manager, void *doneData, void (*doneCallback)(void *data)) {
	celix_status_t status = CELIX_SUCCESS;

	//setup callback data struct to synchronize when all components are destroyed (if doneCallback is not NULL)
    struct celix_dependency_manager_removeall_data* callbackData = NULL;
    if (doneCallback != NULL) {
        callbackData = malloc(sizeof(*callbackData));
        callbackData->destroysInProgress = 0;
        callbackData->doneData = doneData;
        callbackData->doneCallback = doneCallback;
    }

    //remove components and queue a async component destroy
    celixThreadMutex_lock(&manager->mutex);
    if (doneCallback != NULL) {
        callbackData->destroysInProgress = celix_arrayList_size(manager->components);
    }
    if (celix_arrayList_size(manager->components) == 0 && doneCallback != NULL) {
        //corner case: if no components are available, call done callback on event thread.
        celix_framework_t* fw = celix_bundleContext_getFramework(manager->ctx);
        long bndId = celix_bundleContext_getBundleId(manager->ctx);
        celix_framework_fireGenericEvent(
                fw,
                -1,
                bndId,
                "celix_dependencyManager_removeAllComponentsAsync callback",
                doneData,
                doneCallback,
                NULL,
                NULL
        );
        free(callbackData);
    }
    for (int i = 0; i < celix_arrayList_size(manager->components); ++i) {
        celix_dm_component_t *cmp = celix_arrayList_get(manager->components, i);
        if (doneCallback != NULL) {
            celix_dmComponent_destroyAsync(cmp, callbackData, celix_dependencyManager_removeAllComponentsAsyncCallback);
        } else {
            celix_dmComponent_destroyAsync(cmp, NULL, NULL);
        }
    }
    celix_arrayList_clear(manager->components);
    celixThreadMutex_unlock(&manager->mutex);

	return status;
}


static void celix_dm_getInfoCallback(void *handle, const celix_bundle_t *bnd) {
    celix_dependency_manager_info_t **out = handle;

    celix_bundle_context_t *context = celix_bundle_getContext(bnd);
    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(context);

    celix_dependency_manager_info_t *info = calloc(1, sizeof(*info));
    if (info != NULL) {
        info->bndId = celix_bundle_getId(bnd);
        info->bndSymbolicName = celix_utils_strdup(celix_bundle_getSymbolicName(bnd));
        celixThreadMutex_lock(&mng->mutex);
        celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void*)component_destroyComponentInfo;
        info->components = celix_arrayList_createWithOptions(&opts);
        int size = celix_arrayList_size(mng->components);
        for (int i = 0; i < size; i += 1) {
            celix_dm_component_t *cmp = celix_arrayList_get(mng->components, i);
            celix_dm_component_info_t *cmpInfo = NULL;
            component_getComponentInfo(cmp, &cmpInfo);
            celix_arrayList_add(info->components, cmpInfo);
        }
        celixThreadMutex_unlock(&mng->mutex);
    }
    *out = info;
}

celix_dependency_manager_info_t* celix_dependencyManager_createInfo(celix_dependency_manager_t *manager, long bndId) {
    celix_dependency_manager_info_t* info = NULL;
    celix_framework_useBundle(
        celix_bundleContext_getFramework(manager->ctx), true /*onlyActive*/, bndId, &info, celix_dm_getInfoCallback);
    return info;
}

static void celix_dm_getInfosCallback(void* handle, const celix_bundle_t* bnd) {
    celix_array_list_t* infos = handle;

    celix_bundle_context_t* context = celix_bundle_getContext(bnd);
    celix_dependency_manager_t* mng = celix_bundleContext_getDependencyManager(context);

    celix_dependency_manager_info_t* info = calloc(1, sizeof(*info));
    info->bndId = celix_bundle_getId(bnd);
    info->bndSymbolicName = celix_utils_strdup(celix_bundle_getSymbolicName(bnd));
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.simpleRemovedCallback = (void*)component_destroyComponentInfo;
    info->components = celix_arrayList_createWithOptions(&opts);

    celixThreadMutex_lock(&mng->mutex);
    int size = celix_arrayList_size(mng->components);
    for (int i = 0; i < size; i += 1) {
        celix_dm_component_t* cmp = celix_arrayList_get(mng->components, i);
        celix_dm_component_info_t* cmpInfo = NULL;
        celix_dmComponent_getComponentInfo(cmp, &cmpInfo);
        celix_arrayList_add(info->components, cmpInfo);
    }
    celix_arrayList_add(infos, info);
    celixThreadMutex_unlock(&mng->mutex);
}

static void celix_dependencyManager_destroyInfoCallback(void *data, celix_array_list_entry_t entry) {
    celix_dependency_manager_t* manager = data;
    celix_dependency_manager_info_t* info = entry.voidPtrVal;
    celix_dependencyManager_destroyInfo(manager, info);
}

celix_array_list_t * celix_dependencyManager_createInfos(celix_dependency_manager_t* manager) {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.removedCallbackData = manager;
    opts.removedCallback = celix_dependencyManager_destroyInfoCallback;
    celix_array_list_t* infos = celix_arrayList_createWithOptions(&opts);

    celix_framework_t* fw = celix_bundleContext_getFramework(manager->ctx);
    celix_framework_useActiveBundles(fw, true, infos, celix_dm_getInfosCallback);
    return infos;
}

static void celix_dm_allComponentsActiveCallback(void *handle, const celix_bundle_t *bnd) {
	bool *allActivePtr = handle;

    if (celix_bundle_getState(bnd) != CELIX_BUNDLE_STATE_ACTIVE) {
        return;
    }

	celix_bundle_context_t *context = celix_bundle_getContext(bnd);
	celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(context);
    bool allActive = celix_dependencyManager_areComponentsActive(mng);
    if (!allActive) {
        *allActivePtr = false;
    }
}

bool celix_dependencyManager_allComponentsActive(celix_dependency_manager_t *manager) {
	bool allActive = true;
	celix_bundleContext_useBundles(manager->ctx, &allActive, celix_dm_allComponentsActiveCallback);
	return allActive;
}

size_t celix_dependencyManager_nrOfComponents(celix_dependency_manager_t *mng) {
    celixThreadMutex_lock(&mng->mutex);
    size_t nr = (size_t)celix_arrayList_size(mng->components);
    celixThreadMutex_unlock(&mng->mutex);
    return nr;
}

void celix_dependencyManager_wait(celix_dependency_manager_t *mng) {
    celix_framework_t *fw = celix_bundleContext_getFramework(mng->ctx);
    if (!celix_framework_isCurrentThreadTheEventLoop(fw)) {
        celix_bundleContext_waitForEvents(mng->ctx);
    } else {
        celix_bundleContext_log(mng->ctx, CELIX_LOG_LEVEL_WARNING,
        "celix_dependencyManager_wait: Cannot wait for Celix event queue on the Celix event queue thread! "
        "Use async dep man API instead.");
    }
}

bool celix_dependencyManager_areComponentsActive(celix_dependency_manager_t *mng) {
    bool allActive = true;
    celixThreadMutex_lock(&mng->mutex);
    int size = celix_arrayList_size(mng->components);
    for (int i = 0; i < size; i += 1) {
        celix_dm_component_t *cmp = celix_arrayList_get(mng->components, i);
        if (!celix_dmComponent_isActive(cmp)) {
            allActive = false;
            break;
        }
    }
    celixThreadMutex_unlock(&mng->mutex);
    return allActive;
}

void celix_dependencyManager_destroyInfo(celix_dependency_manager_t *manager CELIX_UNUSED, celix_dependency_manager_info_t *info) {
    if (info != NULL) {
        celix_arrayList_destroy(info->components);
        free(info->bndSymbolicName);
        free(info);
    }
}


void celix_dependencyManager_destroyInfos(celix_dependency_manager_t *manager CELIX_UNUSED, celix_array_list_t * infos /*entries celix_dm_dependency_manager_info_t*/) {
	celix_arrayList_destroy(infos);
}




/************************ DEPRECATED API ************************************************/


celix_status_t dependencyManager_create(bundle_context_pt context, celix_dependency_manager_t **out) {
	celix_status_t status = CELIX_SUCCESS;
	celix_dependency_manager_t *manager = celix_bundleContext_getDependencyManager(context);
	if (manager != NULL) {
        *out = manager;
	} else {
		status = CELIX_BUNDLE_EXCEPTION;
	}
	return status;
}

void dependencyManager_destroy(celix_dependency_manager_t *manager) {
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
		info->components = celix_arrayList_createPointerArray();
		size = celix_arrayList_size(manager->components);
		for (i = 0; i < size; i += 1) {
			celix_dm_component_t *cmp = celix_arrayList_get(manager->components, i);
			cmpInfo = NULL;
			component_getComponentInfo(cmp, &cmpInfo);
                        celix_arrayList_add(info->components, cmpInfo);
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

static void celix_dependencyManager_printInfoForEntry(bool fullInfo, bool useAnsiColors, FILE* stream, celix_dependency_manager_info_t* mngInfo) {
	for (int i = 0; i < celix_arrayList_size(mngInfo->components); ++i) {
		celix_dm_component_info_t *cmpInfo = celix_arrayList_get(mngInfo->components, i);
        celix_dmComponent_printComponentInfo(cmpInfo, fullInfo, useAnsiColors, stream);
	}
}

void celix_dependencyManager_printInfo(celix_dependency_manager_t* manager, bool fullInfo, bool useAnsiColors, FILE* stream) {
	celix_array_list_t *infos = celix_dependencyManager_createInfos(manager);
	for (int i = 0; i< celix_arrayList_size(infos); ++i) {
		celix_dependency_manager_info_t* mngInfo = celix_arrayList_get(infos, i);
		celix_dependencyManager_printInfoForEntry(fullInfo, useAnsiColors, stream, mngInfo);
	}
	celix_dependencyManager_destroyInfos(manager, infos);
}

void celix_dependencyManager_printInfoForBundle(celix_dependency_manager_t* manager, bool fullInfo, bool useAnsiColors, long bundleId, FILE* stream) {
	celix_dependency_manager_info_t* mngInfo = celix_dependencyManager_createInfo(manager, bundleId);
    if (mngInfo != NULL) {
        celix_dependencyManager_printInfoForEntry(fullInfo, useAnsiColors, stream, mngInfo);
        celix_dependencyManager_destroyInfo(manager, mngInfo);
    }
}
