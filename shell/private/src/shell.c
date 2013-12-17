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

struct shellServiceActivator {
	shell_pt shell;
	shell_service_pt shellService;
	service_registration_pt registration;
	service_listener_pt listener;

	service_registration_pt psCommand;
	command_pt psCmd;
	command_service_pt psCmdSrv;

	service_registration_pt startCommand;
	command_pt startCmd;
	command_service_pt startCmdSrv;

	service_registration_pt stopCommand;
	command_pt stopCmd;
	command_service_pt stopCmdSrv;

	service_registration_pt installCommand;
	command_pt installCmd;
	command_service_pt installCmdSrv;

	service_registration_pt uninstallCommand;
    command_pt uninstallCmd;
    command_service_pt uninstallCmdSrv;

	service_registration_pt updateCommand;
	command_pt updateCmd;
	command_service_pt updateCmdSrv;

	service_registration_pt logCommand;
    command_pt logCmd;
    command_service_pt logCmdSrv;

    service_registration_pt inspectCommand;
	command_pt inspectCmd;
	command_service_pt inspectCmdSrv;

	service_registration_pt helpCommand;
	command_pt helpCmd;
	command_service_pt helpCmdSrv;
};

static celix_status_t shell_createCommandService(apr_pool_t *pool, command_pt command, command_service_pt *commandService);

shell_pt shell_create(apr_pool_t *pool) {
	shell_pt shell = (shell_pt) malloc(sizeof(*shell));
	shell->pool = pool;
	shell->commandNameMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	shell->commandReferenceMap = hashMap_create(NULL, NULL, NULL, NULL);
	return shell;
}

void shell_destroy(shell_pt shell) {
	hashMap_destroy(shell->commandNameMap, false, false);
	hashMap_destroy(shell->commandReferenceMap, false, false);
	free(shell);
}

array_list_pt shell_getCommands(shell_pt shell) {
	array_list_pt commands = NULL;
	hash_map_iterator_pt iter = hashMapIterator_create(shell->commandNameMap);

	arrayList_create(&commands);
	while (hashMapIterator_hasNext(iter)) {
		char * name = hashMapIterator_nextKey(iter);
		arrayList_add(commands, name);
	}
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
	return NULL;
}

void shell_executeCommand(shell_pt shell, char * commandLine, void (*out)(char *), void (*error)(char *)) {
	unsigned int pos = strcspn(commandLine, " ");
	char * commandName = (pos != strlen(commandLine)) ? string_ndup((char *)commandLine, pos) : strdup(commandLine);
	command_service_pt command = shell_getCommand(shell, commandName);
	if (command != NULL) {
		command->executeCommand(command->command, commandLine, out, error);
	} else {
	    error("No such command\n");
	}
	free(commandName);
}

command_service_pt shell_getCommand(shell_pt shell, char * commandName) {
	command_service_pt command = hashMap_get(shell->commandNameMap, commandName);
	return (command == NULL) ? NULL : command;
}

void shell_addCommand(shell_pt shell, service_reference_pt reference) {
    command_service_pt command = NULL;
	void *cmd = NULL;
	bundleContext_getService(shell->bundleContext, reference, &cmd);
	command = (command_service_pt) cmd;
	hashMap_put(shell->commandNameMap, command->getName(command->command), command);
	hashMap_put(shell->commandReferenceMap, reference, command);
}

void shell_removeCommand(shell_pt shell, service_reference_pt reference) {
	command_service_pt command = (command_service_pt) hashMap_remove(shell->commandReferenceMap, reference);
	if (command != NULL) {
		hashMap_remove(shell->commandNameMap, command->getName(command->command));
	}
}

