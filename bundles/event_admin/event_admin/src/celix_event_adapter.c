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
#include "celix_event_adapter.h"

#include <assert.h>

#include "celix_log_helper.h"
#include "celix_condition.h"
#include "celix_constants.h"
#include "celix_threads.h"
#include "celix_utils.h"
#include "celix_stdlib_cleanup.h"
#include "celix_event_constants.h"

struct celix_event_adapter {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    celix_thread_rwlock_t lock;
    celix_event_admin_service_t* eventAdminService;
    long servicesTrackerId;
    long bundlesTrackerId;
    long fwCondTrackerId;
};

static void celix_eventAdapter_addServiceWithProperties(void* handle, void* svc, const celix_properties_t* props);

static void celix_eventAdapter_removeServiceWithProperties(void* handle, void* svc, const celix_properties_t* props);

static void celix_eventAdapter_onBundleEvent(void* handle, const celix_bundle_event_t* event);

static void celix_eventAdapter_addFrameworkConditionServiceWithProperties(void* handle, void* svc, const celix_properties_t* props);

celix_event_adapter_t* celix_eventAdapter_create(celix_bundle_context_t* ctx) {
    celix_autoptr(celix_log_helper_t) logHelper = celix_logHelper_create(ctx, "CelixEventAdapter");
    if (logHelper == NULL) {
        return NULL;
    }
    celix_autofree celix_event_adapter_t* adapter = calloc(1, sizeof(*adapter));
    if (adapter == NULL) {
        celix_logHelper_error(logHelper, "Error allocating memory for event adapter");
        return NULL;
    }
    adapter->logHelper = logHelper;
    adapter->ctx = ctx;
    adapter->eventAdminService = NULL;
    celix_status_t status = celixThreadRwlock_create(&adapter->lock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating rwlock for event adapter");
        return NULL;
    }

    celix_steal_ptr(logHelper);
    return celix_steal_ptr(adapter);
}

int celix_eventAdapter_start(celix_event_adapter_t* adapter) {

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = NULL;//track all services
    opts.callbackHandle = adapter;
    opts.addWithProperties = celix_eventAdapter_addServiceWithProperties;
    opts.removeWithProperties = celix_eventAdapter_removeServiceWithProperties;
    adapter->servicesTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(adapter->ctx, &opts);
    if (adapter->servicesTrackerId < 0) {
        celix_logHelper_error(adapter->logHelper, "Error creating tracker for services");
    }

    celix_bundle_tracking_options_t bndOpts = CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS;
    bndOpts.callbackHandle = adapter;
    bndOpts.onBundleEvent = celix_eventAdapter_onBundleEvent;
    adapter->bundlesTrackerId = celix_bundleContext_trackBundlesWithOptionsAsync(adapter->ctx, &bndOpts);
    if (adapter->bundlesTrackerId < 0) {
        celix_logHelper_error(adapter->logHelper, "Error creating tracker for bundles");
    }

    celix_service_tracking_options_t condOpts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    condOpts.filter.serviceName = CELIX_CONDITION_SERVICE_NAME;
    condOpts.filter.versionRange = "[1.0.0,2)";
    condOpts.filter.filter = "(|("CELIX_CONDITION_ID"="CELIX_CONDITION_ID_FRAMEWORK_READY")("CELIX_CONDITION_ID"="CELIX_CONDITION_ID_FRAMEWORK_ERROR"))";
    condOpts.callbackHandle = adapter;
    condOpts.addWithProperties = celix_eventAdapter_addFrameworkConditionServiceWithProperties;
    adapter->fwCondTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(adapter->ctx, &condOpts);
    if (adapter->fwCondTrackerId < 0) {
        celix_logHelper_error(adapter->logHelper, "Error creating tracker for framework condition");
    }

    return CELIX_SUCCESS;
}

int celix_eventAdapter_stop(celix_event_adapter_t* adapter) {
    celix_bundleContext_stopTracker(adapter->ctx, adapter->fwCondTrackerId);
    celix_bundleContext_stopTracker(adapter->ctx, adapter->bundlesTrackerId);
    celix_bundleContext_stopTracker(adapter->ctx, adapter->servicesTrackerId);
    return CELIX_SUCCESS;
}

void celix_eventAdapter_destroy(celix_event_adapter_t* adapter) {
    if (adapter != NULL) {
        celixThreadRwlock_destroy(&adapter->lock);
        celix_logHelper_destroy(adapter->logHelper);
        free(adapter);
    }
    return;
}

int celix_eventAdapter_setEventAdminService(void* handle, void* eventAdminService) {
    celix_event_adapter_t* adapter = (celix_event_adapter_t*) handle;
    celixThreadRwlock_writeLock(&adapter->lock);
    adapter->eventAdminService = (celix_event_admin_service_t*) eventAdminService;
    celixThreadRwlock_unlock(&adapter->lock);
    return CELIX_SUCCESS;
}

celix_properties_t* celix_eventAdapter_createServiceEventProperties(const celix_properties_t* serviceProps) {
    celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
    if (eventProps == NULL) {
        return NULL;
    }
    if (celix_properties_set(eventProps, CELIX_EVENT_SERVICE_ID,
                             celix_properties_get(serviceProps, CELIX_FRAMEWORK_SERVICE_ID, "-1")) != CELIX_SUCCESS) {
        return NULL;
    }
    if (celix_properties_set(eventProps, CELIX_EVENT_SERVICE_OBJECTCLASS,
                             celix_properties_get(serviceProps, CELIX_FRAMEWORK_SERVICE_NAME, "")) != CELIX_SUCCESS) {
        return NULL;
    }
    const char* pid = celix_properties_get(serviceProps, CELIX_EVENT_SERVICE_PID, NULL);
    if (pid != NULL && celix_properties_set(eventProps, CELIX_EVENT_SERVICE_PID, pid) != CELIX_SUCCESS) {
        return NULL;
    }
    return celix_steal_ptr(eventProps);
}

