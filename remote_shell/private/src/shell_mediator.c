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
 * shell_mediator.c
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <utils.h>
#include <shell.h>
#include <service_tracker.h>
#include <command.h>

#include "shell_mediator.h"

struct shell_mediator {

	bundle_context_pt context;
	service_tracker_pt tracker;
	celix_thread_mutex_t mutex;

	//protected by mutex
	shell_service_pt shellService;
};

//NOTE: multiple instances of shell_mediator are not supported, because we need
// 		a non ADT - shared between instances - variable.
static int currentOutputSocket = NULL;

static void shellMediator_writeOnCurrentSocket(char *buff);

static celix_status_t shellMediator_addingService(void *handler, service_reference_pt reference, void **service);
static celix_status_t shellMediator_addedService(void *handler, service_reference_pt reference, void * service);
static celix_status_t shellMediator_modifiedService(void *handler, service_reference_pt reference, void * service);
static celix_status_t shellMediator_removedService(void *handler, service_reference_pt reference, void * service);

celix_status_t shellMediator_create(bundle_context_pt context, shell_mediator_pt *instance) {
	celix_status_t status = CELIX_SUCCESS;
	service_tracker_customizer_pt customizer = NULL;

	(*instance) = (shell_mediator_pt) calloc(1, sizeof(**instance));
    if ((*instance) != NULL) {

		(*instance)->context = context;
		(*instance)->tracker = NULL;
		(*instance)->shellService = NULL;

		status = celixThreadMutex_create(&(*instance)->mutex, NULL);

		status = CELIX_DO_IF(status, serviceTrackerCustomizer_create((*instance), shellMediator_addingService, shellMediator_addedService,
				shellMediator_modifiedService, shellMediator_removedService, &customizer));
		status = CELIX_DO_IF(status, serviceTracker_create(context, (char *)OSGI_SHELL_SERVICE_NAME, customizer, &(*instance)->tracker));

		if (status == CELIX_SUCCESS) {
			serviceTracker_open((*instance)->tracker);
		}
	} else {
		status = CELIX_ENOMEM;
	}

    if (status != CELIX_SUCCESS) {
    	printf("Error creating shell_mediator, error code is %i\n", status);
    }
	return status;
}


celix_status_t shellMediator_destroy(shell_mediator_pt instance) {
    celixThreadMutex_lock(&instance->mutex);

    instance->shellService=NULL;
    serviceTracker_close(instance->tracker);
    celixThreadMutex_unlock(&instance->mutex);

    return CELIX_SUCCESS;
}

static void shellMediator_writeOnCurrentSocket(char *buff) {

	size_t len = strlen(buff);
	send(currentOutputSocket, buff, len, 0);
}

celix_status_t shellMediator_executeCommand(shell_mediator_pt instance, char *command, int socket) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&instance->mutex);

	if (instance->shellService != NULL) {
		currentOutputSocket=socket;
		instance->shellService->executeCommand(instance->shellService->shell, command, shellMediator_writeOnCurrentSocket, shellMediator_writeOnCurrentSocket);
		currentOutputSocket=-1;
	}

	celixThreadMutex_unlock(&instance->mutex);

	return status;
}

static celix_status_t shellMediator_addingService(void *handler, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	shell_mediator_pt instance = (shell_mediator_pt) handler;
	bundleContext_getService(instance->context, reference, service);
	return status;
}

static celix_status_t shellMediator_addedService(void *handler, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	shell_mediator_pt instance = (shell_mediator_pt) handler;
	celixThreadMutex_lock(&instance->mutex);
	instance->shellService = (shell_service_pt) service;
	celixThreadMutex_unlock(&instance->mutex);
	return status;
}

static celix_status_t shellMediator_modifiedService(void *handler, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	//ignore
	return status;
}

static celix_status_t shellMediator_removedService(void *handler, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	shell_mediator_pt instance = (shell_mediator_pt) handler;
	celixThreadMutex_lock(&instance->mutex);
	instance->shellService = NULL;
	celixThreadMutex_unlock(&instance->mutex);
	return status;
}

