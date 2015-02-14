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
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
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

struct bundle_instance {
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

typedef struct bundle_instance *bundle_instance_pt;

static celix_status_t shell_createCommandService(command_pt command, command_service_pt *commandService) {
	celix_status_t status = CELIX_SUCCESS;

	*commandService = calloc(1, sizeof(**commandService));

	if (!*commandService) {
		status = CELIX_ENOMEM;
	}
	else {
		(*commandService)->command = command;
		(*commandService)->executeCommand = command->executeCommand;
		(*commandService)->getName = command_getName;
		(*commandService)->getShortDescription = command_getShortDescription;
		(*commandService)->getUsage = command_getUsage;
	}

	return status;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_instance_pt bi = (bundle_instance_pt) calloc(1, sizeof(struct bundle_instance));

	if (!bi) {
		status = CELIX_ENOMEM;
	}
	else if (userData == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
		free(bi);
	}
	else {
		bi->listener = NULL;
		bi->psCommand = NULL;
		bi->startCommand = NULL;
		bi->stopCommand = NULL;
		bi->installCommand = NULL;
		bi->uninstallCommand = NULL;
		bi->updateCommand = NULL;
		bi->logCommand = NULL;
		bi->inspectCommand = NULL;
		bi->helpCommand = NULL;
		bi->registration = NULL;

		status = shell_create(context, &bi->shellService);

		if (status != CELIX_SUCCESS) {
			printf("shell_create failed\n");
		}

		(*userData) = bi;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	status = bundleContext_registerService(context, (char *) OSGI_SHELL_SERVICE_NAME, bi->shellService, NULL, &bi->registration);

	if (status == CELIX_SUCCESS) {
		array_list_pt references = NULL;
		service_listener_pt listener = (service_listener_pt) calloc(1, sizeof(*listener));

		bi->listener = listener;
		listener->handle = bi->shellService->shell;
		listener->serviceChanged = (void *) shell_serviceChanged;

		status = bundleContext_addServiceListener(context, listener, "(objectClass=commandService)");
		status = bundleContext_getServiceReferences(context, "commandService", NULL, &references);

		if (status == CELIX_SUCCESS) {
			int i = 0;

			for (i = 0; i < arrayList_size(references); i++) {
				shell_addCommand(bi->shellService->shell, arrayList_get(references, i));
			}
		}

		if (status == CELIX_SUCCESS) {
			bi->psCmd = psCommand_create(context);
			shell_createCommandService(bi->psCmd, &bi->psCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->psCmdSrv, NULL, &bi->psCommand);

			bi->startCmd = startCommand_create(context);
			shell_createCommandService(bi->startCmd, &bi->startCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->startCmdSrv, NULL, &bi->startCommand);

			bi->stopCmd = stopCommand_create(context);
			shell_createCommandService(bi->stopCmd, &bi->stopCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->stopCmdSrv, NULL, &bi->stopCommand);

			bi->installCmd = installCommand_create(context);
			shell_createCommandService(bi->installCmd, &bi->installCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->installCmdSrv, NULL, &bi->installCommand);

			bi->uninstallCmd = uninstallCommand_create(context);
			shell_createCommandService(bi->uninstallCmd, &bi->uninstallCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->uninstallCmdSrv, NULL, &bi->uninstallCommand);

			bi->updateCmd = updateCommand_create(context);
			shell_createCommandService(bi->updateCmd, &bi->updateCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->updateCmdSrv, NULL, &bi->updateCommand);

			bi->logCmd = logCommand_create(context);
			shell_createCommandService(bi->logCmd, &bi->logCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->logCmdSrv, NULL, &bi->logCommand);

			bi->inspectCmd = inspectCommand_create(context);
			shell_createCommandService(bi->inspectCmd, &bi->inspectCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->inspectCmdSrv, NULL, &bi->inspectCommand);

			bi->helpCmd = helpCommand_create(context);
			shell_createCommandService(bi->helpCmd, &bi->helpCmdSrv);
			bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, bi->helpCmdSrv, NULL, &bi->helpCommand);
		}
		arrayList_destroy(references);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	serviceRegistration_unregister(bi->registration);
	serviceRegistration_unregister(bi->psCommand);
	serviceRegistration_unregister(bi->startCommand);
	serviceRegistration_unregister(bi->stopCommand);
	serviceRegistration_unregister(bi->installCommand);
	serviceRegistration_unregister(bi->uninstallCommand);
	serviceRegistration_unregister(bi->updateCommand);
	serviceRegistration_unregister(bi->logCommand);
	serviceRegistration_unregister(bi->inspectCommand);
	serviceRegistration_unregister(bi->helpCommand);

	status = bundleContext_removeServiceListener(context, bi->listener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	psCommand_destroy(bi->psCmd);
	free(bi->psCmdSrv);

	startCommand_destroy(bi->startCmd);
	free(bi->startCmdSrv);

	stopCommand_destroy(bi->stopCmd);
	free(bi->stopCmdSrv);

	installCommand_destroy(bi->installCmd);
	free(bi->installCmdSrv);

	uninstallCommand_destroy(bi->uninstallCmd);
	free(bi->uninstallCmdSrv);

	updateCommand_destroy(bi->updateCmd);
	free(bi->updateCmdSrv);

	logCommand_destroy(bi->logCmd);
	free(bi->logCmdSrv);

	inspectCommand_destroy(bi->inspectCmd);
	free(bi->inspectCmdSrv);

	inspectCommand_destroy(bi->helpCmd);
	free(bi->helpCmdSrv);

	free(bi->listener);
	bi->listener = NULL;

	shell_destroy(&bi->shellService);
	bi->shellService = NULL;
	free(bi);

	return CELIX_SUCCESS;
}
