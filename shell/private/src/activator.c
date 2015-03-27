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

#include "shell_private.h"
#include "bundle_activator.h"
#include "std_commands.h"

#define NUMBER_OF_COMMANDS 10

struct command {
    celix_status_t (*exec)(void *handle, char *commandLine, FILE *out, FILE *err);
    char *name;
    char *description;
    char *usage;
    command_service_pt service;
    service_registration_pt reg;
    properties_pt props;
};

struct bundle_instance {
	shell_service_pt shellService;
	service_registration_pt registration;
	service_listener_pt listener;

    struct command std_commands[NUMBER_OF_COMMANDS];
};

typedef struct bundle_instance *bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status;

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

        bi->std_commands[0] = (struct command) {.exec = psCommand_execute, .name = "ps", .description = "list installed bundles.", .usage = "ps [-l | -s | -u]"};
        bi->std_commands[1] = (struct command) {.exec = startCommand_execute, .name = "start", .description = "start bundle(s).", .usage = "start <id> [<id> ...]"};
        bi->std_commands[2] = (struct command) {.exec = stopCommand_execute, .name = "stop", .description = "stop bundle(s).", .usage = "stop <id> [<id> ...]"};
        bi->std_commands[3] = (struct command) {.exec = installCommand_execute, .name = "install", .description = "install bundle(s).", .usage = "install <file> [<file> ...]"};
        bi->std_commands[4] = (struct command) {.exec = uninstallCommand_execute, .name = "uninstall", .description = "uninstall bundle(s).", .usage = "uninstall <file> [<file> ...]"};
        bi->std_commands[5] = (struct command) {.exec = updateCommand_execute, .name = "update", .description = "update bundle(s).", .usage = "update <id> [<URL>]"};
        bi->std_commands[6] = (struct command) {.exec = helpCommand_execute, .name = "help", .description = "display available commands and description.", .usage = "help <command>]"};
        bi->std_commands[7] = (struct command) {.exec = logCommand_execute, .name = "log", .description = "print log.", .usage = "log"};
        bi->std_commands[8] = (struct command) {.exec = inspectCommand_execute, .name = "inspect", .description = "inspect services and components.", .usage = "inspect (service) (capability|requirement) [<id> ...]"};
        bi->std_commands[9] = (struct command) {NULL, NULL, NULL, NULL, NULL, NULL, NULL}; /*marker for last element*/

        int i = 0;
        while (bi->std_commands[i].exec != NULL) {
            bi->std_commands[i].props = properties_create();
            if (bi->std_commands[i].props != NULL) {
                    properties_set(bi->std_commands[i].props, "command.name", bi->std_commands[i].name);
                    properties_set(bi->std_commands[i].props, "command.usage", bi->std_commands[i].usage);
                    properties_set(bi->std_commands[i].props, "command.description", bi->std_commands[i].description);

                    bi->std_commands[i].service = calloc(1, sizeof(struct commandService));
                    if (bi->std_commands[i].service != NULL) {
                        bi->std_commands[i].service->handle = context;
                        bi->std_commands[i].service->executeCommand = bi->std_commands[i].exec;
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
	celix_status_t status;
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
            while (bi->std_commands[i].exec != NULL) {
                    status = bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, bi->std_commands[i].service, bi->std_commands[i].props, &bi->std_commands[i].reg);
                    if (status != CELIX_SUCCESS) {
                            break;
                    }
                    i += 1;
            }

		}
		arrayList_destroy(references);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

            int i = 0;
            while (bi->std_commands[i].exec != NULL) {
                    if (bi->std_commands[i].reg!= NULL) {
                            serviceRegistration_unregister(bi->std_commands[i].reg);
                        bi->std_commands[i].reg = NULL;
                        bi->std_commands[i].props = NULL;
                    }
                    i += 1;
            }

	status = bundleContext_removeServiceListener(context, bi->listener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	bundle_instance_pt bi = (bundle_instance_pt) userData;

  int i = 0;
  while (bi->std_commands[i].exec != NULL) {
          if (bi->std_commands[i].props != NULL) {
                  properties_destroy(bi->std_commands[i].props);
          } 
          if (bi->std_commands[i].service != NULL) {
                  free(bi->std_commands[i].service);
          }
          i += 1;
  }

    free(bi);

	return CELIX_SUCCESS;
}
