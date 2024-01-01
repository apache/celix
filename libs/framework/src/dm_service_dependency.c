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

#include <stdlib.h>
#include <string.h>

#include "celix_bundle.h"
#include "celix_compiler.h"
#include "celix_constants.h"
#include "celix_utils.h"
#include "dm_service_dependency_impl.h"
#include "dm_component_impl.h"

#define DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND

static void serviceDependency_addServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props);
static void serviceDependency_removeServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props);
static void serviceDependency_setServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props);
static void* serviceDependency_getCallbackHandle(celix_dm_service_dependency_t *dep);

celix_status_t serviceDependency_create(celix_dm_service_dependency_t **dependency_ptr) {
    celix_dm_service_dependency_t *dep = celix_dmServiceDependency_create();
    if (dep != NULL) {
        *dependency_ptr = dep;
        return CELIX_SUCCESS;
    } else {
        return CELIX_BUNDLE_EXCEPTION;
    }
}

celix_dm_service_dependency_t* celix_dmServiceDependency_create() {
	celix_dm_service_dependency_t *dep = calloc(1, sizeof(*dep));
	dep->strategy = DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY;
	dep->svcTrackerId = -1;
	celixThreadMutex_create(&dep->mutex, NULL);
    return dep;
}

celix_status_t serviceDependency_destroy(celix_dm_service_dependency_t **dependency_ptr) {
	if (dependency_ptr != NULL) {
		celix_dmServiceDependency_destroy(*dependency_ptr);
	}
	return CELIX_SUCCESS;
}

void celix_dmServiceDependency_destroy(celix_dm_service_dependency_t *dep) {
    if (dep == NULL) {
        return;
    } else {
        if (!celix_dmServiceDependency_isDisabled(dep)) {
            celix_bundle_context_t* ctx = celix_dmComponent_getBundleContext(dep->component);
            celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR,
                       "Cannot destroy a service dependency this is still enabled.");
        } else {
            celixThreadMutex_destroy(&dep->mutex);
            free(dep->serviceName);
            free(dep->versionRange);
            free(dep->filter);
            free(dep);
        }
    }
}

celix_status_t serviceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required) {
	return celix_dmServiceDependency_setRequired(dependency, required);
}

celix_status_t celix_dmServiceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->required = required;
	}

	return status;
}

celix_status_t serviceDependency_setStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t strategy) {
	return celix_dmServiceDependency_setStrategy(dependency, strategy);
}

celix_status_t celix_dmServiceDependency_setStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t strategy) {
    dependency->strategy = strategy;
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_getStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t* strategy) {
	celix_dm_service_dependency_strategy_t str = celix_dmServiceDependency_getStrategy(dependency);
	if (strategy != NULL) {
		*strategy = str;
	}
	return CELIX_SUCCESS;
}

celix_dm_service_dependency_strategy_t celix_dmServiceDependency_getStrategy(celix_dm_service_dependency_t *dependency) {
	return dependency->strategy;
}

celix_status_t serviceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter) {
	return celix_dmServiceDependency_setService(dependency, serviceName, serviceVersionRange, filter);
}

celix_status_t celix_dmServiceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter) {
    if (dependency->serviceName != NULL) {
        free(dependency->serviceName);
    }
    if (dependency->versionRange != NULL) {
        free(dependency->versionRange);
    }
    if (dependency->filter != NULL) {
        free(dependency->filter);
    }
    dependency->serviceName = celix_utils_strdup(serviceName);
    dependency->versionRange = celix_utils_strdup(serviceVersionRange);
    dependency->filter = celix_utils_strdup(filter);
    return CELIX_SUCCESS;
}

celix_status_t serviceDependency_getFilter(celix_dm_service_dependency_t *dependency, const char** filter) {
	const char *f =  celix_dmServiceDependency_getFilter(dependency);
	if (filter != NULL) {
		*filter = f;
	}
	return CELIX_SUCCESS;
}

const char* celix_dmServiceDependency_getFilter(celix_dm_service_dependency_t *dependency) {
	return (const char*)dependency->filter;
}

celix_status_t serviceDependency_setCallbacks(celix_dm_service_dependency_t *dependency, service_set_fpt set, service_add_fpt add, service_change_fpt change CELIX_UNUSED, service_remove_fpt remove, service_swap_fpt swap  CELIX_UNUSED) {
    dependency->set = (celix_dm_service_update_fp)set;
    dependency->add = (celix_dm_service_update_fp)add;
    dependency->remove = (celix_dm_service_update_fp)remove;
    return CELIX_SUCCESS;
}

celix_status_t celix_dmServiceDependency_setCallback(celix_dm_service_dependency_t *dependency, celix_dm_service_update_fp set) {
	dependency->set = set;
	return CELIX_SUCCESS;
}


