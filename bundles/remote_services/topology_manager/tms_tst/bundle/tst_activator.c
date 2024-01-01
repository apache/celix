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
#include <assert.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>

#include "celix_bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_reference.h"
#include "celix_errno.h"

#include "tst_service.h"
#include "calculator_service.h"

#define IMPORT_SERVICE_NAME "org.apache.celix.test.MyBundle"  // see TmsTest.cpp

struct activator {
    celix_bundle_context_t *context;
    struct tst_service serv;
    service_registration_t *reg;

    service_tracker_customizer_t *cust;
    service_tracker_t *tracker;
    tst_service_t **import;  // MyBundle service pointer
};

static celix_status_t addImport(void * handle, service_reference_pt reference, void * service);
static celix_status_t removeImport(void * handle, service_reference_pt reference, void * service);

static bool IsImported(void *handle);

celix_status_t celix_bundleActivator_create(celix_bundle_context_t *context, void **out) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *act = calloc(1, sizeof(*act));
    if (act != NULL) {
        act->context = context;
        act->serv.handle = act;
        act->serv.IsImported = IsImported;
        act->import = NULL;

        status = serviceTrackerCustomizer_create(act, NULL, addImport, NULL, removeImport, &act->cust);
        status = CELIX_DO_IF(status, serviceTracker_create(context, IMPORT_SERVICE_NAME, act->cust, &act->tracker));

    } else {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        *out = act;
    } else if (act != NULL) {
        if (act->cust != NULL) {
            free(act->cust);
            act->cust = NULL;
        }
        if (act->tracker != NULL) {
            serviceTracker_destroy(act->tracker);
            act->tracker = NULL;
        }
        free(act);
    }

    return CELIX_SUCCESS;
}

static celix_status_t addImport(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator * act = handle;
    act->import = service;
    return status;
}

static celix_status_t removeImport(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator * act = handle;
    if (act->import == service) {
        act->import = NULL;
    }
    return status;

}

celix_status_t celix_bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    celix_status_t status;
    struct activator * act = userData;

    act->reg = NULL;
    status = bundleContext_registerService(context, (char *) TST_SERVICE_NAME, &act->serv, NULL, &act->reg);

    status = CELIX_DO_IF(status, serviceTracker_open(act->tracker));

    return status;
}

celix_status_t celix_bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    celix_status_t status;
    struct activator * act = userData;

    status = serviceRegistration_unregister(act->reg);
    status = CELIX_DO_IF(status, serviceTracker_close(act->tracker));

    return status;
}

celix_status_t celix_bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
    struct activator *act = userData;
    if (act != NULL) {
        if (act->tracker != NULL) {
            serviceTracker_destroy(act->tracker);
            act->tracker = NULL;
        }
        free(act);
    }
    return CELIX_SUCCESS;
}

static bool IsImported(void *handle) {

    tst_service_t *service = (tst_service_t *) handle;
    struct activator *act = (struct activator *) service->handle;

    return (act->import != NULL);
}
