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
#include <unistd.h>
#include <stdbool.h>

#include "celix_event_admin_service.h"
#include "celix_event_constants.h"
#include "celix_bundle_activator.h"
#include "celix_threads.h"

typedef struct celix_event_publisher_example_activator {
    celix_thread_rwlock_t svcLock;
    celix_event_admin_service_t *service;
    long serviceTrkId;
    bool running;
    celix_thread_t sendEventThread;
} celix_event_publisher_example_activator_t;

static void *celix_eventPublisherExampleActivator_sendEventThread(void *handle) {
    celix_event_publisher_example_activator_t *act = (celix_event_publisher_example_activator_t *)handle;
    while(act->running) {
        celixThreadRwlock_readLock(&act->svcLock);
        celix_event_admin_service_t *svc = act->service;
        if (svc != NULL) {
            {
                printf("Sending sync event\n");
                celix_autoptr(celix_properties_t) props = celix_properties_create();
                celix_properties_set(props, CELIX_EVENT_TOPIC, "example/syncEvent");
                celix_properties_set(props, "example", "data");
                svc->sendEvent(svc->handle, "example/syncEvent", props);
            }
            {
                printf("Sending async event\n");
                celix_autoptr(celix_properties_t) props = celix_properties_create();
                celix_properties_set(props, CELIX_EVENT_TOPIC, "example/asyncEvent");
                celix_properties_set(props, "example", "data");
                svc->postEvent(svc->handle, "example/asyncEvent", props);
            }
            {
                printf("Sending remote enable sync event\n");
                celix_autoptr(celix_properties_t) props = celix_properties_create();
                celix_properties_set(props, CELIX_EVENT_TOPIC, "example/remoteSyncEvent");
                celix_properties_setBool(props, CELIX_EVENT_REMOTE_ENABLE, true);
                celix_properties_set(props, "example", "data");
                svc->sendEvent(svc->handle, "example/remoteSyncEvent", props);
            }
            {
                printf("Sending remote enable async event\n");
                celix_autoptr(celix_properties_t) props = celix_properties_create();
                celix_properties_set(props, CELIX_EVENT_TOPIC, "example/remoteAsyncEvent");
                celix_properties_setBool(props, CELIX_EVENT_REMOTE_ENABLE, true);
                celix_properties_set(props, "example", "data");
                svc->postEvent(svc->handle, "example/remoteAsyncEvent", props);
            }
        }
        celixThreadRwlock_unlock(&act->svcLock);
        sleep(3);
    }
    return CELIX_SUCCESS;
}

static void celix_eventPublisherExampleActivator_setEventAdminService(void *handle, void *svc) {
    celix_event_publisher_example_activator_t *act = (celix_event_publisher_example_activator_t *)handle;
    celixThreadRwlock_writeLock(&act->svcLock);
    act->service = svc;
    celixThreadRwlock_unlock(&act->svcLock);
}

static void onEventAdminServiceTrackerStopped(void *data) {
    celix_event_publisher_example_activator_t *act = data;
    celixThreadRwlock_destroy(&act->svcLock);
}

celix_status_t celix_eventPublisherExampleActivator_start(celix_event_publisher_example_activator_t *act, celix_bundle_context_t *ctx) {
    celix_status_t status = celixThreadRwlock_create(&act->svcLock, NULL);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_autoptr(celix_thread_rwlock_t) svcLock = &act->svcLock;

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = CELIX_EVENT_ADMIN_SERVICE_NAME;
    opts.filter.versionRange = CELIX_EVENT_ADMIN_SERVICE_USE_RANGE;
    opts.callbackHandle = act;
    opts.set = celix_eventPublisherExampleActivator_setEventAdminService;
    act->serviceTrkId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    if (act->serviceTrkId < 0) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    celix_steal_ptr(svcLock);

    act->running = true;
    status = celixThread_create(&act->sendEventThread, NULL, celix_eventPublisherExampleActivator_sendEventThread, act);
    if (status != CELIX_SUCCESS) {
        celix_bundleContext_stopTrackerAsync(ctx, act->serviceTrkId, act, onEventAdminServiceTrackerStopped);//celix_bundleActivator_destroy will wait for all events
        return status;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_eventPublisherExampleActivator_stop(celix_event_publisher_example_activator_t *act, celix_bundle_context_t *ctx) {
    act->running = false;
    celixThread_join(act->sendEventThread, NULL);
    celix_bundleContext_stopTrackerAsync(ctx, act->serviceTrkId, act, onEventAdminServiceTrackerStopped);//celix_bundleActivator_destroy will wait for all events
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_event_publisher_example_activator_t, celix_eventPublisherExampleActivator_start, celix_eventPublisherExampleActivator_stop)