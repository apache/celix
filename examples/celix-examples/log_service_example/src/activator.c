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
/*
 * activator.c
 *
 *  \date       Sep 11, 2017
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "bundle_activator.h"
#include "log_helper.h"

struct userData {
	pthread_t logger_thread;
	bool running;
	log_helper_pt log_helper;
};

static void *loggerThread(void *userData);

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
    *userData = calloc(1, sizeof(struct userData));
    if (*userData != NULL) {
        struct userData * data = (struct userData *) *userData;
        status = logHelper_create(context, &data->log_helper);
	} else {
		status = CELIX_START_ERROR;
	}
	return status;
}


celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
	struct userData * data = (struct userData *) userData;
	printf("Started log example\n");
    logHelper_start(data->log_helper);
	data->running = true;
    pthread_create(&data->logger_thread, NULL, loggerThread, data);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
	struct userData * data = (struct userData *) userData;
	printf("Stopping logger example\n");
	data->running = false;
	pthread_join(data->logger_thread, NULL);
    logHelper_stop(data->log_helper);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
    struct userData * data = (struct userData *) userData;
    logHelper_destroy(&data->log_helper);
    free(userData);
	return CELIX_SUCCESS;
}

static void *loggerThread(void *userData) {
    struct userData * data = (struct userData *) userData;
    while (data->running) {
        logHelper_log(data->log_helper, OSGI_LOGSERVICE_INFO, "My log message");
        sleep(1);
    }
    return NULL;
}
