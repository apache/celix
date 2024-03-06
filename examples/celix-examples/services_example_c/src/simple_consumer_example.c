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

#include <celix_bundle_activator.h>
#include <celix_compiler.h>

#include "example_calc.h"


typedef struct activator_data {
    celix_bundle_context_t *ctx;
    long trkId;
} activator_data_t;


static void addSvc(activator_data_t *data CELIX_UNUSED, example_calc_t *calc) {
    int result = calc->calc(calc->handle, 2);
    printf("Added calc service, result is %i\n", result);
}

static void removeSvc(activator_data_t *data CELIX_UNUSED, example_calc_t *calc) {
    int result = calc->calc(calc->handle, 3);
    printf("Removing calc service, result is %i\n", result);
}

static void useCalc(activator_data_t *data CELIX_UNUSED, example_calc_t *calc) {
    int result = calc->calc(calc->handle, 2);
    printf("Called highest ranking service. Result is %i\n", result);
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;
    data->trkId = -1L;

    printf("Starting service tracker\n");
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = EXAMPLE_CALC_NAME;
    opts.callbackHandle = data;
    opts.addWithProperties = (void*)addSvc;
    opts.remove = (void*)removeSvc;
    data->trkId = celix_bundleContext_trackServicesWithOptions(data->ctx, &opts);

    printf("Trying to use calc service\n");
    celix_bundleContext_useService(data->ctx, EXAMPLE_CALC_NAME, data, (void*)useCalc);

    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(data->ctx, data->trkId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
