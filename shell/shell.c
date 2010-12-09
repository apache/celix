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
#include "hashtable_itr.h"
#include "service_registration.h"

#include "ps_command.h"
#include "start_command.h"
#include "stop_command.h"

#include "utils.h"

struct shellServiceActivator {
	SHELL shell;
	SERVICE_REGISTRATION registration;
};

SHELL shell_create() {
	SHELL shell = (SHELL) malloc(sizeof(*shell));
	shell->commandNameMap = hashMap_create(string_hash, NULL, string_equals, NULL);
	shell->commandReferenceMap = hashMap_create(NULL, NULL, NULL, NULL);
	return shell;
}

ARRAY_LIST shell_getCommands(SHELL shell) {
	ARRAY_LIST commands = arrayList_create();
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
	char * commandName = (pos != strlen(commandLine)) ? string_ndup((char *)commandLine, pos) : commandLine;
	COMMAND command = shell_getCommand(shell, commandName);
	if (command != NULL) {
		command->executeCommand(command, commandLine, out, error);
	}
}

COMMAND shell_getCommand(SHELL shell, char * commandName) {
	COMMAND command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command;
}

void shell_addCommand(SHELL shell, SERVICE_REFERENCE reference) {
	void * cmd = bundleContext_getService(shell->bundleContext, reference);
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

void * bundleActivator_create() {
	struct shellServiceActivator * activator = malloc(sizeof(*activator));
	SHELL shell = shell_create();
	activator->shell = shell;
	return activator;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	activator->shell->bundleContext = context;

	SHELL_SERVICE shellService = (SHELL_SERVICE) malloc(sizeof(*shellService));
	shellService->shell = activator->shell;
	shellService->getCommands = shell_getCommands;
	shellService->getCommandDescription = shell_getCommandDescription;
	shellService->getCommandUsage = shell_getCommandUsage;
	shellService->getCommandReference = shell_getCommandReference;
	shellService->executeCommand = shell_executeCommand;

	activator->registration = bundleContext_registerService(context, (char *) SHELL_SERVICE_NAME, shellService, NULL);

	SERVICE_LISTENER listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
	listener->handle = activator->shell;
	listener->serviceChanged = (void *) shell_serviceChanged;
	addServiceListener(context, listener, "(objectClass=commandService)");

	COMMAND psCommand = psCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, psCommand, NULL);

	COMMAND startCommand = startCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, startCommand, NULL);

	COMMAND stopCommand = stopCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, stopCommand, NULL);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	serviceRegistration_unregister(activator->registration);
}

void bundleActivator_destroy(void * userData) {

}
