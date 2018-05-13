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

#include "example_calc.h"
#include "bundle_activator.h"
#include "constants.h"

typedef struct activator_data {
    long svcIds[100];
    example_calc_t svc;
    celix_bundle_context_t *ctx;
    pthread_t thread;

    pthread_mutex_t mutex; //protects running
    bool running;
} activator_data_t;


static int calc(void *handle __attribute__((unused)), int input) {
    return 42 * input;
}

static bool isRunning(activator_data_t *data) {
    bool result = false;
    pthread_mutex_lock(&data->mutex);
    result = data->running;
    pthread_mutex_unlock(&data->mutex);
    return result;
}

static void setRunning(activator_data_t *data, bool val) {
    pthread_mutex_lock(&data->mutex);
    data->running = val;
    pthread_mutex_unlock(&data->mutex);
}

void * run(void *handle) {
    activator_data_t *data = handle;

    printf("starting service register thread\n");

    int i = 0;
    bool up = true;
    while (isRunning(data)) {
        if (up) {
            properties_t *props = properties_create();
            celix_properties_setLong(props, OSGI_FRAMEWORK_SERVICE_RANKING, rand());
            data->svcIds[i++] = celix_bundleContext_registerService(data->ctx, &data->svc, EXAMPLE_CALC_NAME, props);
        } else { //down
            celix_bundleContext_unregisterService(data->ctx, data->svcIds[i]);
            data->svcIds[--i] = -1L;
        }
        if (i == 99) {
            up = false;
        } else if (i == 0) {
            up = true;
        }
        usleep(100000);
    }

    for (int i = 0; i < 100; ++i) {
        long id = data->svcIds[i];
        celix_bundleContext_unregisterService(data->ctx, id);
    }

    printf("exiting service register thread\n");

    pthread_exit(NULL);
    return NULL;
}

celix_status_t bundleActivator_create(celix_bundle_context_t *ctx, void **out) {
	celix_status_t status = CELIX_SUCCESS;
    activator_data_t *data = calloc(1, sizeof(*data));
    if (data != NULL) {
       data->svc.handle = data;
       data->svc.calc = calc;
       data->ctx = ctx;
       data->running = true;
       pthread_mutex_init(&data->mutex, NULL);

       for (int i = 0; i < 100; ++i) {
           data->svcIds[i] = -1L;
       }

       *out = data;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    pthread_create(&data->thread, NULL, run ,data);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    setRunning(data, false);
    pthread_join(data->thread, NULL);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * handle, celix_bundle_context_t *ctx) {
    activator_data_t *data = handle;
    if (data != NULL) {
        pthread_mutex_destroy(&data->mutex);
        free(data);
    }
	return CELIX_SUCCESS;
}
