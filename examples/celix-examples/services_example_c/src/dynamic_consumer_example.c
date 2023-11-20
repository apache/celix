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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <celix_constants.h>

#include "example_calc.h"
#include <celix_bundle_activator.h>
#include <celix_compiler.h>

typedef struct activator_data {
    celix_bundle_context_t *ctx;
    pthread_t thread;

    pthread_mutex_t mutex; //protects running
    bool running;
    int trackCount;
} activator_data_t;

static bool isRunning(activator_data_t *data) {
    bool result = false;
    pthread_mutex_lock(&data->mutex);
    result = data->running;
    pthread_mutex_unlock(&data->mutex);
    return result;
}

static void setRunning(activator_data_t *data, bool val) {
    pthread_mutex_lock(&data->mutex);
    data->running = val;
    pthread_mutex_unlock(&data->mutex);
}

struct info {
    int result;
    int count;
};

static void useCalc(void *handle, void *svc) {
    struct info *i = handle;
    example_calc_t *calc = svc;
    i->result += calc->calc(calc->handle, 1);
    i->count += 1;
}

static void gccExample(activator_data_t *data) {
#ifdef USE_NESTED_FUNCTION_EXAMPLE

    int result = 0;
    long rank = 0;
    long svcId = 0;

    void use(void *handle, void *svc, const celix_properties_t *props) {
        example_calc_t *calc = svc;
        rank = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, -1L);
        svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
        result = calc->calc(calc->handle, 1);
    }

    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;

    opts.filter.serviceName = EXAMPLE_CALC_NAME;
    opts.callbackHandle = NULL; //can be null
    opts.useWithProperties = use;
    bool called = celix_bundleContext_useServiceWithOptions(data->ctx, &opts);

    printf("Called func %s. Result is %i, rank is %li and svc id is %li\n", called ? "called" : "not called", result, rank, svcId);

#endif
}

static void addSvc(activator_data_t *data, void *svc CELIX_UNUSED) {
    pthread_mutex_lock(&data->mutex);
    data->trackCount += 1;
    pthread_mutex_unlock(&data->mutex);
}

static void removeSvc(activator_data_t *data, void *svc CELIX_UNUSED) {
    pthread_mutex_lock(&data->mutex);
    data->trackCount -= 1;
    pthread_mutex_unlock(&data->mutex);
}

static void useHighest(activator_data_t *data CELIX_UNUSED, example_calc_t *svc, const celix_properties_t *props) {
    int result = svc->calc(svc->handle, 2);
    long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    long rank = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, -1L);
    printf("Called highest ranking service. Result is %i, svc id is %li, svc ranking is %li\n", result, svcId, rank);
}


void * run(void *handle) {
    activator_data_t *data = handle;

    printf("starting consumer thread\n");
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = EXAMPLE_CALC_NAME;
    opts.callbackHandle = data;
    opts.addWithProperties = (void*)addSvc;
    opts.removeWithProperties = (void*)removeSvc;
    long trkId = celix_bundleContext_trackServicesWithOptions(data->ctx, &opts);

    while (isRunning(data)) {
        struct info info;
        info.result = 0;
        info.count = 0;
        celix_bundleContext_useServices(data->ctx, EXAMPLE_CALC_NAME, &info, useCalc);
        printf("Called calc services %i times, total result is %i\n", info.count, info.result);

        gccExample(data); //gcc trampolines example (nested functions)

        celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;

        opts.filter.serviceName = EXAMPLE_CALC_NAME;
        opts.callbackHandle = data;
        opts.useWithProperties = (void*)useHighest;
        celix_bundleContext_useServiceWithOptions(data->ctx, &opts);

        pthread_mutex_lock(&data->mutex);
        int count = data->trackCount;
        pthread_mutex_unlock(&data->mutex);
        printf("Current tracking count is %i\n", count);

        sleep(5);
    }

    celix_bundleContext_stopTracker(data->ctx, trkId);
    printf("exiting consumer thread\n");

    pthread_exit(NULL);
    return NULL;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data != NULL) {
        data->ctx = ctx;
        data->trackCount = 0;
        data->running = true;
        pthread_mutex_init(&data->mutex, NULL);
        pthread_create(&data->thread, NULL, run, data);
    }
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx CELIX_UNUSED) {
    if (data != NULL) {
        setRunning(data, false);
        pthread_join(data->thread, NULL);
        pthread_mutex_destroy(&data->mutex);
    }
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)