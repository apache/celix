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
#include "bundle_context.h"
#include "service_registration.h"
#include "service_listener.h"
#include "utils.h"
#include "std_commands.h"
#include "properties.h"

struct command {
    celix_status_t (*exec)(void *handle, char *commandLine, FILE *out, FILE *err);
    char *name;
    char *description;
    char *usage;
    command_service_pt service;
    service_registration_pt reg;
    properties_pt props;
};

static struct command std_commands[] = {
        {psCommand_execute, "ps", "list installed bundles.", "ps [-l | -s | -u]", NULL, NULL, NULL},
        {NULL, NULL, NULL, NULL, NULL, NULL, NULL} /*marker for last element*/ 
};


struct bundle_instance {
	shell_service_pt shellService;
	service_registration_pt registration;
	service_listener_pt listener;

  service_registration_pt commandRegistrations;
  command_service_pt commandServices;
  properties_pt commandProperties;

    /*
    ps
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
    */
};

typedef struct bundle_instance *bundle_instance_pt;

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
		status = shell_create(context, &bi->shellService);

    int i = 0;
    while (std_commands[i].exec != NULL) {
            std_commands[i].props = properties_create();
            if (std_commands[i].props != NULL) {
                    properties_set(std_commands[i].props, "command.name", std_commands[i].name);
                    properties_set(std_commands[i].props, "command.usage", std_commands[i].usage);
                    properties_set(std_commands[i].props, "command.description", std_commands[i].description);

                    std_commands[i].service = calloc(1, sizeof(struct commandService));
                    if (std_commands[i].service != NULL) {
                            std_commands[i].service->handle = context;
                            std_commands[i].service->executeCommand = std_commands[i].exec;
                    } else {
                            status = CELIX_ENOMEM;
                            break;
                    }
            } else {
                    status = CELIX_ENOMEM;
                    break;
            }
              
            i += 1;
    }

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

            int i = 0;
            while (std_commands[i].exec != NULL) {
                    status = bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, std_commands[i].service, std_commands[i].props, &std_commands[i].reg);
                    if (status != CELIX_SUCCESS) {
                            break;
                    }
            }

		}
		arrayList_destroy(references);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	//serviceRegistration_unregister(bi->registration);
            int i = 0;
            while (std_commands[i].exec != NULL) {
                    if (std_commands[i].reg!= NULL) {
                            serviceRegistration_unregister(std_commands[i].reg);
                    }
            }

	status = bundleContext_removeServiceListener(context, bi->listener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	bundle_instance_pt bi = (bundle_instance_pt) userData;

  int i = 0;
  while (std_commands[i].exec != NULL) {
          if (std_commands[i].props != NULL) {
                  properties_destroy(std_commands[i].props);
          } 
          if (std_commands[i].service != NULL) {
                  free(std_commands[i].service);
          }
          i += 1;
  }

	return CELIX_SUCCESS;
}
