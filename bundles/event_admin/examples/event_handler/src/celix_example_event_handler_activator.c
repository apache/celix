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
#include <stdio.h>

#include "celix_event_handler_service.h"
#include "celix_event_constants.h"
#include "celix_bundle_activator.h"

typedef struct celix_event_handler_example_activator {
    celix_event_handler_service_t service;
    long serviceId;
} celix_event_handler_example_activator_t;

static celix_status_t celix_eventHandlerExampleActivator_handleEvent(void *handle, const char *topic, const celix_properties_t *properties) {
    (void)handle;
    (void)properties;
    printf("Received event with topic %s\n\n", topic);
    return CELIX_SUCCESS;
}

celix_status_t celix_eventHandlerExampleActivator_start(celix_event_handler_example_activator_t *act, celix_bundle_context_t *ctx) {
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, CELIX_EVENT_TOPIC, "example/*");
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
    act->service.handle = act;
    act->service.handleEvent = celix_eventHandlerExampleActivator_handleEvent;
    act->serviceId = celix_bundleContext_registerService(ctx, &act->service, CELIX_EVENT_HANDLER_SERVICE_NAME, properties);
    return CELIX_SUCCESS;
}

celix_status_t celix_eventHandlerExampleActivator_stop(celix_event_handler_example_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->serviceId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_event_handler_example_activator_t, celix_eventHandlerExampleActivator_start, celix_eventHandlerExampleActivator_stop)