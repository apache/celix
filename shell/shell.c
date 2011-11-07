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
 * shell.c
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "shell_private.h"
#include "bundle_activator.h"
#include "command_private.h"
#include "headers.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "ps_command.h"
#include "start_command.h"
#include "stop_command.h"
#include "install_command.h"
#include "uninstall_command.h"
#include "update_command.h"
#include "log_command.h"
#include "inspect_command.h"

#include "utils.h"

struct shellServiceActivator {
	SHELL shell;
	SHELL_SERVICE shellService;
	SERVICE_REGISTRATION registration;
	SERVICE_LISTENER listener;

	SERVICE_REGISTRATION psCommand;
	COMMAND psCmd;

	SERVICE_REGISTRATION startCommand;
	COMMAND startCmd;

	SERVICE_REGISTRATION stopCommand;
	COMMAND stopCmd;

	SERVICE_REGISTRATION installCommand;
	COMMAND installCmd;

	SERVICE_REGISTRATION uninstallCommand;
    COMMAND uninstallCmd;

	SERVICE_REGISTRATION updateCommand;
	COMMAND updateCmd;

	SERVICE_REGISTRATION logCommand;
    COMMAND logCmd;

    SERVICE_REGISTRATION inspectCommand;
	COMMAND inspectCmd;
};

SHELL shell_create(apr_pool_t *pool) {
	SHELL shell = (SHELL) malloc(sizeof(*shell));
	shell->pool = pool;
	shell->commandNameMap = hashMap_create(string_hash, NULL, string_equals, NULL);
	shell->commandReferenceMap = hashMap_create(NULL, NULL, NULL, NULL);
	return shell;
}

void shell_destroy(SHELL shell) {
	hashMap_destroy(shell->commandNameMap, false, false);
	hashMap_destroy(shell->commandReferenceMap, false, false);
	free(shell);
}

ARRAY_LIST shell_getCommands(SHELL shell) {
	ARRAY_LIST commands = NULL;
	arrayList_create(shell->pool, &commands);
	HASH_MAP_ITERATOR iter = hashMapIterator_create(shell->commandNameMap);
	while (hashMapIterator_hasNext(iter)) {
		char * name = hashMapIterator_nextKey(iter);
		arrayList_add(commands, name);
	}
	return commands;
}

char * shell_getCommandUsage(SHELL shell, char * commandName) {
	COMMAND command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command->usage;
}

char * shell_getCommandDescription(SHELL shell, char * commandName) {
	COMMAND command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command->shortDescription;
}

SERVICE_REFERENCE shell_getCommandReference(SHELL shell, char * command) {
	HASH_MAP_ITERATOR iter = hashMapIterator_create(shell->commandReferenceMap);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		COMMAND cmd = (COMMAND) hashMapEntry_getValue(entry);
		if (strcmp(cmd->name, command) == 0) {
			return (SERVICE_REFERENCE) hashMapEntry_getValue(entry);
		}
	}
	return NULL;
}

void shell_executeCommand(SHELL shell, char * commandLine, void (*out)(char *), void (*error)(char *)) {
	unsigned int pos = strcspn(commandLine, " ");
	char * commandName = (pos != strlen(commandLine)) ? string_ndup((char *)commandLine, pos) : strdup(commandLine);
	COMMAND command = shell_getCommand(shell, commandName);
	if (command != NULL) {
		command->executeCommand(command, commandLine, out, error);
	} else {
	    error("No such command\n");
	}
	free(commandName);
}

COMMAND shell_getCommand(SHELL shell, char * commandName) {
	COMMAND command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command;
}

void shell_addCommand(SHELL shell, SERVICE_REFERENCE reference) {
    void *cmd = NULL;
	bundleContext_getService(shell->bundleContext, reference, &cmd);
	COMMAND command = (COMMAND) cmd;
	hashMap_put(shell->commandNameMap, command->name, command);
	hashMap_put(shell->commandReferenceMap, reference, command);
}

void shell_removeCommand(SHELL shell, SERVICE_REFERENCE reference) {
	COMMAND command = (COMMAND) hashMap_remove(shell->commandReferenceMap, reference);
	if (command != NULL) {
		hashMap_remove(shell->commandNameMap, command->name);
	}
}

