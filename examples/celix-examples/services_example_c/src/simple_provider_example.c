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
#include <stdlib.h>

#include "example_calc.h"
#include <celix_api.h>

typedef struct activator_data {
    example_calc_t svc;
    int seed;
    long svcId;
} activator_data_t;


static int calc(activator_data_t *data, int input) {
    return data->seed * input;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->svc.handle = data;
    data->svc.calc = (void*)calc;
    data->seed = 42;
    data->svcId = -1L;

    data->svcId = celix_bundleContext_registerService(ctx, &data->svc, EXAMPLE_CALC_NAME, NULL);
    printf("Registered calc service with service id %li\n", data->svcId);

    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, data->svcId);
    printf("Unregistered calc service with service id %li\n", data->svcId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)