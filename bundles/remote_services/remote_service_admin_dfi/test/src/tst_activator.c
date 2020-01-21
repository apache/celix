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
#include "remote_example.h"
#include <unistd.h>

struct activator {
    long svcId;
    struct tst_service testSvc;

    long trackerId1;
    long trackerId2;

    pthread_mutex_t mutex; //protects below
    calculator_service_t *calc;
    remote_example_t *remoteExample;
};

static void bndSetCalc(void* handle, void* svc) {
    struct activator * act = handle;
    pthread_mutex_lock(&act->mutex);
    act->calc = svc;
    pthread_mutex_unlock(&act->mutex);
}

static void bndSetRemoteExample(void* handle, void* svc) {
    struct activator * act = handle;
    pthread_mutex_lock(&act->mutex);
    act->remoteExample = svc;
    pthread_mutex_unlock(&act->mutex);
}

static bool bndIsCalculatorDiscovered(void *handle) {
    struct activator *act = handle;

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

    bool discovered = local != NULL;
    return discovered;
}

static bool bndIsRemoteExampleDiscovered(void *handle) {
    struct activator *act = handle;

    int retries = 40;

    pthread_mutex_lock(&act->mutex);
    remote_example_t *local = act->remoteExample;
    pthread_mutex_unlock(&act->mutex);

    while (local == NULL && retries > 0) {
        printf("Waiting for remote example service .. %d\n", retries);
        usleep(100000);
        --retries;
        pthread_mutex_lock(&act->mutex);
        local = act->remoteExample;
        pthread_mutex_unlock(&act->mutex);
    }

    bool discovered = local != NULL;
    return discovered;
}

static int bndTestCalculator(void *handle) {
    int status = 0;
    struct activator *act = handle;

    double result = -1.0;

    pthread_mutex_lock(&act->mutex);
    int rc = 1;
    if (act->calc != NULL) {
        rc = act->calc->sqrt(act->calc->handle, 4, &result);
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

static int bndTestRemoteExample(void *handle) {
    bool ok = true;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {

        if (ok) {
            //test pow
            double p;
            act->remoteExample->pow(act->remoteExample->handle, 2, 2, &p);
            ok = (p == 4.0);
        }

        if (ok) {
            //test fib
            int32_t f;
            act->remoteExample->fib(act->remoteExample->handle, 4, &f);
            ok = (f == 3);
        }

        //TODO enable and fix segfault
//        if (ok) {
//            //test string call with taking ownership
//            char *tmp = strndup("test1", 1024);
//            char *result = NULL;
//            act->remoteExample->setName1(act->remoteExample->handle, tmp, &result);
//            free(tmp);
//            ok = strncmp(tmp, result, 1024) == 0;
//            free(result);
//        }
//
//        if (ok) {
//            //test string call with keeping ownership
//            const char *tmp = "test2";
//            char *result = NULL;
//            act->remoteExample->setName2(act->remoteExample->handle, tmp, &result);
//            ok = strncmp(tmp, result, 1024) == 0;
//            free(result);
//        }

    } else {
        fprintf(stderr, "remote example service not available");
        ok  = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok ? 0 : 1;
}

static celix_status_t bndStart(struct activator *act, celix_bundle_context_t* ctx) {
    //initialize service struct
    act->testSvc.handle = act;
    act->testSvc.isCalcDiscovered = bndIsCalculatorDiscovered;
    act->testSvc.isRemoteExampleDiscovered = bndIsRemoteExampleDiscovered;
    act->testSvc.testCalculator = bndTestCalculator;
    act->testSvc.testRemoteExample = bndTestRemoteExample;

    //create mutex
    pthread_mutex_init(&act->mutex, NULL);

    //track (remote) service
    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.set = bndSetCalc;
        opts.callbackHandle = act;
        opts.filter.serviceName = CALCULATOR_SERVICE;
        opts.filter.ignoreServiceLanguage = true;
        act->trackerId1 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }
    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.set = bndSetRemoteExample;
        opts.callbackHandle = act;
        opts.filter.serviceName = REMOTE_EXAMPLE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        act->trackerId2 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register test service
    act->svcId = celix_bundleContext_registerService(ctx, &act->testSvc, TST_SERVICE_NAME, NULL);
    return CELIX_SUCCESS;
}

static celix_status_t bndStop(struct activator *act, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, act->svcId);
    celix_bundleContext_stopTracker(ctx, act->trackerId1);
    celix_bundleContext_stopTracker(ctx, act->trackerId2);
    pthread_mutex_destroy(&act->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bndStart, bndStop);