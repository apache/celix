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
#include "celix_threads.h"
#include "framework_private.h"

#define CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED                                                                     \
    "CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED"                // TODO move to constants.h
#define CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT true // TODO move to constants.h

typedef struct celix_framework_bundle_activator {
    celix_bundle_context_t* ctx;
    celix_condition_t conditionInstance; /**< condition instance which can be used for multiple condition services.*/
    long trueConditionSvcId;             /**< service id of the condition service which is always true. */
    framework_listener_t listener;       /**< framework listener to check if the framework is ready. */

    celix_thread_mutex_t mutex; /**< protects below. */
    bool frameworkStartedEventReceived; /**< true if the framework started event is received. */
    long frameworkReadyConditionSvcId;   /**< service id of the condition service which is set when the framework is
                                            ready. */
    long checkFrameworkScheduledEventId; /**< event id of the scheduled event to check if the framework is ready. */

} celix_framework_bundle_activator_t;

static celix_status_t celix_frameworkBundle_frameworkEvent(void* handle, framework_event_t* event) {
    framework_listener_t* listener = handle;
    celix_framework_bundle_activator_t* act = listener->handle;
    if (event->type == OSGI_FRAMEWORK_EVENT_STARTED) {
        celixThreadMutex_lock(&act->mutex);
        act->frameworkStartedEventReceived = true;
        celixThreadMutex_unlock(&act->mutex);
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_frameworkBundle_create(celix_bundle_context_t* ctx, void** userData) {
    *userData = NULL;
    celix_framework_bundle_activator_t* act = calloc(1, sizeof(*act));
    if (act) {
        act->ctx = ctx;
        act->trueConditionSvcId = -1L;
        act->listener.handle = act;
        act->listener.frameworkEvent = celix_frameworkBundle_frameworkEvent;
        act->frameworkStartedEventReceived = false;
        act->frameworkReadyConditionSvcId = -1L;
        act->checkFrameworkScheduledEventId = -1L;
        act->conditionInstance.handle = act;
        celixThreadMutex_create(&act->mutex, NULL);
        *userData = act;
    }
    return act != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

static void celix_frameworkBundle_registerTrueCondition(celix_framework_bundle_activator_t* act) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    opts.svc = &act->conditionInstance;
    opts.properties = celix_properties_create();
    if (opts.properties) {
        celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_TRUE);
        act->trueConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
        celix_bundleContext_log(
            act->ctx, CELIX_LOG_LEVEL_INFO, "Registered true condition service with id %li", act->trueConditionSvcId);
    } else {
        celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for true condition service");
    }
}

static void celix_frameworkBundle_readyCheck(void* data) {
    celix_framework_bundle_activator_t* act = data;
    celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "celix_frameworkBundle_readyCheck");
    celixThreadMutex_lock(&act->mutex);
    bool ready = act->frameworkStartedEventReceived &&
                 celix_framework_isEventQueueEmpty(celix_bundleContext_getFramework(act->ctx));
    if (ready) {
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
        opts.svc = &act->conditionInstance;
        opts.properties = celix_properties_create();
        if (opts.properties) {
            celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
            act->frameworkReadyConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
        } else {
            celix_bundleContext_log(
                act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for framework.ready condition service");
        }
    } else {
        // not ready yet, schedule a new check
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.callback = celix_frameworkBundle_readyCheck;
        opts.callbackData = act;
        opts.initialDelayInSeconds = 0.01; // TBD use a small delay or accept a lot of scheduled events during startup.
        act->checkFrameworkScheduledEventId = celix_bundleContext_scheduleEvent(act->ctx, &opts);
    }
    celixThreadMutex_unlock(&act->mutex);
}

celix_status_t celix_frameworkBundle_start(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_activator_t* act = userData;

    bool conditionsEnabled = celix_bundleContext_getPropertyAsBool(
        ctx, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT);
    if (conditionsEnabled) {
        celix_frameworkBundle_registerTrueCondition(act);
        fw_addFrameworkListener(celix_bundleContext_getFramework(ctx), celix_bundleContext_getBundle(ctx), &act->listener);
        celix_frameworkBundle_readyCheck(act);
        return act->trueConditionSvcId >= 0 ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_frameworkBundle_stop(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_activator_t* act = userData;
    celix_status_t status = CELIX_SUCCESS;

    // remove framework listener
    fw_removeFrameworkListener(celix_bundleContext_getFramework(ctx), celix_bundleContext_getBundle(ctx), &act->listener);

    // stop ready check and remove framework ready condition service if present
    celixThreadMutex_lock(&act->mutex);
    celix_bundleContext_tryRemoveScheduledEventAsync(ctx, act->checkFrameworkScheduledEventId);
    act->checkFrameworkScheduledEventId = -1L;
    celix_bundleContext_unregisterServiceAsync(ctx, act->frameworkReadyConditionSvcId, NULL, NULL);
    act->frameworkReadyConditionSvcId = -1L;
    celixThreadMutex_unlock(&act->mutex);

    // remove true condition service
    celix_bundleContext_unregisterServiceAsync(ctx, act->trueConditionSvcId, NULL, NULL);
    act->trueConditionSvcId = -1L;

    // framework shutdown
    celix_framework_t* framework = celix_bundleContext_getFramework(ctx);
    celix_framework_shutdownAsync(framework);
    return status;
}

celix_status_t celix_frameworkBundle_destroy(void* userData,
                                                      celix_bundle_context_t* ctx __attribute__((unused))) {
    celix_framework_bundle_activator_t* act = userData;
    if (act) {
        celixThreadMutex_destroy(&act->mutex);
        free(userData);
    }
    return CELIX_SUCCESS;
}
