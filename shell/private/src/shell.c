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
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "celix_errno.h"

#include "shell_private.h"
#include "bundle_activator.h"
#include "command_impl.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_listener.h"

#include "ps_command.h"
#include "start_command.h"
#include "stop_command.h"
#include "install_command.h"
#include "uninstall_command.h"
#include "update_command.h"
#include "log_command.h"
#include "inspect_command.h"
#include "help_command.h"

#include "utils.h"

static command_service_pt shell_getCommand(shell_pt shell, char * commandName);
array_list_pt shell_getCommands(shell_pt shell);

celix_status_t shell_create(bundle_context_pt context, shell_service_pt* shellService) {
	celix_status_t status = CELIX_ENOMEM;

	shell_service_pt lclService = (shell_service_pt) calloc(1, sizeof(struct shellService));
	shell_pt lclShell = (shell_pt) calloc(1, sizeof(struct shell));

	if (lclService && lclShell) {
		lclShell->bundleContext = context;
		lclShell->commandNameMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		lclShell->commandReferenceMap = hashMap_create(NULL, NULL, NULL, NULL);
		lclService->shell = lclShell;

		lclService->getCommands = shell_getCommands;
		lclService->getCommandDescription = shell_getCommandDescription;
		lclService->getCommandUsage = shell_getCommandUsage;
		lclService->getCommandReference = shell_getCommandReference;
		lclService->executeCommand = shell_executeCommand;

		*shellService = lclService;
		status = CELIX_SUCCESS;
	}

	return status;
}

celix_status_t shell_destroy(shell_service_pt* shellService) {
	celix_status_t status = CELIX_SUCCESS;

	hashMap_destroy((*shellService)->shell->commandNameMap, false, false);
	hashMap_destroy((*shellService)->shell->commandReferenceMap, false, false);

	free((*shellService)->shell);
	free(*shellService);

	return status;
}

celix_status_t shell_addCommand(shell_pt shell, service_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;

	command_service_pt command = NULL;
	void *cmd = NULL;
	bundleContext_getService(shell->bundleContext, reference, &cmd);
	command = (command_service_pt) cmd;
	hashMap_put(shell->commandNameMap, command->getName(command->command), command);
	hashMap_put(shell->commandReferenceMap, reference, command);

	return status;
}

celix_status_t shell_removeCommand(shell_pt shell, service_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;

	command_service_pt command = (command_service_pt) hashMap_remove(shell->commandReferenceMap, reference);
	if (command != NULL) {
		bool result = false;
		hashMap_remove(shell->commandNameMap, command->getName(command->command));
		bundleContext_ungetService(shell->bundleContext, reference, &result);
	}

	return status;
}

array_list_pt shell_getCommands(shell_pt shell) {
	array_list_pt commands = NULL;
	hash_map_iterator_pt iter = hashMapIterator_create(shell->commandNameMap);

	arrayList_create(&commands);
	while (hashMapIterator_hasNext(iter)) {
		char * name = hashMapIterator_nextKey(iter);
		arrayList_add(commands, name);
	}
	hashMapIterator_destroy(iter);
	return commands;
}

char * shell_getCommandUsage(shell_pt shell, char * commandName) {
	command_service_pt command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command->getUsage(command->command);
}

char * shell_getCommandDescription(shell_pt shell, char * commandName) {
	command_service_pt command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command->getShortDescription(command->command);
}

service_reference_pt shell_getCommandReference(shell_pt shell, char * command) {
	hash_map_iterator_pt iter = hashMapIterator_create(shell->commandReferenceMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		command_service_pt cmd = (command_service_pt) hashMapEntry_getValue(entry);
		if (strcmp(cmd->getName(cmd->command), command) == 0) {
			return (service_reference_pt) hashMapEntry_getValue(entry);
		}
	}
	hashMapIterator_destroy(iter);
	return NULL;
}

void shell_executeCommand(shell_pt shell, char * commandLine, void (*out)(char *), void (*error)(char *)) {
	unsigned int pos = strcspn(commandLine, " ");
	char * commandName = (pos != strlen(commandLine)) ? string_ndup((char *) commandLine, pos) : strdup(commandLine);
	command_service_pt command = shell_getCommand(shell, commandName);
	if (command != NULL) {
		command->executeCommand(command->command, commandLine, out, error);
	} else {
		error("No such command\n");
	}
	free(commandName);
}

static command_service_pt shell_getCommand(shell_pt shell, char * commandName) {
	command_service_pt command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command;
}

void shell_serviceChanged(service_listener_pt listener, service_event_pt event) {
	shell_pt shell = (shell_pt) listener->handle;
	if (event->type == OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED) {
		shell_addCommand(shell, event->reference);
	} else if (event->type == OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING) {
		shell_removeCommand(shell, event->reference);
	}
}

