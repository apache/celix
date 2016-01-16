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
	celix_status_t status;

	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;
	service_listener_pt listener = (service_listener_pt) calloc(1, sizeof(*listener));

	act->context = context;
	act->running = true;

	act->listener = listener;
	act->listener->handle = act;
	act->listener->serviceChanged = (void *) shellTui_serviceChanged;
	status = bundleContext_addServiceListener(context, act->listener, "(objectClass=shellService)");

	if (status == CELIX_SUCCESS) {
		shellTui_initializeService(act);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status;
	shell_tui_activator_pt activator = (shell_tui_activator_pt) userData;

	bundleContext_ungetService(activator->context,activator->reference,NULL);
	bundleContext_ungetServiceReference(activator->context,activator->reference);

	status = bundleContext_removeServiceListener(context, activator->listener);

	if (status == CELIX_SUCCESS) {
		free(activator->listener);

		activator->running = false;
		activator->listener = NULL;
		activator->context = NULL;
		activator->running = false;

		celixThread_join(activator->runnable, NULL);
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	shell_tui_activator_pt activator = (shell_tui_activator_pt) userData;

	free(activator);

	return CELIX_SUCCESS;
}
