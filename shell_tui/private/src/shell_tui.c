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
 * shell_tui.c
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

#include "bundle_context.h"
#include "bundle_activator.h"
#include "shell.h"
#include "utils.h"

struct shellTuiActivator {
	bundle_context_pt context;
	shell_service_pt shell;
	service_reference_pt reference;
	struct serviceListener * listener;
	bool running;
	celix_thread_t runnable;
};

typedef struct shellTuiActivator * shell_tui_activator_pt;

void shellTui_write(char * line) {
	fprintf(stdout, "%s", line);
}

static void* shellTui_runnable(void *data) {
	shell_tui_activator_pt act = (shell_tui_activator_pt) data;

	char in[256];
	char dline[256];
	bool needPrompt = true;

	fd_set rfds;
	struct timeval tv;

	while (act->running) {
		char * line = NULL;
		if (needPrompt) {
			printf("-> ");
			fflush(stdout);
			needPrompt = false;
		}
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (select(1, &rfds, NULL, NULL, &tv)) {
			fgets(in, 256, stdin);
			needPrompt = true;
			memset(dline, 0, 256);
			strncpy(dline, in, 256);

			line = utils_stringTrim(dline);
			if ((strlen(line) == 0) || (act->shell == NULL)) {
				continue;
			}

			act->shell->executeCommand(act->shell->shell, line, shellTui_write, shellTui_write);
		}
	}

	return NULL;
}

void shellTui_initializeService(shell_tui_activator_pt activator) {
	if (activator->shell == NULL) {
		bundleContext_getServiceReference(activator->context, (char *) OSGI_SHELL_SERVICE_NAME, &activator->reference);
		if (activator->reference != NULL) {
			void *shell_svc = NULL;
			bundleContext_getService(activator->context, activator->reference, &shell_svc);
			activator->shell = (shell_service_pt) shell_svc;
		}
	}
}

void shellTui_serviceChanged(service_listener_pt listener, service_event_pt event) {
	bool result = false;
	shell_tui_activator_pt act = (shell_tui_activator_pt) listener->handle;
	bool equals = false;

	serviceReference_equals(act->reference, event->reference, &equals);

	if ((event->type == OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED) && (act->reference == NULL)) {
		shellTui_initializeService(act);
	} else if ((event->type == OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING) && (equals)) {
		bundleContext_ungetService(act->context, act->reference, &result);
		act->reference = NULL;
		act->shell = NULL;

		shellTui_initializeService(act);
	}
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
	celix_status_t status;

	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));

	act->context = context;
	act->running = true;

	act->listener = listener;
	act->listener->handle = act;
	act->listener->serviceChanged = (void *) shellTui_serviceChanged;
	status = bundleContext_addServiceListener(context, act->listener, "(objectClass=shellService)");

	if (status == CELIX_SUCCESS) {
		shellTui_initializeService(act);
		celixThread_create(&act->runnable, NULL, shellTui_runnable, act);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status;
	shell_tui_activator_pt activator = (shell_tui_activator_pt) userData;
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
