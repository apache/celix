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

#include "celix_framework_bundle.h"

#include "celix_condition.h"
#include "celix_constants.h"
#include "celix_threads.h"
#include "celix_dependency_manager.h"
#include "framework_private.h"

#define CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT true

/**
 * @brief Celix framework bundle activator struct.
 */
typedef struct celix_framework_bundle {
    celix_bundle_context_t* ctx;
    celix_condition_t conditionInstance; /**< condition instance which can be used for multiple condition services.*/
    framework_listener_t listener;       /**< framework listener to check if the framework is ready. */

    celix_thread_mutex_t mutex;               /**< protects below. */
    long trueConditionSvcId;                  /**< service id of the condition service which is always true. */
    long frameworkReadyOrErrorConditionSvcId; /**< service id of the condition service which is set when the framework
                                            is ready or started up with an error */
    long checkComponentsScheduledEventId; /**< event id of the scheduled event to check if the framework is ready. */
    long componentsReadyConditionSvcId;   /**< service id of the condition service which is set when all components are
                                            ready. */
} celix_framework_bundle_t;

celix_status_t celix_frameworkBundle_create(celix_bundle_context_t* ctx, void** userData) {
    *userData = NULL;
    celix_framework_bundle_t* act = calloc(1, sizeof(*act));
    if (!act) {
        return ENOMEM;
    }

    celix_status_t status = celixThreadMutex_create(&act->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        free(act);
        return status;
    }

    act->ctx = ctx;
    act->trueConditionSvcId = -1L;
    act->listener.handle = act;
    act->listener.frameworkEvent = celix_frameworkBundle_handleFrameworkEvent;
    act->frameworkReadyOrErrorConditionSvcId = -1L;
    act->checkComponentsScheduledEventId = -1L;
    act->componentsReadyConditionSvcId = -1L;
    act->conditionInstance.handle = act;
    *userData = act;

    return CELIX_SUCCESS;
}

static void celix_frameworkBundle_registerTrueCondition(celix_framework_bundle_t* act) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    opts.svc = &act->conditionInstance;
    opts.properties = celix_properties_create();
    if (opts.properties) {
        celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_TRUE);
        act->trueConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
    } else {
        celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for true condition service");
    }
}

celix_status_t celix_frameworkBundle_handleFrameworkEvent(void* handle, framework_event_t* event) {
    framework_listener_t* listener = handle;
    celix_framework_bundle_t* act = listener->handle;
    if (event->type == OSGI_FRAMEWORK_EVENT_STARTED || event->type == OSGI_FRAMEWORK_EVENT_ERROR) {
        celixThreadMutex_lock(&act->mutex);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
        opts.svc = &act->conditionInstance;
        opts.properties = celix_properties_create();
        if (opts.properties) {
            if (event->type == OSGI_FRAMEWORK_EVENT_STARTED) {
                celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
                celix_bundleContext_log(
                        act->ctx,
                        CELIX_LOG_LEVEL_DEBUG,
                        "Framework started event received -> registering framework.ready condition service");
            } else /*error*/ {
                celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_ERROR);
                celix_bundleContext_log(
                        act->ctx,
                        CELIX_LOG_LEVEL_INFO,
                        "Framework error event received -> registering framework.error condition service");
            }
            act->frameworkReadyOrErrorConditionSvcId =
                    celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
            celix_bundleContext_wakeupScheduledEvent(act->ctx, act->checkComponentsScheduledEventId);
        } else {
            celix_bundleContext_log(act->ctx,
                                    CELIX_LOG_LEVEL_ERROR,
                                    "Cannot create properties for framework.ready/framework.error condition service");
        }
        celixThreadMutex_unlock(&act->mutex);
    }
    return CELIX_SUCCESS;
}

