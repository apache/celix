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
#include <time.h>
#include "celix_threads.h"
#include "celix_bundle_context.h"
#include "tst_service.h"
#include "calculator_service.h"
#include "remote_example.h"
#include <unistd.h>
#include "celix_utils.h"
#include "celix_dm_component.h"
#include "dm_dependency_manager.h"
#include "celix_bundle_activator.h"

//note exports double diff variable (time in ms)
#define TIMED_EXPR(expr) \
    double diff; \
    do { \
        struct timespec _begin, _end; \
        clock_gettime(CLOCK_MONOTONIC, &_begin); \
        expr; \
        clock_gettime(CLOCK_MONOTONIC, &_end); \
        diff = celix_difftime(&_begin, &_end) * 1000.0; \
    } while(0)

struct activator {
    celix_bundle_context_t *ctx;
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

static bool bndTestCalculator(void *handle) {
    struct activator *act = handle;

    double result = -1.0;

    pthread_mutex_lock(&act->mutex);
    int rc = 1;
    if (act->calc != NULL) {
        TIMED_EXPR(rc = act->calc->sqrt(act->calc->handle, 4, &result));
        printf("calc result is %f. Call took %f ms\n", result, diff);
    } else {
        printf("calc not ready\n");
    }
    pthread_mutex_unlock(&act->mutex);

    return rc == 0 && result == 2.0;
}

static bool bndTestRemoteString(void *handle) {
    bool ok;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {
        //test string Call with taking ownership

        //test with a large very large string to verify mg_write limits.
        int testLength = 1024 * 1024 * 5; //5mb
        char *buf = NULL;
        size_t bufLen = 0;
        FILE* stream = open_memstream(&buf, &bufLen);
        for (int i =0; i < testLength; i++) {
            fputc('A', stream);
        }
        fputc('\0', stream);
        fclose(stream);
        char *result = NULL;
        TIMED_EXPR(act->remoteExample->setName1(act->remoteExample->handle, buf, &result));
        printf("Call setName1 took %f ms\n", diff);
        //note setName1 should take ownership of tmp, so no free(tmp) needed.
        ok = strncmp("AAAA", result, 4) == 0;
        if (ok) {
            ok = strlen(result) == testLength;
            if (!ok) {
                fprintf(stderr, "result length is not currect. expected %i, but len is %li\n", testLength, strlen(result));
            }
        } else {
            fprintf(stderr, "result does not start with AAAA\n");
        }
        free(result);
    } else {
        fprintf(stderr, "remote example service not available");
        ok = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestRemoteConstString(void *handle) {
    bool ok;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {
        //test pow
        const char *name = "name2";
        char *result = NULL;
        TIMED_EXPR(act->remoteExample->setName2(act->remoteExample->handle, name, &result));
        printf("Call setName2 took %f ms\n", diff);
        ok = strncmp(result, "name2", 1024) == 0;
        free(result);
    } else {
        fprintf(stderr, "remote example service not available");
        ok = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestRemoteNumbers(void *handle) {
    bool ok = true;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {

        if (ok) {
            //test pow
            double p;
            TIMED_EXPR(act->remoteExample->pow(act->remoteExample->handle, 2, 2, &p));
            printf("Call pow took %f ms\n", diff);
            ok = (p == 4.0);
        }

        if (ok) {
            //test fib
            int32_t f;
            TIMED_EXPR(act->remoteExample->fib(act->remoteExample->handle, 4, &f));
            printf("Call fib took %f ms\n", diff);
            ok = (f == 3);
        }
    } else {
        fprintf(stderr, "remote example service not available");
        ok  = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestRemoteEnum(void *handle) {
    bool ok;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {
        enum enum_example e = ENUM_EXAMPLE_VAL2;
        enum enum_example result = ENUM_EXAMPLE_VAL3;
        int rc = act->remoteExample->setEnum(act->remoteExample->handle, e, &result);
        ok = rc == 0 && result == ENUM_EXAMPLE_VAL2;
    } else {
        fprintf(stderr, "remote example service not available");
        ok = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestRemoteAction(void *handle) {
    bool ok;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {
        int rc = act->remoteExample->action(act->remoteExample->handle);
        ok = rc == 0;
    } else {
        fprintf(stderr, "remote example service not available");
        ok = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestRemoteComplex(void *handle) {
    bool ok;
    struct activator *act = handle;

    pthread_mutex_lock(&act->mutex);
    if (act->remoteExample != NULL) {
       struct complex_input_example exmpl;
       exmpl.a = 2;
       exmpl.b = 3;
       exmpl.n = 5;
       exmpl.name = "name";
       exmpl.e = ENUM_EXAMPLE_VAL3;
       struct complex_output_example* result = NULL;
       int rc;
       TIMED_EXPR(rc = act->remoteExample->setComplex(act->remoteExample->handle, &exmpl, &result));
       printf("Call setComplex took %f ms\n", diff);
       ok = rc == 0 && result->pow == 8 && result->fib == 5 && strncmp("name", result->name, 64) == 0;
       if (rc == 0) {
           free(result->name);
           free(result);
       }
    } else {
        fprintf(stderr, "remote example service not available");
        ok = false;
    }
    pthread_mutex_unlock(&act->mutex);

    return ok;
}

static bool bndTestCreateDestroyComponentWithRemoteService(void *handle) {
    struct activator *act = handle;

    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, "service.exported.interfaces", CALCULATOR_SERVICE);

    calculator_service_t calcSvc;
    calcSvc.handle = NULL;
    calcSvc.add = NULL; //note for this test case the actual service methods can be NULL
    calcSvc.sub = NULL; //note for this test case the actual service methods can be NULL
    calcSvc.sqrt = NULL; //note for this test case the actual service methods can be NULL

    celix_dm_component_t *cmp = celix_dmComponent_create(act->ctx, "test");
    celix_dmComponent_addInterface(cmp, CALCULATOR_SERVICE, NULL, &calcSvc, properties);

    celix_dependency_manager_t *dm = celix_bundleContext_getDependencyManager(act->ctx);
    dependencyManager_add(dm, cmp);
    dependencyManager_removeAllComponents(dm); //note should not deadlock
    return true;
}

static bool testCreateRemoteServiceInRemoteCall(void *handle) {
    struct activator *act = handle;
    int rc;
    TIMED_EXPR(rc = act->remoteExample->createAdditionalRemoteService(act->remoteExample->handle));
    printf("Call createAdditionalRemoteService took %f ms\n", diff);
    return rc == 0;
}

static celix_status_t bndStart(struct activator *act, celix_bundle_context_t* ctx) {
    //initialize service struct
    act->ctx = ctx;
    act->testSvc.handle = act;
    act->testSvc.isCalcDiscovered = bndIsCalculatorDiscovered;
    act->testSvc.isRemoteExampleDiscovered = bndIsRemoteExampleDiscovered;
    act->testSvc.testCalculator = bndTestCalculator;
    act->testSvc.testRemoteString = bndTestRemoteString;
    act->testSvc.testRemoteConstString = bndTestRemoteConstString;
    act->testSvc.testRemoteNumbers = bndTestRemoteNumbers;
    act->testSvc.testRemoteEnum = bndTestRemoteEnum;
    act->testSvc.testRemoteAction = bndTestRemoteAction;
    act->testSvc.testRemoteComplex = bndTestRemoteComplex;
    act->testSvc.testCreateRemoteServiceInRemoteCall = testCreateRemoteServiceInRemoteCall;
    act->testSvc.testCreateDestroyComponentWithRemoteService = bndTestCreateDestroyComponentWithRemoteService;

    act->testSvc.testCreateRemoteServiceInRemoteCall = testCreateRemoteServiceInRemoteCall;

    //create mutex
    pthread_mutex_init(&act->mutex, NULL);

    //track (remote) service
    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.set = bndSetCalc;
        opts.callbackHandle = act;
        opts.filter.serviceName = CALCULATOR_SERVICE;
        act->trackerId1 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }
    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.set = bndSetRemoteExample;
        opts.callbackHandle = act;
        opts.filter.serviceName = REMOTE_EXAMPLE_NAME;
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