celix_status_t celix_dmServiceDependency_setCallbackWithProperties(celix_dm_service_dependency_t *dependency, celix_dm_service_update_with_props_fp set) {
	dependency->setWithProperties = set;
	return CELIX_SUCCESS;
}


celix_status_t celix_dmServiceDependency_setCallbacksWithOptions(celix_dm_service_dependency_t *dependency, const celix_dm_service_dependency_callback_options_t *opts) {
	dependency->set = opts->set;
	dependency->add = opts->add;
	dependency->remove = opts->remove;

	dependency->setWithProperties = opts->setWithProps;
	dependency->addWithProperties = opts->addWithProps;
	dependency->remWithProperties = opts->removeWithProps;
	return CELIX_SUCCESS;
}

celix_status_t celix_dmServiceDependency_setComponent(celix_dm_service_dependency_t *dependency, celix_dm_component_t *component) {
    dependency->component = component;
	return CELIX_SUCCESS;
}

celix_status_t celix_dmServiceDependency_enable(celix_dm_service_dependency_t* dependency) {
        celix_bundle_context_t* ctx = celix_dmComponent_getBundleContext(dependency->component);

        if (dependency->serviceName == NULL && dependency->filter == NULL) {
                celix_bundleContext_log(
                    ctx, CELIX_LOG_LEVEL_ERROR, "Cannot start a service dependency without a service name and filter");
                return CELIX_ILLEGAL_ARGUMENT;
        }

        celixThreadMutex_lock(&dependency->mutex);
        if (dependency->svcTrackerId == -1L) {
                celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
                opts.filter.filter = dependency->filter;
                opts.filter.serviceName = dependency->serviceName;
                opts.filter.versionRange = dependency->versionRange;
                opts.callbackHandle = dependency;
                opts.addWithProperties = serviceDependency_addServiceTrackerCallback;
                opts.removeWithProperties = serviceDependency_removeServiceTrackerCallback;
                opts.setWithProperties = serviceDependency_setServiceTrackerCallback;
                dependency->svcTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
        }
        celixThreadMutex_unlock(&dependency->mutex);

        return CELIX_SUCCESS;
}

static void celix_serviceDependency_stopCallback(void *data) {
    celix_dm_service_dependency_t* dependency = data;
    celixThreadMutex_lock(&dependency->mutex);
    dependency->nrOfActiveStoppingTrackers -= 1;
    celixThreadMutex_unlock(&dependency->mutex);
}

celix_status_t celix_dmServiceDependency_disable(celix_dm_service_dependency_t *dependency) {
    celixThreadMutex_lock(&dependency->mutex);
    if (dependency->svcTrackerId >= 0) {
        celix_bundle_context_t* ctx = celix_dmComponent_getBundleContext(dependency->component);
        celix_bundleContext_stopTrackerAsync(ctx, dependency->svcTrackerId, dependency, celix_serviceDependency_stopCallback);
        dependency->svcTrackerId = -1;
        dependency->nrOfActiveStoppingTrackers += 1;
    }
    celixThreadMutex_unlock(&dependency->mutex);

	return CELIX_SUCCESS;
}

bool celix_dmServiceDependency_isDisabled(celix_dm_service_dependency_t *dependency) {
    celixThreadMutex_lock(&dependency->mutex);
    bool isStopped = dependency->svcTrackerId == -1L && dependency->nrOfActiveStoppingTrackers == 0;
    celixThreadMutex_unlock(&dependency->mutex);
    return isStopped;
}

static void serviceDependency_setServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props) {
    celix_dm_service_dependency_t* dependency = handle;

    celix_dm_event_t event;
    event.dep = dependency;
    event.eventType = CELIX_DM_EVENT_SVC_SET;
    event.svc = svc;
    event.props = props;
    celix_private_dmComponent_handleEvent(dependency->component, &event);
}

celix_status_t celix_dmServiceDependency_invokeSet(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props) {
    if (dependency->set) {
        dependency->set(serviceDependency_getCallbackHandle(dependency), svc);
    }
    if (dependency->setWithProperties) {
        dependency->setWithProperties(serviceDependency_getCallbackHandle(dependency), svc, props);
    }
    return CELIX_SUCCESS;
}

static void serviceDependency_addServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props) {
    celix_dm_service_dependency_t* dependency = handle;

    celixThreadMutex_lock(&dependency->mutex);
    dependency->trackedSvcCount += 1;
    celixThreadMutex_unlock(&dependency->mutex);

    celix_dm_event_t event;
    event.dep = dependency;
    event.eventType = CELIX_DM_EVENT_SVC_ADD;
    event.svc = svc;
    event.props = props;
    celix_private_dmComponent_handleEvent(dependency->component, &event);
}