static void celix_eventAdapter_addServiceWithProperties(void* handle, void* svc, const celix_properties_t* props) {
    (void) svc;//unused
    celix_event_adapter_t* adapter = (celix_event_adapter_t*) handle;
    const char* topic = "celix/framework/ServiceEvent/REGISTERED";
    celix_autoptr(celix_properties_t) eventProps = celix_eventAdapter_createServiceEventProperties(props);
    if (eventProps == NULL) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to create properties for service register event.");
        return;
    }

    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&adapter->lock);
    celix_event_admin_service_t *eventAdminService = adapter->eventAdminService;
    if (eventAdminService == NULL) {
        return;
    }
    eventAdminService->postEvent(eventAdminService->handle, topic, eventProps);
    return;
}

static void celix_eventAdapter_removeServiceWithProperties(void* handle, void* svc, const celix_properties_t* props) {
    (void) svc;//unused
    celix_event_adapter_t* adapter = (celix_event_adapter_t*) handle;
    const char* topic = "celix/framework/ServiceEvent/UNREGISTERING";
    celix_autoptr(celix_properties_t) eventProps = celix_eventAdapter_createServiceEventProperties(props);
    if (eventProps == NULL) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to create properties for service unregister event.");
        return;
    }

    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&adapter->lock);
    celix_event_admin_service_t *eventAdminService = adapter->eventAdminService;
    if (eventAdminService == NULL) {
        return;
    }
    eventAdminService->postEvent(eventAdminService->handle, topic, eventProps);
    return;
}

static void celix_eventAdapter_onBundleEvent(void* handle, const celix_bundle_event_t* event) {
    celix_event_adapter_t* adapter = (celix_event_adapter_t*) handle;
    const char* topic = NULL;
    switch (event->type) {
        case CELIX_BUNDLE_EVENT_INSTALLED:
            topic = "celix/framework/BundleEvent/INSTALLED";
            break;
        case CELIX_BUNDLE_EVENT_STARTED:
            topic = "celix/framework/BundleEvent/STARTED";
            break;
        case CELIX_BUNDLE_EVENT_STOPPED:
            topic = "celix/framework/BundleEvent/STOPPED";
            break;
        case CELIX_BUNDLE_EVENT_UPDATED:
            topic = "celix/framework/BundleEvent/UPDATED";
            break;
        case CELIX_BUNDLE_EVENT_UNINSTALLED:
            topic = "celix/framework/BundleEvent/UNINSTALLED";
            break;
        case CELIX_BUNDLE_EVENT_RESOLVED:
            topic = "celix/framework/BundleEvent/RESOLVED";
            break;
        case CELIX_BUNDLE_EVENT_UNRESOLVED:
            topic = "celix/framework/BundleEvent/UNRESOLVED";
            break;
        default:
            //ignore unknown event
            return;
    }
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (props == NULL) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to create properties for bundle event.");
        return;
    }

    if (celix_properties_setLong(props, CELIX_EVENT_BUNDLE_ID, celix_bundle_getId(event->bnd)) != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to set bundle id to bundle event.");
        return;
    }
    const char* symName = celix_bundle_getSymbolicName(event->bnd);
    if (symName != NULL && celix_properties_set(props, CELIX_EVENT_BUNDLE_SYMBOLICNAME, symName) != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to set bundle symbolic name to bundle event.");
        return;
    }
    if (celix_properties_setVersion(props, CELIX_EVENT_BUNDLE_VERSION, celix_bundle_getVersion(event->bnd)) !=
        CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(adapter->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(adapter->logHelper, "Failed to set bundle version to bundle event.");
        return;
    }

    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&adapter->lock);
    celix_event_admin_service_t *eventAdminService = adapter->eventAdminService;
    if (eventAdminService == NULL) {
        return;
    }
    eventAdminService->postEvent(eventAdminService->handle, topic, props);
    return;
}

static void celix_eventAdapter_addFrameworkConditionServiceWithProperties(void* handle, void* svc,
                                                                          const celix_properties_t* props) {
    (void) svc;//unused
    celix_event_adapter_t* adapter = (celix_event_adapter_t* ) handle;
    const char* condId = celix_properties_get(props, CELIX_CONDITION_ID, NULL);
    assert(condId != NULL);//It must be set, because the filter is set to only accept services with this property set.
    const char* topic = NULL;
    if (celix_utils_stringEquals(condId, CELIX_CONDITION_ID_FRAMEWORK_READY)) {
        topic = "celix/framework/FrameworkEvent/STARTED";
    } else {
        topic = "celix/framework/FrameworkEvent/ERROR"; //It must be CELIX_CONDITION_ID_FRAMEWORK_ERROR, because the filter is set to only accept services with CELIX_CONDITION_ID_FRAMEWORK_READY or CELIX_CONDITION_ID_FRAMEWORK_ERROR properties.
    }

    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&adapter->lock);
    celix_event_admin_service_t* eventAdminService = adapter->eventAdminService;
    if (eventAdminService == NULL) {
        return;
    }
    eventAdminService->postEvent(eventAdminService->handle, topic, NULL);
    return;
}

