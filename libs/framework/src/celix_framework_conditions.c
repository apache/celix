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

#include "celix_condition.h"
#include "celix_constants.h"
#include "framework_private.h"

void celix_frameworkConditions_registerInitialConditions(celix_framework_t* framework) {
    // long svcId = -1L;
    // celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);
    // celixThreadMutex_lock(&framework->conditions.mutex);
    // celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    // opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    // opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    // opts.svc = &CELIX_CONDITION_INSTANCE;
    // opts.properties = celix_properties_create();
    // if (opts.properties) {
    //     celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_TRUE);
    //     svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    // }
    // celix_status_t addStatus = celix_arrayList_addLong(framework->conditions.initialConditionSvcIds, svcId);
    // if (addStatus != CELIX_SUCCESS) {
    //     celix_bundleContext_unregisterService(ctx, svcId);
    //     fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Error adding initial condition service id to list");
    // }
    // celixThreadMutex_unlock(&framework->conditions.mutex);
}

static void celix_frameworkConditions_checkFrameworkReady(void* data) {
    // celix_framework_t* framework = data;
    // celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);

    // celixThreadMutex_lock(&framework->dispatcher.mutex);
    // int eventQueueSize =
    //     framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
    // bool ready = celix_framework_isCurrentThreadTheEventLoop(framework) ?
    //         eventQueueSize == 1: /*note 1, because celix_frameworkConditions_checkFrameworkReady is called from an event*/
    //         eventQueueSize == 0;
    // bool cancel = framework->conditions.cancelRegistrations;
    // celixThreadMutex_unlock(&framework->dispatcher.mutex);

    // if (cancel) {
    //     fw_log(framework->logger,
    //            CELIX_LOG_LEVEL_DEBUG,
    //            "Framework is stopping or not active anymore, so no need to register the framework ready condition");
    //     return;
    // }

    // if (ready) {
    //     celixThreadMutex_lock(&framework->conditions.mutex);
    //     celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    //     opts.serviceName = CELIX_CONDITION_SERVICE_NAME;
    //     opts.serviceVersion = CELIX_CONDITION_SERVICE_VERSION;
    //     opts.svc = &CELIX_CONDITION_INSTANCE;
    //     opts.properties = celix_properties_create();
    //     if (opts.properties) {
    //         celix_properties_set(opts.properties, CELIX_CONDITION_ID, CELIX_CONDITION_ID_FRAMEWORK_READY);
    //         long svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    //         celix_status_t addStatus =
    //             celix_arrayList_addLong(framework->conditions.frameworkReadyConditionSvcIds, svcId);
    //         if (addStatus != CELIX_SUCCESS) {
    //             celix_bundleContext_unregisterService(ctx, svcId);
    //             fw_log(framework->logger,
    //                    CELIX_LOG_LEVEL_ERROR,
    //                    "Error adding framework ready condition service id to list");
    //         }
    //     }
    //     celixThreadMutex_unlock(&framework->conditions.mutex);
    // } else {
    //     // try again later on the event queue
    //     fprintf(stderr,
    //             "Event queue not empty, so try again later. bundle state is %s\n",
    //             celix_bundleState_getName(celix_bundle_getState(framework->bundle))); // TODO remove
    //     celix_framework_fireGenericEvent(framework,
    //                                      -1,
    //                                      CELIX_FRAMEWORK_BUNDLE_ID,
    //                                      "Check event queue for framework ready condition service",
    //                                      framework,
    //                                      celix_frameworkConditions_checkFrameworkReady,
    //                                      NULL,
    //                                      NULL);
    // }
}

void celix_frameworkConditions_registerFrameworkReadyConditions(celix_framework_t* framework) {
    // note called when all bundles are installed and started
    celix_frameworkConditions_checkFrameworkReady(framework);
}

// TODO rename to cleanup
void celix_frameworkConditions_unregisterConditions(celix_framework_t* framework) {
    // celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(framework);

    // celixThreadMutex_lock(&framework->conditions.mutex);
    // framework->conditions.cancelRegistrations = true;
    // celixThreadMutex_unlock(&framework->conditions.mutex);
    // celix_framework_waitUntilNoEventsForBnd(framework, CELIX_FRAMEWORK_BUNDLE_ID); //TODO maybe remove

    // celixThreadMutex_lock(&framework->conditions.mutex);
    // for (int i = 0; i < celix_arrayList_size(framework->conditions.initialConditionSvcIds); ++i) {
    //     long svcId = (long)celix_arrayList_getLong(framework->conditions.initialConditionSvcIds, i);
    //     fprintf(stderr, "Unregistering svc id %li\n", svcId);
    //     celix_bundleContext_unregisterServiceAsync(ctx, svcId, NULL, NULL);
    // }
    // celix_arrayList_clear(framework->conditions.initialConditionSvcIds);
    // for (int i = 0; i < celix_arrayList_size(framework->conditions.frameworkReadyConditionSvcIds); ++i) {
    //     long svcId = (long)celix_arrayList_getLong(framework->conditions.frameworkReadyConditionSvcIds, i);
    //     fprintf(stderr, "Unregistering svc id %li\n", svcId);
    //     celix_bundleContext_unregisterServiceAsync(ctx, svcId, NULL, NULL);
    // }
    // celix_arrayList_clear(framework->conditions.frameworkReadyConditionSvcIds);
    // celixThreadMutex_unlock(&framework->conditions.mutex);
}
