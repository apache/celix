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
 * calculator_shell_activator.c
 *
 *  \date       Oct 13, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <command.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "add_command.h"
#include "sub_command.h"
#include "sqrt_command.h"

struct activator {
	service_registration_pt addCommand;
	command_service_pt addCmd;
	command_service_pt addCmdSrv;

	service_registration_pt subCommand;
	command_service_pt subCmd;
	command_service_pt subCmdSrv;

	service_registration_pt sqrtCommand;
	command_service_pt sqrtCmd;
	command_service_pt sqrtCmdSrv;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	if (status == CELIX_SUCCESS) {
		*userData = calloc(1, sizeof(struct activator));
		if (!*userData) {
			status = CELIX_ENOMEM;
		} else {
			((struct activator *) (*userData))->addCommand = NULL;
			((struct activator *) (*userData))->subCommand = NULL;
			((struct activator *) (*userData))->sqrtCommand = NULL;

			((struct activator *) (*userData))->addCmd = NULL;
			((struct activator *) (*userData))->subCmd = NULL;
			((struct activator *) (*userData))->sqrtCmd = NULL;

			((struct activator *) (*userData))->addCmdSrv = NULL;
			((struct activator *) (*userData))->subCmdSrv = NULL;
			((struct activator *) (*userData))->sqrtCmdSrv = NULL;
		}
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;

	struct activator * activator = (struct activator *) userData;

	activator->addCmdSrv = calloc(1, sizeof(*activator->addCmdSrv));
	activator->addCmdSrv->handle = context;
	activator->addCmdSrv->executeCommand = (void *)addCommand_execute;
	properties_pt props = properties_create();
	properties_set(props, OSGI_SHELL_COMMAND_NAME, "add");
	bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, activator->addCmdSrv, props, &activator->addCommand);


	activator->sqrtCmdSrv = calloc(1, sizeof(*activator->sqrtCmdSrv));
	activator->sqrtCmdSrv->handle = context;
	activator->sqrtCmdSrv->executeCommand = (void *)sqrtCommand_execute;
	props = properties_create();
	properties_set(props, OSGI_SHELL_COMMAND_NAME, "sqrt");
	bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, activator->sqrtCmdSrv, props, &activator->sqrtCommand);

	activator->subCmdSrv = calloc(1, sizeof(*activator->subCmdSrv));
	activator->subCmdSrv->handle = context;
	activator->subCmdSrv->executeCommand = (void *)subCommand_execute;
	props = properties_create();
	properties_set(props, OSGI_SHELL_COMMAND_NAME, "sub");
	bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, activator->subCmdSrv, props, &activator->subCommand);

	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = (struct activator *) userData;
	serviceRegistration_unregister(activator->addCommand);
	serviceRegistration_unregister(activator->subCommand);
	serviceRegistration_unregister(activator->sqrtCommand);

	free(activator->addCmdSrv);
	free(activator->subCmdSrv);
	free(activator->sqrtCmdSrv);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	free(userData);
	return CELIX_SUCCESS;
}

