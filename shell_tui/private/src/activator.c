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
 *  \date       Jan 15, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "bundle_context.h"
#include "bundle_activator.h"


#include "shell_tui.h"
#include "service_tracker.h"

#define SHELL_USE_ANSI_CONTROL_SEQUENCES "SHELL_USE_ANSI_CONTROL_SEQUENCES"

typedef struct shell_tui_activator {
    shell_tui_t* shellTui;
    service_tracker_pt tracker;
    shell_service_t* currentSvc;
    bool useAnsiControlSequences;
} shell_tui_activator_t;


static celix_status_t activator_addShellService(void *handle, service_reference_pt ref, void *svc) {
    shell_tui_activator_t* act = (shell_tui_activator_t*) handle;
    act->currentSvc = svc;
    shellTui_setShell(act->shellTui, svc);
    return CELIX_SUCCESS;
}

static celix_status_t activator_removeShellService(void *handle, service_reference_pt ref, void *svc) {
    shell_tui_activator_t* act = (shell_tui_activator_t*) handle;
    if (act->currentSvc == svc) {
        act->currentSvc = NULL;
        shellTui_setShell(act->shellTui, NULL);
    }
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

    shell_tui_activator_t* activator = calloc(1, sizeof(*activator));

	if (activator != NULL) {
        bool useCommands;
        const char* config = NULL;
        bundleContext_getProperty(context, SHELL_USE_ANSI_CONTROL_SEQUENCES, &config);
        if (config != NULL) {
            useCommands = strncmp("true", config, 5) == 0;
        } else {
            char *term = getenv("TERM");
            useCommands = term != NULL;
        }

        activator->shellTui = shellTui_create(useCommands);

        service_tracker_customizer_t* cust = NULL;
        serviceTrackerCustomizer_create(activator, NULL, activator_addShellService, NULL, activator_removeShellService, &cust);
        serviceTracker_create(context, OSGI_SHELL_SERVICE_NAME, cust, &activator->tracker);
	}

    if (activator != NULL && activator->shellTui != NULL) {
        (*userData) = activator;
    } else {
        if (activator != NULL) {
            shellTui_destroy(activator->shellTui);
            if (activator->tracker != NULL) {
                serviceTracker_destroy(activator->tracker);
            }
        }
        free(activator);
		status = CELIX_ENOMEM;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    act->currentSvc = NULL;
    serviceTracker_open(act->tracker);
    shellTui_start(act->shellTui);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    if (act != NULL) {
        serviceTracker_close(act->tracker);
        shellTui_stop(act->shellTui);
    }

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    shellTui_destroy(act->shellTui);
    serviceTracker_destroy(act->tracker);
	free(act);

	return CELIX_SUCCESS;
}
