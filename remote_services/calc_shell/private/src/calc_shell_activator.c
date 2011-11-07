/*
 * calc_shell_activator.c
 *
 *  Created on: Oct 13, 2011
 *      Author: alexander
 */
#include <stdlib.h>
#include <string.h>

#include "bundle_context.h"
#include "service_registration.h"

#include "command_private.h"

#include "add_command.h"
#include "sub_command.h"
#include "sqrt_command.h"

struct activator {
	SERVICE_REGISTRATION addCommand;
	COMMAND addCmd;

	SERVICE_REGISTRATION subCommand;
	COMMAND subCmd;

	SERVICE_REGISTRATION sqrtCommand;
	COMMAND sqrtCmd;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
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
		}
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;

	struct activator * activator = (struct activator *) userData;

	activator->addCmd = addCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->addCmd, NULL, &activator->addCommand);

	activator->subCmd = subCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->subCmd, NULL, &activator->subCommand);

	activator->sqrtCmd = sqrtCommand_create(context);
	bundleContext_registerService(context, (char *) COMMAND_SERVICE_NAME, activator->sqrtCmd, NULL, &activator->sqrtCommand);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
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

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

