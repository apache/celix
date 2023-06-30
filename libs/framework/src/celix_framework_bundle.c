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
#include "framework_private.h"

#define CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT true

/**
 * @brief Celix framework bundle activator struct.
 */
typedef struct celix_framework_bundle_activator {
    celix_bundle_context_t* ctx;
    celix_condition_t conditionInstance; /**< condition instance which can be used for multiple condition services.*/
    framework_listener_t listener;       /**< framework listener to check if the framework is ready. */

    celix_thread_mutex_t mutex;               /**< protects below. */
    long trueConditionSvcId;                  /**< service id of the condition service which is always true. */
    bool frameworkStartedEventReceived;       /**< true if the framework started event is received. */
    bool frameworkErrorEventReceived;         /**< true if the framework error event is received. */
    long frameworkReadyOrErrorConditionSvcId; /**< service id of the condition service which is set when the framework
                                            is ready or started up with an error */
    long checkFrameworkScheduledEventId; /**< event id of the scheduled event to check if the framework is ready. */
} celix_framework_bundle_activator_t;

static celix_status_t celix_frameworkBundle_frameworkEvent(void* handle, framework_event_t* event) {
    framework_listener_t* listener = handle;
    celix_framework_bundle_activator_t* act = listener->handle;
    if (event->type == OSGI_FRAMEWORK_EVENT_STARTED || event->type == OSGI_FRAMEWORK_EVENT_ERROR) {
        celixThreadMutex_lock(&act->mutex);
        if (event->type == OSGI_FRAMEWORK_EVENT_STARTED) {
            act->frameworkStartedEventReceived = true;
        } else {
            act->frameworkErrorEventReceived = true;
        }
        celix_bundleContext_wakeupScheduledEvent(act->ctx, act->checkFrameworkScheduledEventId);
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
        act->frameworkErrorEventReceived = false;
        act->frameworkReadyOrErrorConditionSvcId = -1L;
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
    } else {
        celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for true condition service");
    }
}

static void celix_frameworkBundle_readyCheck(void* data) {
    celix_framework_bundle_activator_t* act = data;
    celixThreadMutex_lock(&act->mutex);
    
    bool ready = act->frameworkStartedEventReceived &&
                 celix_framework_isEventQueueEmpty(celix_bundleContext_getFramework(act->ctx));
    bool error = act->frameworkErrorEventReceived;

    if (ready || error) {
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
        opts.svc = &act->conditionInstance;
        opts.properties = celix_properties_create();
        if (opts.properties) {
            if (ready) {
                celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
            } else /*error*/ {
                celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_ERROR);
                celix_bundleContext_log(act->ctx,
                                        CELIX_LOG_LEVEL_INFO,
                                        "Framework error received -> no framework.ready condition service will be registered");
            }
            celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "Registering framework.ready/framework.error condition service %s", celix_properties_get(opts.properties, CELIX_CONDITION_ID, "!ERROR!"));
            act->frameworkReadyOrErrorConditionSvcId = celix_bundleContext_registerServiceWithOptionsAsync(act->ctx, &opts);
        } else {
            celix_bundleContext_log(
                act->ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create properties for framework.ready/framework.error condition service");
        }
        celix_bundleContext_removeScheduledEventAsync(act->ctx, act->checkFrameworkScheduledEventId);
        act->checkFrameworkScheduledEventId = -1L;
    }
    celixThreadMutex_unlock(&act->mutex);
}

static void celix_frameworkBundle_startReadyCheck(celix_framework_bundle_activator_t* act) {
    celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
    opts.name = "celix_frameworkBundle_readyCheck";
    opts.callback = celix_frameworkBundle_readyCheck;
    opts.callbackData = act;
    opts.intervalInSeconds = 0.001;
    act->checkFrameworkScheduledEventId = celix_bundleContext_scheduleEvent(act->ctx, &opts);
}

celix_status_t celix_frameworkBundle_start(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_activator_t* act = userData;

    bool conditionsEnabled = celix_bundleContext_getPropertyAsBool(
        ctx, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED_DEFAULT);
    if (conditionsEnabled) {
        celix_frameworkBundle_registerTrueCondition(act);
        fw_addFrameworkListener(
            celix_bundleContext_getFramework(ctx), celix_bundleContext_getBundle(ctx), &act->listener);
        celix_frameworkBundle_startReadyCheck(act);
        return act->trueConditionSvcId >= 0 ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_frameworkBundle_stop(void* userData, celix_bundle_context_t* ctx) {
    celix_framework_bundle_activator_t* act = userData;
    celix_framework_t* framework = celix_bundleContext_getFramework(ctx);
    celix_status_t status = CELIX_SUCCESS;

    // remove framework listener
    fw_removeFrameworkListener(framework, celix_bundleContext_getBundle(ctx), &act->listener);

    // stop ready check
    celixThreadMutex_lock(&act->mutex);
    long checkEventId = act->checkFrameworkScheduledEventId;
    act->checkFrameworkScheduledEventId = -1L;
    celixThreadMutex_unlock(&act->mutex);
    celix_bundleContext_removeScheduledEvent(ctx, checkEventId);

    // remove framework true condition service and - if present - framework.ready condition service,
    celixThreadMutex_lock(&act->mutex);
    long trueSvcId = act->trueConditionSvcId;
    long readyOrErrorSvcId = act->frameworkReadyOrErrorConditionSvcId;
    act->trueConditionSvcId = -1L;
    act->frameworkReadyOrErrorConditionSvcId = -1L;
    celixThreadMutex_unlock(&act->mutex);
    celix_bundleContext_unregisterService(ctx, readyOrErrorSvcId);
    celix_bundleContext_unregisterService(ctx, trueSvcId);

    // framework shutdown
    celix_framework_shutdownAsync(framework);
    return status;
}

celix_status_t celix_frameworkBundle_destroy(void* userData, celix_bundle_context_t* ctx __attribute__((unused))) {
    celix_framework_bundle_activator_t* act = userData;
    if (act) {
        celixThreadMutex_destroy(&act->mutex);
        free(userData);
    }
    return CELIX_SUCCESS;
}
