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
#include <celix_threads.h>
#include <celix_constants.h>
#include <celix_shell_command.h>

typedef struct track_command_services_example_data {
    long trackerId;
    celix_thread_mutex_t mutex; //protects below
    celix_array_list_t* commandServices;
} track_command_services_example_data_t;


static void addShellCommandService(void* data,void* svc, const celix_properties_t * properties) {
    track_command_services_example_data_t* activatorData = data;
    celix_shell_command_t* cmdSvc = svc;

    printf("Adding command service with svc id %li\n", celix_properties_getAsLong(properties, CELIX_FRAMEWORK_SERVICE_ID, -1));
    celixThreadMutex_lock(&activatorData->mutex);
    celix_arrayList_add(activatorData->commandServices, cmdSvc);
    printf("Nr of command service found: %i\n", celix_arrayList_size(activatorData->commandServices));
    celixThreadMutex_unlock(&activatorData->mutex);
}

static void removeShellCommandService(void* data,void* svc, const celix_properties_t * properties) {
    track_command_services_example_data_t* activatorData = data;
    celix_shell_command_t* cmdSvc = svc;

    printf("Removing command service with svc id %li\n", celix_properties_getAsLong(properties, CELIX_FRAMEWORK_SERVICE_ID, -1));
    celixThreadMutex_lock(&activatorData->mutex);
    celix_arrayList_remove(activatorData->commandServices, cmdSvc);
    printf("Nr of command service found: %i\n", celix_arrayList_size(activatorData->commandServices));
    celixThreadMutex_unlock(&activatorData->mutex);
}

static celix_status_t track_command_services_example_start(track_command_services_example_data_t *data, celix_bundle_context_t *ctx) {
    celixThreadMutex_create(&data->mutex, NULL);
    data->commandServices = celix_arrayList_createPointerArray();

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=my_command)";
    opts.callbackHandle = data;
    opts.addWithProperties = addShellCommandService;
    opts.removeWithProperties = removeShellCommandService;
    data->trackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    return CELIX_SUCCESS;
}

static celix_status_t track_command_services_example_stop(track_command_services_example_data_t *data, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(ctx, data->trackerId);
    celixThreadMutex_lock(&data->mutex);
    celix_arrayList_destroy(data->commandServices);
    celixThreadMutex_unlock(&data->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(track_command_services_example_data_t, track_command_services_example_start, track_command_services_example_stop)