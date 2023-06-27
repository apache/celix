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

#include "framework_private.h"
#include "celix_condition.h"
#include "celix_constants.h"

void celix_frameworkConditions_registerInitialConditions(celix_framework_t* framework) {
    long svcId = -1L;
    celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);
    celixThreadMutex_lock(&framework->conditions.mutex);
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    opts.svc = &CELIX_CONDITION_INSTANCE;
    opts.properties = celix_properties_create();
    if (opts.properties) {
        celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
        svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    }
    celix_status_t addStatus = celix_arrayList_add(framework->conditions.initialConditionSvcIds, (void*)svcId);
    if (addStatus != CELIX_SUCCESS) {
        celix_bundleContext_unregisterService(ctx, svcId);
        fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Error adding initial condition service id to list");
    }
    celixThreadMutex_unlock(&framework->conditions.mutex);
}

void celix_frameworkConditions_unregisterInitialConditions(celix_framework_t* framework) {
    celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);
    celixThreadMutex_lock(&framework->conditions.mutex);
    for (int i = 0; i < celix_arrayList_size(framework->conditions.initialConditionSvcIds); ++i) {
        long svcId = (long)celix_arrayList_get(framework->conditions.initialConditionSvcIds, i);
        celix_bundleContext_unregisterServiceAsync(ctx, svcId, NULL, NULL);
    }
    celix_arrayList_clear(framework->conditions.initialConditionSvcIds);
    celixThreadMutex_unlock(&framework->conditions.mutex);
}

static void celix_frameworkConditions_checkFrameworkReady(void* data) {
    celix_framework_t* framework = data;
    celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);

    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool isEventQueueEmpty = framework->dispatcher.eventQueueSize == 0 && celix_arrayList_size(framework->dispatcher.dynamicEventQueue) == 0;
    bool isActive = framework->dispatcher.active;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    if (!isActive) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_DEBUG, "Framework is not active anymore, so no need to register the framework ready condition");
        return;
    }

    if (isEventQueueEmpty) {
        celixThreadMutex_lock(&framework->conditions.mutex);
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
        opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
        opts.svc = &CELIX_CONDITION_INSTANCE;
        opts.properties = celix_properties_create();
        if (opts.properties) {
            celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
            long svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
            celix_status_t addStatus = celix_arrayList_add(framework->conditions.frameworkReadyConditionSvcIds, (void*)svcId);
            if (addStatus != CELIX_SUCCESS) {
                celix_bundleContext_unregisterService(ctx, svcId);
                fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Error adding framework ready condition service id to list");
            }
        }
        celixThreadMutex_unlock(&framework->conditions.mutex);
    } else {
        //try again later on the event queue
        celix_framework_fireGenericEvent(framework, -1, CELIX_FRAMEWORK_BUNDLE_ID, "Check event queue for framework ready condition service", framework, celix_frameworkConditions_checkFrameworkReady, NULL, NULL);
    }
}

void celix_frameworkConditions_registerFrameworkReadyConditions(celix_framework_t* framework) {
    //note called when all bundles are installed and started
    celix_frameworkConditions_checkFrameworkReady(framework);
}

void celix_frameworkConditions_unregisterFrameworkReadyConditions(celix_framework_t* framework) {
    celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);
    celixThreadMutex_lock(&framework->conditions.mutex);
    for (int i = 0; i < celix_arrayList_size(framework->conditions.frameworkReadyConditionSvcIds); ++i) {
        long svcId = (long)celix_arrayList_get(framework->conditions.frameworkReadyConditionSvcIds, i);
        celix_bundleContext_unregisterServiceAsync(ctx, svcId, NULL, NULL);
    }
    celix_arrayList_clear(framework->conditions.frameworkReadyConditionSvcIds);
    celixThreadMutex_unlock(&framework->conditions.mutex);
}