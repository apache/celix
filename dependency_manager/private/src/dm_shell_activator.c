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
 * dm_shell_activator.c
 *
 *  \date       16 Oct 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "bundle_context.h"
#include "service_registration.h"
#include "command.h"

#include "dm_shell_list_command.h"

struct bundle_instance {
    service_registration_pt reg;
    command_service_pt  dmCommand;
};

typedef struct bundle_instance * bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {

	if(userData==NULL){
		return CELIX_ILLEGAL_ARGUMENT;
	}

    struct bundle_instance *bi = calloc(sizeof (struct bundle_instance), 1);

    if (bi==NULL) {
        return CELIX_ENOMEM;
    }

    (*userData) = bi;

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt) userData;
    command_service_pt commandService = calloc(1, sizeof(*commandService));

    if (commandService != NULL) {
        commandService->handle = context;
        commandService->executeCommand = (void *)dmListCommand_execute;

        properties_pt props = properties_create();
        properties_set(props, OSGI_SHELL_COMMAND_NAME, "dm");
        properties_set(props, OSGI_SHELL_COMMAND_USAGE, "dm");
        properties_set(props, OSGI_SHELL_COMMAND_DESCRIPTION,
                       "Gives an overview of the component managemend by a dependency manager.");

        bi->dmCommand = commandService;

        status = bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, commandService, props,
                                               &bi->reg);
    } else {
        status = CELIX_ENOMEM;
        free(commandService);
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    bundle_instance_pt bi = (bundle_instance_pt) userData;
    serviceRegistration_unregister(bi->reg);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    bundle_instance_pt bi = (bundle_instance_pt) userData;
    if (bi != NULL) {
        free(bi->dmCommand);
    }
    free(bi);
    return CELIX_SUCCESS;
}

