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

#include "bundle_context.h"
#include "bundle_activator.h"


#include "shell_tui.h"
#include "service_tracker.h"


static celix_status_t activator_addShellService(void *handle, service_reference_pt ref, void *svc) {
    shell_tui_activator_pt act = (shell_tui_activator_pt) handle;
    act->shell = svc;
    return CELIX_SUCCESS;
}

static celix_status_t activator_removeShellService(void *handle, service_reference_pt ref, void *svc) {
    shell_tui_activator_pt act = (shell_tui_activator_pt) handle;
    act->shell = svc;
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	shell_tui_activator_pt activator = (shell_tui_activator_pt) calloc(1, sizeof(*activator));

	if (activator) {
		activator->shell = NULL;
		(*userData) = activator;
	}
	else {
		status = CELIX_ENOMEM;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;

    service_tracker_customizer_pt cust = NULL;
    serviceTrackerCustomizer_create(userData, NULL, activator_addShellService, NULL, activator_removeShellService, &cust);
    serviceTracker_create(context, (char *) OSGI_SHELL_SERVICE_NAME, cust, &act->tracker);
    serviceTracker_open(act->tracker);

    shellTui_start(act);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;

    if (act != NULL) {
        shellTui_stop(act);
        if (act->tracker != NULL) {
            serviceTracker_close(act->tracker);
        }
    }

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	shell_tui_activator_pt activator = (shell_tui_activator_pt) userData;

    if (activator->tracker != NULL) {
        serviceTracker_destroy(activator->tracker);
    }
	free(activator);

	return CELIX_SUCCESS;
}