void celix_frameworkBundle_componentsCheck(void* data) {
    celix_framework_bundle_t* act = data;
    celix_dependency_manager_t* mng = celix_bundleContext_getDependencyManager(act->ctx);

    celixThreadMutex_lock(&act->mutex);
    bool allComponentsActive = celix_dependencyManager_allComponentsActive(mng);
    bool ready = allComponentsActive && act->frameworkReadyOrErrorConditionSvcId >= 0;
    if (ready) {
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
        opts.svc = &act->conditionInstance;
        opts.properties = celix_properties_create();
        if (opts.properties) {
            celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_COMPONENTS_READY);
            celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_DEBUG, "Registering components.ready condition service");
            act->componentsReadyConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
        } else {
            celix_bundleContext_log(
                    act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for components.ready condition service");
        }
        celix_bundleContext_removeScheduledEventAsync(act->ctx, act->checkComponentsScheduledEventId);
        act->checkComponentsScheduledEventId = -1L;
    }
    celixThreadMutex_unlock(&act->mutex);
}

static void celix_frameworkBundle_startComponentsCheck(celix_framework_bundle_t* act) {
    celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
    opts.name = "celix_frameworkBundle_componentsCheck";
    opts.callback = celix_frameworkBundle_componentsCheck;
    opts.callbackData = act;
    opts.initialDelayInSeconds = 1; //note will be wakeup by framework event
    opts.intervalInSeconds = 0.001;
    act->checkComponentsScheduledEventId = celix_bundleContext_scheduleEvent(act->ctx, &opts);
}

celix_status_t celix_frameworkBundle_start(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_t* act = userData;

    bool conditionsEnabled = celix_bundleContext_getPropertyAsBool(
        ctx, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT);
    if (!conditionsEnabled) {
        return CELIX_SUCCESS;
    }

    celix_frameworkBundle_registerTrueCondition(act);
    if (act->trueConditionSvcId < 0) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    fw_addFrameworkListener(
        celix_bundleContext_getFramework(ctx), celix_bundleContext_getBundle(ctx), &act->listener);
    celix_frameworkBundle_startComponentsCheck(act);

    return CELIX_SUCCESS;
}

celix_status_t celix_frameworkBundle_stop(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_t* act = userData;
    celix_framework_t* framework = celix_bundleContext_getFramework(ctx);

    // remove framework listener
    fw_removeFrameworkListener(framework, celix_bundleContext_getBundle(ctx), &act->listener);

    // stop ready check
    celixThreadMutex_lock(&act->mutex);
    long checkEventId = act->checkComponentsScheduledEventId;
    act->checkComponentsScheduledEventId = -1L;
    celixThreadMutex_unlock(&act->mutex);
    celix_bundleContext_removeScheduledEvent(ctx, checkEventId);

    // remove framework true condition service and - if present - framework.ready condition service,
    celixThreadMutex_lock(&act->mutex);
    long trueConditionSvcId = act->trueConditionSvcId;
    long frameworkReadyOrErrorConditionSvcId = act->frameworkReadyOrErrorConditionSvcId;
    long componentsReadyConditionSvcId = act->componentsReadyConditionSvcId;
    act->trueConditionSvcId = -1L;
    act->frameworkReadyOrErrorConditionSvcId = -1L;
    act->componentsReadyConditionSvcId = -1L;
    celixThreadMutex_unlock(&act->mutex);

    celix_bundleContext_unregisterService(ctx, componentsReadyConditionSvcId);
    celix_bundleContext_unregisterService(ctx, frameworkReadyOrErrorConditionSvcId);
    celix_bundleContext_unregisterService(ctx, trueConditionSvcId);

    // framework shutdown
    celix_framework_shutdownAsync(framework);
    return CELIX_SUCCESS;
}

celix_status_t celix_frameworkBundle_destroy(void* userData, celix_bundle_context_t* ctx __attribute__((unused))) {
    celix_framework_bundle_t* act = userData;
    if (act) {
        celixThreadMutex_destroy(&act->mutex);
        free(userData);
    }
    return CELIX_SUCCESS;
}
