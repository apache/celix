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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	apr_thread_t *runnable;
};

typedef struct shellTuiActivator * shell_tui_activator_pt;

void shellTui_write(char * line) {
	fprintf(stdout, "%s", line);
}

static void *APR_THREAD_FUNC shellTui_runnable(apr_thread_t *thd, void *data) {
	shell_tui_activator_pt act = (shell_tui_activator_pt) data;

	char in[256];
	bool needPrompt = true;
	while (act->running) {
		char * dline = NULL;
		char * line = NULL;
		if (needPrompt) {
			printf("-> ");
			needPrompt = false;
		}
		fgets(in, 256, stdin);
		needPrompt = true;
		dline = strdup(in);
		line = utils_stringTrim(dline);
		if (strlen(line) == 0) {
			continue;
		}
		if (act->shell == NULL) {
			continue;
		}
		act->shell->executeCommand(act->shell->shell, line, shellTui_write, shellTui_write);
		free(dline);
	}
	apr_thread_exit(thd, APR_SUCCESS);
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

	if ((event->type == OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED) && (act->reference == NULL)) {
		shellTui_initializeService(act);
	} else if ((event->type == OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING) && (act->reference == event->reference)) {
		bundleContext_ungetService(act->context, act->reference, &result);
		act->reference = NULL;
		act->shell = NULL;

		shellTui_initializeService(act);
	}
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	apr_pool_t *pool = NULL;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	shell_tui_activator_pt activator = (shell_tui_activator_pt) apr_palloc(pool, sizeof(*activator));
	//shell_tui_activator_pt activator = (shell_tui_activator_pt) malloc(sizeof(*activator));
	activator->shell = NULL;
	(*userData) = activator;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status;
	apr_pool_t *pool = NULL;

	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));

	act->context = context;
	act->running = true;

	
	bundleContext_getMemoryPool(context, &pool);
	act->listener = listener;
	act->listener->pool = pool;
	act->listener->handle = act;
	act->listener->serviceChanged = (void *) shellTui_serviceChanged;
	status = bundleContext_addServiceListener(context, act->listener, "(objectClass=shellService)");

	if (status == CELIX_SUCCESS) {
        shellTui_initializeService(act);
		apr_thread_create(&act->runnable, NULL, shellTui_runnable, act, pool);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status;
	shell_tui_activator_pt act = (shell_tui_activator_pt) userData;
	status = bundleContext_removeServiceListener(context, act->listener);

	if (status == CELIX_SUCCESS) {
		apr_status_t join;
        free(act->listener);
        act->listener = NULL;
        act->context = NULL;
        act->running = false;
        apr_thread_join(&join, act->runnable);
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}
