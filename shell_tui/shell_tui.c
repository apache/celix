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
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "bundle_context.h"
#include "bundle_activator.h"
#include "shell.h"
#include "utils.h"

struct shellTuiActivator {
	BUNDLE_CONTEXT context;
	SHELL_SERVICE shell;
	SERVICE_REFERENCE reference;
	bool running;
	pthread_t runnable;
};

typedef struct shellTuiActivator * SHELL_TUI_ACTIVATOR;

void shellTui_write(char * line) {
	fprintf(stdout, "%s", line);
}

void * shellTui_runnable(void * data) {
	SHELL_TUI_ACTIVATOR act = (SHELL_TUI_ACTIVATOR) data;

	char in[256];
	bool needPrompt = true;
	while (act->running) {
		if (needPrompt) {
			printf("-> ");
			needPrompt = false;
		}
		fgets(in, 256, stdin);
		needPrompt = true;
		char * line = string_trim(strdup(in));
		if (strlen(line) == 0) {
			continue;
		}
		if (act->shell == NULL) {
			continue;
		}
		act->shell->executeCommand(act->shell->shell, line, shellTui_write, shellTui_write);
	}
	pthread_exit(NULL);
}

void shellTui_initializeService(SHELL_TUI_ACTIVATOR activator) {
	if (activator->shell == NULL) {
		activator->reference = bundleContext_getServiceReference(activator->context, (char *) SHELL_SERVICE_NAME);
		if (activator->reference != NULL) {
			activator->shell = (SHELL_SERVICE) bundleContext_getService(activator->context, activator->reference);
		}
	}
}

void shellTui_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	SHELL_TUI_ACTIVATOR act = (SHELL_TUI_ACTIVATOR) listener->handle;
	if ((event->type == REGISTERED) && (act->reference == NULL)) {
		shellTui_initializeService(act);
	} else if ((event->type == UNREGISTERING) && (act->reference == event->reference)) {
		// bundleContext_ungetService(act->reference);
		act->reference = NULL;
		act->shell = NULL;

		shellTui_initializeService(act);
	}
}

void * bundleActivator_create() {
	SHELL_TUI_ACTIVATOR activator = (SHELL_TUI_ACTIVATOR) malloc(sizeof(*activator));
	activator->shell = NULL;
	return activator;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	SHELL_TUI_ACTIVATOR act = (SHELL_TUI_ACTIVATOR) userData;
	act->context = context;
	act->running = true;

	SERVICE_LISTENER listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
	listener->handle = act;
	listener->serviceChanged = (void *) shellTui_serviceChanged;
	addServiceListener(context, listener, "(objectClass=shellService)");

	shellTui_initializeService(act);
	pthread_create(&act->runnable, NULL, shellTui_runnable, act);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	SHELL_TUI_ACTIVATOR act = (SHELL_TUI_ACTIVATOR) userData;
}

void bundleActivator_destroy(void * userData) {

}
