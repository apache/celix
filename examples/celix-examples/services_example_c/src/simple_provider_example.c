/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <stdio.h>
#include <stdlib.h>

#include "example_calc.h"
#include "bundle_activator.h"
#include "constants.h"

typedef struct activator_data {
    celix_bundle_context_t *ctx;
    example_calc_t svc;
    int seed;
    long svcId;
} activator_data_t;


static int calc(activator_data_t *data, int input) {
    return data->seed * input;
}

celix_status_t bundleActivator_create(celix_bundle_context_t *ctx, void **out) {
	celix_status_t status = CELIX_SUCCESS;
    activator_data_t *data = calloc(1, sizeof(*data));
    if (data != NULL) {
       data->svc.handle = data;
       data->svc.calc = (void*)calc;
       data->ctx = ctx;
       data->seed = 42;
       data->svcId = -1L;
       *out = data;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    data->svcId = celix_bundleContext_registerService(data->ctx, &data->svc, EXAMPLE_CALC_NAME, NULL);
    printf("Registered calc service with service id %li\n", data->svcId);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    celix_bundleContext_unregisterService(data->ctx, data->svcId);
    printf("Unregistered calc service with service id %li\n", data->svcId);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    free(data);
	return CELIX_SUCCESS;
}
