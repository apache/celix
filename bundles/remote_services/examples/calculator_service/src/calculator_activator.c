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
/**
 * calculator_activator.c
 *
 *  \date       Oct 5, 2011
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "calculator_impl.h"
#include "remote_constants.h"

struct activator {
    calculator_t *calculator;
    calculator_service_t *service;

    service_registration_t *calculatorReg;
};

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator;

    activator = calloc(1, sizeof(*activator));
    if (!activator) {
        status = CELIX_ENOMEM;
    } else {
        activator->calculatorReg = NULL;

        *userData = activator;
    }

    return status;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    status = calculator_create(&activator->calculator);
    if (status == CELIX_SUCCESS) {
        activator->service = calloc(1, sizeof(*activator->service));
        if (!activator->service) {
            status = CELIX_ENOMEM;
        } else {
            activator->service->calculator = activator->calculator;
            activator->service->add = calculator_add;
            activator->service->sub = calculator_sub;
            activator->service->sqrt = calculator_sqrt;

            celix_properties_t *properties = celix_properties_create();
            celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_INTERFACES, CALCULATOR_SERVICE);
            celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_CONFIGS, CALCULATOR_CONFIGURATION_TYPE);
            bundleContext_registerService(context, CALCULATOR_SERVICE, activator->service, properties, &activator->calculatorReg);
        }
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    serviceRegistration_unregister(activator->calculatorReg);

    free(activator->service);

    calculator_destroy(&activator->calculator);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;

    free(userData);

    return status;
}
