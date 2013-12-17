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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "command_impl.h"

#include "add_command.h"
#include "sub_command.h"
#include "sqrt_command.h"

struct activator {
	service_registration_pt addCommand;
	command_pt addCmd;
	command_service_pt addCmdSrv;

	service_registration_pt subCommand;
	command_pt subCmd;
	command_service_pt subCmdSrv;

	service_registration_pt sqrtCommand;
	command_pt sqrtCmd;
	command_service_pt sqrtCmdSrv;
};

static celix_status_t calculatorShell_createCommandService(apr_pool_t *pool, command_pt command, command_service_pt *commandService);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool = NULL;
	status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct activator));
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

	apr_pool_t *pool;
	struct activator * activator = (struct activator *) userData;

	bundleContext_getMemoryPool(context, &pool);

	activator->addCmd = addCommand_create(context);
	calculatorShell_createCommandService(pool, activator->addCmd, &activator->addCmdSrv);
	bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->addCmdSrv, NULL, &activator->addCommand);

	activator->subCmd = subCommand_create(context);
	calculatorShell_createCommandService(pool, activator->subCmd, &activator->subCmdSrv);
	bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->subCmdSrv, NULL, &activator->subCommand);

	activator->sqrtCmd = sqrtCommand_create(context);
	calculatorShell_createCommandService(pool, activator->sqrtCmd, &activator->sqrtCmdSrv);
	bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, activator->sqrtCmdSrv, NULL, &activator->sqrtCommand);

	return status;
}

static celix_status_t calculatorShell_createCommandService(apr_pool_t *pool, command_pt command, command_service_pt *commandService) {
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
	struct activator * activator = (struct activator *) userData;
	serviceRegistration_unregister(activator->addCommand);
	serviceRegistration_unregister(activator->subCommand);
	serviceRegistration_unregister(activator->sqrtCommand);

	if (status == CELIX_SUCCESS) {
        addCommand_destroy(activator->addCmd);
        subCommand_destroy(activator->subCmd);
        sqrtCommand_destroy(activator->sqrtCmd);
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}

