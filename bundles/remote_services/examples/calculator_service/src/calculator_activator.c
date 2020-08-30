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

#include <celix_api.h>

#include "calculator_impl.h"
#include "remote_constants.h"

struct activator {
    calculator_t *calculator;
    calculator_service_t service;
    long svcId;
};

celix_status_t calculatorBndStart(struct activator *act, celix_bundle_context_t *ctx) {
    act->svcId = -1L;
    act->calculator = calculator_create();
    if (act->calculator != NULL) {
        act->service.handle = act->calculator;
        act->service.add = (void*)calculator_add;
        act->service.sub = (void*)calculator_sub;
        act->service.sqrt = (void*)calculator_sqrt;

        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_INTERFACES, CALCULATOR_SERVICE);
        celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_CONFIGS, CALCULATOR_CONFIGURATION_TYPE);

        act->svcId = celix_bundleContext_registerService(ctx, &act->service, CALCULATOR_SERVICE, properties);
    }
    return CELIX_SUCCESS;
}

celix_status_t calculatorBndStop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->svcId);
    if (act->calculator != NULL) {
        calculator_destroy(act->calculator);
    }
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, calculatorBndStart, calculatorBndStop);