void shell_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	SHELL shell = (SHELL) listener->handle;
	if (event->type == REGISTERED) {
		shell_addCommand(shell, event->reference);
	}
}

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	*userData = malloc(sizeof(struct shellServiceActivator));
	apr_pool_t *pool = NULL;
	bundleContext_getMemoryPool(context, &pool);
	SHELL shell = shell_create(pool);
//	struct shellServiceActivator * activator = (struct shellServiceActivator *) (*userData);
	((struct shellServiceActivator *) (*userData))->shell = shell;
	((struct shellServiceActivator *) (*userData))->listener = NULL;
	((struct shellServiceActivator *) (*userData))->psCommand = NULL;
	((struct shellServiceActivator *) (*userData))->startCommand = NULL;
	((struct shellServiceActivator *) (*userData))->stopCommand = NULL;
	((struct shellServiceActivator *) (*userData))->installCommand = NULL;
	((struct shellServiceActivator *) (*userData))->uninstallCommand = NULL;
	((struct shellServiceActivator *) (*userData))->updateCommand = NULL;
	((struct shellServiceActivator *) (*userData))->logCommand = NULL;
	((struct shellServiceActivator *) (*userData))->inspectCommand = NULL;
	((struct shellServiceActivator *) (*userData))->registration = NULL;

	//(*userData) = &(*activator);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status;

	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	activator->shell->bundleContext = context;

	activator->shellService = (SHELL_SERVICE) malloc(sizeof(*activator->shellService));
	activator->shellService->shell = activator->shell;
	activator->shellService->getCommands = shell_getCommands;
	activator->shellService->getCommandDescription = shell_getCommandDescription;
	activator->shellService->getCommandUsage = shell_getCommandUsage;
	activator->shellService->getCommandReference = shell_getCommandReference;
	activator->shellService->executeCommand = shell_executeCommand;

	status = bundleContext_registerService(context, (char *) SHELL_SERVICE_NAME, activator->shellService, NULL, &activator->registration);

	if (status == CELIX_SUCCESS) {
	    SERVICE_LISTENER listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
	    activator->listener = listener;
	    listener->handle = activator->shell;
	    listener->serviceChanged = (void *) shell_serviceChanged;
	    status = bundleContext_addServiceListener(context, listener, "(objectClass=commandService)");

	    if (status == CELIX_SUCCESS) {
	        activator->psCmd = psCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->psCmd, NULL, &activator->psCommand);

	        activator->startCmd = startCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->startCmd, NULL, &activator->startCommand);

	        activator->stopCmd = stopCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->stopCmd, NULL, &activator->stopCommand);

	        activator->installCmd = installCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->installCmd, NULL, &activator->installCommand);

	        activator->uninstallCmd = uninstallCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->uninstallCmd, NULL, &activator->uninstallCommand);

	        activator->updateCmd = updateCommand_create(context);
	        bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->updateCmd, NULL, &activator->updateCommand);

	        activator->logCmd = logCommand_create(context);
            bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->logCmd, NULL, &activator->logCommand);

            activator->inspectCmd = inspectCommand_create(context);
			bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->inspectCmd, NULL, &activator->inspectCommand);
	    }
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	serviceRegistration_unregister(activator->registration);
	serviceRegistration_unregister(activator->psCommand);
	serviceRegistration_unregister(activator->startCommand);
	serviceRegistration_unregister(activator->stopCommand);
	serviceRegistration_unregister(activator->installCommand);
	serviceRegistration_unregister(activator->uninstallCommand);
	serviceRegistration_unregister(activator->updateCommand);
	serviceRegistration_unregister(activator->logCommand);
	serviceRegistration_unregister(activator->inspectCommand);
	status = bundleContext_removeServiceListener(context, activator->listener);

	if (status == CELIX_SUCCESS) {
        psCommand_destroy(activator->psCmd);
        startCommand_destroy(activator->startCmd);
        stopCommand_destroy(activator->stopCmd);
        installCommand_destroy(activator->installCmd);
        uninstallCommand_destroy(activator->uninstallCmd);
        updateCommand_destroy(activator->updateCmd);
        logCommand_destroy(activator->logCmd);
        inspectCommand_destroy(activator->inspectCmd);

        free(activator->shellService);
        activator->shellService = NULL;

        free(activator->listener);
        activator->listener = NULL;
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	shell_destroy(activator->shell);
	free(activator);

	return CELIX_SUCCESS;
}
