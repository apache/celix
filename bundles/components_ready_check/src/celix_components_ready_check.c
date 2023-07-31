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

#include "celix_components_ready_check.h"

#include <assert.h>
#include <stdlib.h>

#include "celix_components_ready_constants.h"
#include "celix_condition.h"
#include "celix_dependency_manager.h"
#include "celix_framework.h"
#include "celix_threads.h"

struct celix_components_ready_check {
    celix_bundle_context_t* ctx;
    celix_condition_t conditionInstance;  /**< condition instance which can be used for multiple condition services.*/
    celix_thread_mutex_t mutex;           /**< mutex to protect the fields below. */
    long frameworkReadyTrackerId;         /**< tracker id for the framework ready condition service. */
    long checkComponentsScheduledEventId; /**< event id of the scheduled event to check if the framework is ready. */
    long componentsReadyConditionSvcId;   /**< service id of the condition service which is set when all components are
                                            ready. */
};

celix_components_ready_check_t* celix_componentsReadyCheck_create(celix_bundle_context_t* ctx) {
    celix_status_t status;
    celix_components_ready_check_t* rdy = calloc(1, sizeof(*rdy));
    if (rdy) {
        rdy->ctx = ctx;
        rdy->frameworkReadyTrackerId = -1L;
        rdy->checkComponentsScheduledEventId = -1L;
        rdy->componentsReadyConditionSvcId = -1L;

        status = celixThreadMutex_create(&rdy->mutex, NULL);
        if (status != CELIX_SUCCESS) {
            goto mutex_init_err;
        }

        char filter[32];
        snprintf(filter, sizeof(filter), "(%s=%s)", CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);

        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.filter.filter = filter;
        opts.set = celix_componentReadyCheck_setFrameworkReadySvc;
        opts.callbackHandle = rdy;
        rdy->frameworkReadyTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
        if (rdy->frameworkReadyTrackerId < 0) {
            goto tracker_err;
        }
    } else {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create components ready check. ENOMEM");
    }
    return rdy;
mutex_init_err:
    celix_bundleContext_log(
        ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create components ready check. Cannot create mutex. Got status %d", status);
    free(rdy);
    return NULL;
tracker_err:
    celix_bundleContext_log(ctx,
                            CELIX_LOG_LEVEL_ERROR,
                            "Cannot create components ready check. Cannot track framework ready condition "
                            "service. Got tracker id %ld",
                            rdy->frameworkReadyTrackerId);
    celixThreadMutex_destroy(&rdy->mutex);
    free(rdy);
    return NULL;
}

void celix_componentsReadyCheck_destroy(celix_components_ready_check_t* rdy) {
    if (rdy) {
        celix_bundleContext_stopTracker(rdy->ctx, rdy->frameworkReadyTrackerId);

        celixThreadMutex_lock(&rdy->mutex);
        long schedId = rdy->checkComponentsScheduledEventId;
        rdy->checkComponentsScheduledEventId = -1L;
        celixThreadMutex_unlock(&rdy->mutex);
        celix_bundleContext_removeScheduledEvent(rdy->ctx, schedId);

        celixThreadMutex_lock(&rdy->mutex);
        long svcId = rdy->componentsReadyConditionSvcId;
        rdy->componentsReadyConditionSvcId = -1L;
        celixThreadMutex_unlock(&rdy->mutex);
        celix_bundleContext_unregisterService(rdy->ctx, svcId);

        free(rdy);
    }
}

void celix_componentReadyCheck_registerCondition(celix_components_ready_check_t* rdy) {
    //precondition rdy->mutex is locked
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    opts.svc = &rdy->conditionInstance;
    opts.properties = celix_properties_create();
    if (opts.properties) {
        celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_COMPONENTS_READY);
        celix_bundleContext_log(rdy->ctx, CELIX_LOG_LEVEL_DEBUG, "Registering components.ready condition service");
        rdy->componentsReadyConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(rdy->ctx, &opts);
    } else {
        celix_bundleContext_log(
            rdy->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for components.ready condition service");
    }
}

static void celix_componentReadyCheck_check(void* data) {
    celix_components_ready_check_t* rdy = data;
    celix_dependency_manager_t* mng = celix_bundleContext_getDependencyManager(rdy->ctx);
    celix_framework_t* fw = celix_bundleContext_getFramework(rdy->ctx);
    bool ready = celix_dependencyManager_allComponentsActive(mng) && celix_framework_isEventQueueEmpty(fw);
    if (ready) {
        celixThreadMutex_lock(&rdy->mutex);
        celix_bundleContext_removeScheduledEventAsync(rdy->ctx, rdy->checkComponentsScheduledEventId);
        rdy->checkComponentsScheduledEventId = -1L;
        celix_componentReadyCheck_registerCondition(rdy);
        celixThreadMutex_unlock(&rdy->mutex);
    }
}

void celix_componentReadyCheck_setFrameworkReadySvc(void* handle, void* svc) {
    celix_components_ready_check_t* rdy = handle;
    celixThreadMutex_lock(&rdy->mutex);
    if (svc && rdy->checkComponentsScheduledEventId < 0) {
        // framework ready, now periodically check if all components are ready
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "celix_componentReady_check";
        opts.callback = celix_componentReadyCheck_check;
        opts.callbackData = rdy;
        opts.initialDelayInSeconds = 0.1;
        opts.intervalInSeconds = 0.1;
        rdy->checkComponentsScheduledEventId = celix_bundleContext_scheduleEvent(rdy->ctx, &opts);
        if (rdy->checkComponentsScheduledEventId < 0) {
            celix_bundleContext_log(rdy->ctx,
                                    CELIX_LOG_LEVEL_ERROR,
                                    "Cannot schedule components ready check. Got event id %ld",
                                    rdy->checkComponentsScheduledEventId);
        }
    }
    celixThreadMutex_unlock(&rdy->mutex);
}