celix_status_t celix_dmServiceDependency_invokeAdd(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props) {
    void *handle = serviceDependency_getCallbackHandle(dependency);
    if (dependency->add) {
        dependency->add(handle, svc);
    }
    if (dependency->addWithProperties) {
        dependency->addWithProperties(handle, svc, props);
    }
    return CELIX_SUCCESS;
}

static void serviceDependency_removeServiceTrackerCallback(void *handle, void *svc, const celix_properties_t *props) {
    celix_dm_service_dependency_t* dependency = handle;

    celixThreadMutex_lock(&dependency->mutex);
    dependency->trackedSvcCount -= 1;
    celixThreadMutex_unlock(&dependency->mutex);

    celix_dm_event_t event;
    event.dep = dependency;
    event.eventType = CELIX_DM_EVENT_SVC_REM;
    event.svc = svc;
    event.props = props;
    celix_private_dmComponent_handleEvent(dependency->component, &event);
}

celix_status_t celix_dmServiceDependency_invokeRemove(celix_dm_service_dependency_t *dependency, void* svc, const celix_properties_t* props) {
    if (dependency->remove) {
        dependency->remove(serviceDependency_getCallbackHandle(dependency), svc);
    }
    if (dependency->remWithProperties) {
        dependency->remWithProperties(serviceDependency_getCallbackHandle(dependency), svc, props);
    }
    return CELIX_SUCCESS;
}

bool celix_dmServiceDependency_isAvailable(celix_dm_service_dependency_t *dependency) {
    celixThreadMutex_lock(&dependency->mutex);
    bool avail = dependency->trackedSvcCount > 0;
    celixThreadMutex_unlock(&dependency->mutex);
    return avail;
}

bool celix_dmServiceDependency_isRequired(const celix_dm_service_dependency_t* dep) {
    return dep->required;
}

bool celix_dmServiceDependency_isTrackerOpen(celix_dm_service_dependency_t* dependency) {
    celixThreadMutex_lock(&dependency->mutex);
    bool isOpen = dependency->svcTrackerId >= 0;
    celixThreadMutex_unlock(&dependency->mutex);
    return isOpen;
}

bool celix_dmServiceDependency_isSetCallbackConfigured(celix_dm_service_dependency_t* dependency) {
    bool isUsed = dependency->set != NULL || dependency->setWithProperties != NULL;
    return isUsed;
}
bool celix_dmServiceDependency_isAddRemCallbacksConfigured(celix_dm_service_dependency_t* dependency) {
    bool isUsed = dependency->add != NULL || dependency->addWithProperties != NULL ||
            dependency->remove != NULL || dependency->remWithProperties != NULL;
    return isUsed;
}

celix_status_t serviceDependency_getServiceDependencyInfo(celix_dm_service_dependency_t *dep, dm_service_dependency_info_t **out) {
	if (out != NULL) {
		*out = celix_dmServiceDependency_createInfo(dep);
	}
	return CELIX_SUCCESS;
}

dm_service_dependency_info_t* celix_dmServiceDependency_createInfo(celix_dm_service_dependency_t* dep) {
	celix_dm_service_dependency_info_t *info = calloc(1, sizeof(*info));
	if (info != NULL) {
		celixThreadMutex_lock(&dep->mutex);
		info->available = dep->trackedSvcCount > 0;
		info->serviceName = celix_utils_strdup(dep->serviceName);
		info->filter = celix_utils_strdup(dep->filter);
		info->versionRange = celix_utils_strdup(dep->versionRange);
		info->required = dep->required;
		info->count = dep->trackedSvcCount;
		celixThreadMutex_unlock(&dep->mutex);
	}
	return info;
}


void dependency_destroyDependencyInfo(dm_service_dependency_info_pt info) {
	celix_dmServiceDependency_destroyInfo(NULL, info);
}

void celix_dmServiceDependency_destroyInfo(celix_dm_service_dependency_t *dep CELIX_UNUSED, dm_service_dependency_info_t *info) {
	if (info != NULL) {
	    free(info->serviceName);
		free(info->filter);
		free(info->versionRange);
	}
	free(info);
}

celix_status_t serviceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle) {
	return celix_dmServiceDependency_setCallbackHandle(dependency, handle);
}

celix_status_t celix_dmServiceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle) {
	dependency->callbackHandle = handle;
    return CELIX_SUCCESS;
}

static void* serviceDependency_getCallbackHandle(celix_dm_service_dependency_t *dependency) {
    return dependency->callbackHandle == NULL ? component_getImplementation(dependency->component) : dependency->callbackHandle;
}
