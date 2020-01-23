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
#include <string.h>

#include <celix_api.h>

#include "tst_service.h"
#include "calculator_service.h"
#include <unistd.h>

struct activator {
    long svcId;
    struct tst_service testSvc;

    long trackerId;

    pthread_mutex_t mutex; //protects below
    calculator_service_t *calc;
};

static void bndSetCalc(void* handle, void* svc) {
    struct activator * act = handle;
    pthread_mutex_lock(&act->mutex);
    act->calc = svc;
    pthread_mutex_unlock(&act->mutex);
}

static int bndTest(void *handle) {
    int status = 0;
    struct activator *act = handle;

    double result = -1.0;

    int retries = 40;

    pthread_mutex_lock(&act->mutex);
    calculator_service_t *local = act->calc;
    pthread_mutex_unlock(&act->mutex);

    while (local == NULL && retries > 0) {
        printf("Waiting for calc service .. %d\n", retries);
        usleep(100000);
        --retries;
        pthread_mutex_lock(&act->mutex);
        local = act->calc;
        pthread_mutex_unlock(&act->mutex);
    }


    pthread_mutex_lock(&act->mutex);
    int rc = 1;
    if (act->calc != NULL) {
        rc = act->calc->sqrt(act->calc->calculator, 4, &result);
        printf("calc result is %f\n", result);
    } else {
        printf("calc not ready\n");
    }
    pthread_mutex_unlock(&act->mutex);


    if (rc != 0 || result != 2.0) {
        status = 1;
    }
    return status;
}

static celix_status_t bndStart(struct activator *act, celix_bundle_context_t* ctx) {
    //initialize service struct
    act->testSvc.handle = act;
    act->testSvc.test = bndTest;

    //create mutex
    pthread_mutex_init(&act->mutex, NULL);

    //track (remote) service
    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.set = bndSetCalc;
        opts.callbackHandle = act;
        opts.filter.serviceName = CALCULATOR_SERVICE;
        opts.filter.ignoreServiceLanguage = true;
        act->trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register test service
    act->svcId = celix_bundleContext_registerService(ctx, &act->testSvc, TST_SERVICE_NAME, NULL);
    return CELIX_SUCCESS;
}

static celix_status_t bndStop(struct activator *act, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, act->svcId);
    celix_bundleContext_stopTracker(ctx, act->trackerId);
    pthread_mutex_destroy(&act->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bndStart, bndStop);