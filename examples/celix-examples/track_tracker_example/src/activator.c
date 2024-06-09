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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <celix_bundle_activator.h>
#include <celix_compiler.h>

#define CALC_SERVICE_NAME   "CALC_SERVICE"
typedef struct calc_service {
    void *handle;
    int (*calc)(void *handle, int a);
} calc_service_t;

typedef struct activator_data {
    int incr;

    long trackerId;

    long calcTrk1;
    long calcTrk2;

    calc_service_t svc;
    long svcId;
} activator_data_t;

static void addCalcTracker(void *handle, const celix_service_tracker_info_t *info) {
    activator_data_t *act = handle;
    printf("Calc service tracker created with filter '%s'. Tracked with tracker id %li\n", info->filter->filterStr, act->trackerId);
    const char *prop1Val = celix_filter_findAttribute(info->filter, "prop1");
    const char *prop2Val = celix_filter_findAttribute(info->filter, "prop2");
    printf("Found filter attribute value for prop1: %s and prop2: %s\n", prop1Val, prop2Val);
}

static void removeCalcTracker(void *handle, const celix_service_tracker_info_t *info) {
    activator_data_t *act = handle;
    printf("Calc service tracker with filter '%s' removed. Tracked with tracker id %li\n", info->filter->filterStr, act->trackerId);
}

static void addCalcSvc(void* handle CELIX_UNUSED, void* svc) {
    calc_service_t *calc = svc;
    printf("Calc service added. result calc(2) is %i\n", calc->calc(calc->handle, 2));
}

static void removeCalcSvc(void* handle CELIX_UNUSED, void* svc CELIX_UNUSED) {
    printf("Calc service removed\n");
}

static int calc(void *handle, int a) {
    activator_data_t *act = handle;
    return act->incr + a;
}

celix_status_t activator_start(activator_data_t* act, celix_bundle_context_t *ctx) {
    act->incr = 42;

    act->trackerId = celix_bundleContext_trackServiceTrackers(ctx, CALC_SERVICE_NAME, act, addCalcTracker, removeCalcTracker);

    celix_service_tracking_options_t opts1 = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts1.filter.serviceName = CALC_SERVICE_NAME;
    opts1.callbackHandle = act;
    opts1.add = addCalcSvc;
    opts1.remove = removeCalcSvc;
    act->calcTrk1 = celix_bundleContext_trackServicesWithOptions(ctx, &opts1);

    celix_service_tracking_options_t opts2 = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts2.filter.serviceName = CALC_SERVICE_NAME;
    opts2.filter.filter = "(&(prop1=val1)(prop2=val2))";
    opts2.callbackHandle = act;
    opts2.add = addCalcSvc;
    opts2.remove = removeCalcSvc;
    act->calcTrk2 = celix_bundleContext_trackServicesWithOptions(ctx, &opts2);

    act->svc.handle = act;
    act->svc.calc = calc;
    //note only triggers on calcTrk1, because of filter restrictions
    act->svcId = celix_bundleContext_registerService(ctx, &act->svc, CALC_SERVICE_NAME, NULL);

    return CELIX_SUCCESS;
}

celix_status_t activator_stop(activator_data_t* act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->svcId);
    celix_bundleContext_stopTracker(ctx, act->calcTrk1);
    celix_bundleContext_stopTracker(ctx, act->calcTrk2);
    usleep(1000);
    celix_bundleContext_stopTracker(ctx, act->trackerId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop);