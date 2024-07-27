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
#include <sys/socket.h>

#include "celix_log_helper.h"
#include "celix_shell.h"
#include "service_tracker.h"
#include "shell_mediator.h"

static celix_status_t shellMediator_addedService(void* handler, service_reference_pt reference, void* service);
static celix_status_t shellMediator_removedService(void* handler, service_reference_pt reference, void* service);

celix_status_t shellMediator_create(bundle_context_pt context, shell_mediator_pt* instance) {
    celix_status_t status = CELIX_SUCCESS;
    service_tracker_customizer_pt customizer = NULL;

    (*instance) = (shell_mediator_pt)calloc(1, sizeof(**instance));
    if ((*instance) != NULL) {
        (*instance)->context = context;
        (*instance)->tracker = NULL;
        (*instance)->shellService = NULL;

        (*instance)->loghelper = celix_logHelper_create(context, "celix_shell");

        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*instance)->mutex, NULL));

        status = CELIX_DO_IF(
            status,
            serviceTrackerCustomizer_create(
                (*instance), NULL, shellMediator_addedService, NULL, shellMediator_removedService, &customizer));
        status = CELIX_DO_IF(
            status, serviceTracker_create(context, (char*)CELIX_SHELL_SERVICE_NAME, customizer, &(*instance)->tracker));

        if (status == CELIX_SUCCESS) {
            serviceTracker_open((*instance)->tracker);
        }
    } else {
        status = CELIX_ENOMEM;
    }

    if ((status != CELIX_SUCCESS) && ((*instance) != NULL)) {
        celix_logHelper_log(
            (*instance)->loghelper, CELIX_LOG_LEVEL_ERROR, "Error creating shell_mediator, error code is %i\n", status);
    }
    return status;
}

celix_status_t shellMediator_stop(shell_mediator_pt instance) {
    service_tracker_pt tracker;
    celixThreadMutex_lock(&instance->mutex);
    tracker = instance->tracker;
    celixThreadMutex_unlock(&instance->mutex);

    if (tracker != NULL) {
        serviceTracker_close(tracker);
    }

    return CELIX_SUCCESS;
}

celix_status_t shellMediator_destroy(shell_mediator_pt instance) {
    celix_status_t status = CELIX_SUCCESS;

    instance->shellService = NULL;
    serviceTracker_destroy(instance->tracker);
    celix_logHelper_destroy(instance->loghelper);
    celixThreadMutex_destroy(&instance->mutex);

    free(instance);

    return status;
}

celix_status_t shellMediator_executeCommand(shell_mediator_pt instance, char* command, FILE* out, FILE* err) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&instance->mutex);

    if (instance->shellService != NULL) {
        instance->shellService->executeCommand(instance->shellService->handle, command, out, err);
    }

    celixThreadMutex_unlock(&instance->mutex);

    return status;
}

static celix_status_t shellMediator_addedService(void* handler, service_reference_pt reference, void* service) {
    celix_status_t status = CELIX_SUCCESS;
    shell_mediator_pt instance = (shell_mediator_pt)handler;
    celixThreadMutex_lock(&instance->mutex);
    instance->shellService = (celix_shell_t*)service;
    celixThreadMutex_unlock(&instance->mutex);
    return status;
}

static celix_status_t shellMediator_removedService(void* handler, service_reference_pt reference, void* service) {
    celix_status_t status = CELIX_SUCCESS;
    shell_mediator_pt instance = (shell_mediator_pt)handler;
    celixThreadMutex_lock(&instance->mutex);
    instance->shellService = NULL;
    celixThreadMutex_unlock(&instance->mutex);
    return status;
}