void shell_serviceChanged(service_listener_pt listener, service_event_pt event) {
	shell_pt shell = (shell_pt) listener->handle;
	if (event->type == OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED) {
		shell_addCommand(shell, event->reference);
	}
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	apr_pool_t *pool = NULL;
	shell_pt shell = NULL;
	*userData = malloc(sizeof(struct shellServiceActivator));
	bundleContext_getMemoryPool(context, &pool);
	shell = shell_create(pool);
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
	((struct shellServiceActivator *) (*userData))->helpCommand = NULL;
	((struct shellServiceActivator *) (*userData))->registration = NULL;

	//(*userData) = &(*activator);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status;
	apr_pool_t *pool = NULL;

	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	activator->shell->bundleContext = context;

	activator->shellService = (shell_service_pt) malloc(sizeof(*activator->shellService));
	activator->shellService->shell = activator->shell;
	activator->shellService->getCommands = shell_getCommands;
	activator->shellService->getCommandDescription = shell_getCommandDescription;
	activator->shellService->getCommandUsage = shell_getCommandUsage;
	activator->shellService->getCommandReference = shell_getCommandReference;
	activator->shellService->executeCommand = shell_executeCommand;

	status = bundleContext_registerService(context, (char *) OSGI_SHELL_SERVICE_NAME, activator->shellService, NULL, &activator->registration);

	bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
	    service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	    activator->listener = listener;
	    listener->pool = pool;
	    listener->handle = activator->shell;
	    listener->serviceChanged = (void *) shell_serviceChanged;
	    status = bundleContext_addServiceListener(context, listener, "(objectClass=commandService)");

	    if (status == CELIX_SUCCESS) {
	        activator->psCmd = psCommand_create(context);
	        shell_createCommandService(pool, activator->psCmd, &activator->psCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->psCmdSrv, NULL, &activator->psCommand);

	        activator->startCmd = startCommand_create(context);
	        shell_createCommandService(pool, activator->startCmd, &activator->startCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->startCmdSrv, NULL, &activator->startCommand);

	        activator->stopCmd = stopCommand_create(context);
	        shell_createCommandService(pool, activator->stopCmd, &activator->stopCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->stopCmdSrv, NULL, &activator->stopCommand);

	        activator->installCmd = installCommand_create(context);
	        shell_createCommandService(pool, activator->installCmd, &activator->installCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->installCmdSrv, NULL, &activator->installCommand);

	        activator->uninstallCmd = uninstallCommand_create(context);
	        shell_createCommandService(pool, activator->uninstallCmd, &activator->uninstallCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->uninstallCmdSrv, NULL, &activator->uninstallCommand);

	        activator->updateCmd = updateCommand_create(context);
	        shell_createCommandService(pool, activator->updateCmd, &activator->updateCmdSrv);
	        bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->updateCmdSrv, NULL, &activator->updateCommand);

	        activator->logCmd = logCommand_create(context);
	        shell_createCommandService(pool, activator->logCmd, &activator->logCmdSrv);
            bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->logCmdSrv, NULL, &activator->logCommand);

            activator->inspectCmd = inspectCommand_create(context);
            shell_createCommandService(pool, activator->inspectCmd, &activator->inspectCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->inspectCmdSrv, NULL, &activator->inspectCommand);

			activator->helpCmd = helpCommand_create(context);
			shell_createCommandService(pool, activator->helpCmd, &activator->helpCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->helpCmdSrv, NULL, &activator->helpCommand);
	    }
	}

	return status;
}

static celix_status_t shell_createCommandService(apr_pool_t *pool, command_pt command, command_service_pt *commandService) {
	*commandService = apr_palloc(pool, sizeof(**commandService));
	(*commandService)->command = command;
	(*commandService)->executeCommand = command->executeCommand;
	(*commandService)->getName = command_getName;
	(*commandService)->getShortDescription = command_getShortDescription;
	(*commandService)->getUsage = command_getUsage;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
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
	serviceRegistration_unregister(activator->helpCommand);
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
        inspectCommand_destroy(activator->helpCmd);

        free(activator->shellService);
        activator->shellService = NULL;

        free(activator->listener);
        activator->listener = NULL;
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct shellServiceActivator * activator = (struct shellServiceActivator *) userData;
	shell_destroy(activator->shell);
	free(activator);

	return CELIX_SUCCESS;
}
