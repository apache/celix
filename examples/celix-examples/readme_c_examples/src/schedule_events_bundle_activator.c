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
#include <celix_bundle_activator.h>

typedef struct schedule_events_bundle_activator_data {
    celix_bundle_context_t* ctx;
    long scheduledEventId;
} schedule_events_bundle_activator_data_t;

void scheduleEventsBundle_oneShot(void* data) {
    schedule_events_bundle_activator_data_t* act = data;
    celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "One shot scheduled event fired");
}

void scheduleEventsBundle_process(void* data) {
    schedule_events_bundle_activator_data_t* act = data;
    celix_bundleContext_log(act->ctx, CELIX_LOG_LEVEL_INFO, "Recurring scheduled event fired");
}

static celix_status_t scheduleEventsBundle_start(schedule_events_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;

    //schedule recurring event
    {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "recurring scheduled event example";
        opts.initialDelayInSeconds = 0.1;
        opts.intervalInSeconds = 1.0;
        opts.callbackData = data;
        opts.callback = scheduleEventsBundle_process;
        data->scheduledEventId = celix_bundleContext_scheduleEvent(ctx, &opts);
    }

    //schedule one time event
    {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "one shot scheduled event example";
        opts.initialDelayInSeconds = 0.1;
        opts.callbackData = data;
        opts.callback = scheduleEventsBundle_oneShot;
        celix_bundleContext_scheduleEvent(ctx, &opts);
    }

    return CELIX_SUCCESS;
}

static celix_status_t scheduleEventsBundle_stop(schedule_events_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_bundleContext_removeScheduledEvent(ctx, data->scheduledEventId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(schedule_events_bundle_activator_data_t, scheduleEventsBundle_start, scheduleEventsBundle_stop)
