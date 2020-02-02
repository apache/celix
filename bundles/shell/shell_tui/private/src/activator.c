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
#include <zconf.h>
#include <stdio.h>
#include <unistd.h>

#include "bundle_context.h"
#include "bundle_activator.h"


#include "shell_tui.h"
#include "service_tracker.h"

#define SHELL_USE_ANSI_CONTROL_SEQUENCES "SHELL_USE_ANSI_CONTROL_SEQUENCES"

typedef struct shell_tui_activator {
    shell_tui_t* shellTui;
    long trackerId;
    shell_service_t* currentSvc;
    bool useAnsiControlSequences;
} shell_tui_activator_t;


celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	//Check if tty exists
	if (!isatty(fileno(stdin))) {
        printf("[Shell TUI] no tty connected. Shell TUI will not activate.\n");
        return status;
	}

    shell_tui_activator_t* activator = calloc(1, sizeof(*activator));

	if (activator != NULL) {
        activator->trackerId = -1L;
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

        {
            celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
            opts.filter.serviceName = OSGI_SHELL_SERVICE_NAME;
            opts.callbackHandle = activator->shellTui;
            opts.set = (void*)shellTui_setShell;

            activator->trackerId = celix_bundleContext_trackServicesWithOptions(context, &opts);
        }
	}

    if (activator != NULL && activator->shellTui != NULL) {
        (*userData) = activator;
    } else {
        if (activator != NULL) {
            shellTui_destroy(activator->shellTui);
            celix_bundleContext_stopTracker(context, activator->trackerId);
            activator->trackerId = -1L;
        }
        free(activator);
		status = CELIX_ENOMEM;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    if (act != NULL) {
        shellTui_start(act->shellTui);
    }

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    if (act != NULL) {
        celix_bundleContext_stopTracker(context, act->trackerId);
        shellTui_stop(act->shellTui);
    }

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    shell_tui_activator_t* act = (shell_tui_activator_t*) userData;

    if (act != NULL) {
        shellTui_destroy(act->shellTui);
        free(act);
    }

	return CELIX_SUCCESS;
}
