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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <constants.h>

#include "example_calc.h"
#include "bundle_activator.h"

typedef struct activator_data {
    celix_bundle_context_t *ctx;
    long trkId;
} activator_data_t;


static void addSvc(activator_data_t *data __attribute__((unused)), example_calc_t *calc) {
    int result = calc->calc(calc->handle, 2);
    printf("Added calc service, result is %i\n", result);
}

static void removeSvc(activator_data_t *data __attribute__((unused)), example_calc_t *calc) {
    int result = calc->calc(calc->handle, 3);
    printf("Removing calc service, result is %i\n", result);
}

static void useCalc(activator_data_t *data __attribute__((unused)), example_calc_t *calc) {
    int result = calc->calc(calc->handle, 2);
    printf("Called highest ranking service. Result is %i\n", result);
}

celix_status_t bundleActivator_create(celix_bundle_context_t *ctx, void **out) {
	celix_status_t status = CELIX_SUCCESS;
    activator_data_t *data = calloc(1, sizeof(*data));
    if (data != NULL) {
       data->ctx = ctx;
       data->trkId = -1L;
       *out = data;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;

    printf("Starting service tracker\n");
    data->trkId = celix_bundleContext_trackServices(data->ctx, EXAMPLE_CALC_NAME, data, (void*)addSvc, (void*)removeSvc);

    printf("Trying to use calc service\n");
    celix_bundleContext_useService(data->ctx, EXAMPLE_CALC_NAME, data, (void*)useCalc);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    celix_bundleContext_stopTracker(data->ctx, data->trkId);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    free(data);
	return CELIX_SUCCESS;
}
