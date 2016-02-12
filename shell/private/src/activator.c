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
#include "service_tracker.h"

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
    service_tracker_pt tracker;

    struct command std_commands[NUMBER_OF_COMMANDS];
};

typedef struct bundle_instance *bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context_ptr, void **_pptr) {
	celix_status_t status = CELIX_SUCCESS;

    bundle_instance_pt instance_ptr = NULL;

    if (!_pptr || !context_ptr) {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        instance_ptr = (bundle_instance_pt) calloc(1, sizeof(struct bundle_instance));
        if (!instance_ptr) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
        status = shell_create(context_ptr, &instance_ptr->shellService);
    }

    if (status == CELIX_SUCCESS) {
        instance_ptr->std_commands[0] =
                (struct command) {
                        .exec = psCommand_execute,
                        .name = "lb",
                        .description = "list bundles.",
                        .usage = "lb [-l | -s | -u]"
                };
        instance_ptr->std_commands[1] =
                (struct command) {
                        .exec = startCommand_execute,
                        .name = "start",
                        .description = "start bundle(s).",
                        .usage = "start <id> [<id> ...]"
                };
        instance_ptr->std_commands[2] =
                (struct command) {
                        .exec = stopCommand_execute,
                        .name = "stop",
                        .description = "stop bundle(s).",
                        .usage = "stop <id> [<id> ...]"
                };
        instance_ptr->std_commands[3] =
                (struct command) {
                        .exec = installCommand_execute,
                        .name = "install",
                        .description = "install bundle(s).",
                        .usage = "install <file> [<file> ...]"
                };
        instance_ptr->std_commands[4] =
                (struct command) {
                        .exec = uninstallCommand_execute,
                        .name = "uninstall",
                        .description = "uninstall bundle(s).",
                        .usage = "uninstall <file> [<file> ...]"
                };
        instance_ptr->std_commands[5] =
                (struct command) {
                        .exec = updateCommand_execute,
                        .name = "update",
                        .description = "update bundle(s).",
                        .usage = "update <id> [<URL>]"
                };
        instance_ptr->std_commands[6] =
                (struct command) {
                        .exec = helpCommand_execute,
                        .name = "help",
                        .description = "display available commands and description.",
                        .usage = "help <command>]"
                };
        instance_ptr->std_commands[7] =
                (struct command) {
                        .exec = logCommand_execute,
                        .name = "log",
                        .description = "print log.",
                        .usage = "log"
                };
        instance_ptr->std_commands[8] =
                (struct command) {
                        .exec = inspectCommand_execute,
                        .name = "inspect",
                        .description = "inspect services and components.",
                        .usage = "inspect (service) (capability|requirement) [<id> ...]"
                };
        instance_ptr->std_commands[9] =
                (struct command) { NULL, NULL, NULL, NULL, NULL, NULL, NULL }; /*marker for last element*/

        unsigned int i = 0;
        while (instance_ptr->std_commands[i].exec != NULL) {
            instance_ptr->std_commands[i].props = properties_create();
            if (!instance_ptr->std_commands[i].props) {
                status = CELIX_BUNDLE_EXCEPTION;
                break;
            }

            properties_set(instance_ptr->std_commands[i].props, OSGI_SHELL_COMMAND_NAME, instance_ptr->std_commands[i].name);
            properties_set(instance_ptr->std_commands[i].props, OSGI_SHELL_COMMAND_USAGE, instance_ptr->std_commands[i].usage);
            properties_set(instance_ptr->std_commands[i].props, OSGI_SHELL_COMMAND_DESCRIPTION, instance_ptr->std_commands[i].description);

            instance_ptr->std_commands[i].service = calloc(1, sizeof(*instance_ptr->std_commands[i].service));
            if (!instance_ptr->std_commands[i].service) {
                status = CELIX_ENOMEM;
                break;
            }

            instance_ptr->std_commands[i].service->handle = context_ptr;
            instance_ptr->std_commands[i].service->executeCommand = instance_ptr->std_commands[i].exec;

            i += 1;
        }
    }

    if (status == CELIX_SUCCESS) {
        *_pptr = instance_ptr;
    }


    if (status != CELIX_SUCCESS) {
        bundleActivator_destroy(instance_ptr, context_ptr);
    }

	return status;
}

celix_status_t bundleActivator_start(void *_ptr, bundle_context_pt context_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_instance_pt instance_ptr  = (bundle_instance_pt) _ptr;

    if (!instance_ptr || !context_ptr) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        status = bundleContext_registerService(context_ptr, (char *) OSGI_SHELL_SERVICE_NAME, instance_ptr->shellService, NULL, &instance_ptr->registration);
    }

	if (status == CELIX_SUCCESS) {
        service_tracker_customizer_pt cust = NULL;
        serviceTrackerCustomizer_create(instance_ptr->shellService->shell, NULL, (void *)shell_addCommand, NULL, (void *)shell_removeCommand, &cust);
        serviceTracker_create(context_ptr, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, cust, &instance_ptr->tracker);
        serviceTracker_open(instance_ptr->tracker);
    }


    if (status == CELIX_SUCCESS) {
        for (unsigned int i = 0; instance_ptr->std_commands[i].exec != NULL; i++) {
            status = bundleContext_registerService(context_ptr, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME,
                                                   instance_ptr->std_commands[i].service,
                                                   instance_ptr->std_commands[i].props,
                                                   &instance_ptr->std_commands[i].reg);
            if (status != CELIX_SUCCESS) {
                break;
            }

        }
	}

	return status;
}

celix_status_t bundleActivator_stop(void *_ptr, bundle_context_pt context_ptr) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_instance_pt instance_ptr = (bundle_instance_pt) _ptr;

    if (instance_ptr) {
        for (unsigned int i = 0; instance_ptr->std_commands[i].exec != NULL; i++) {
            if (instance_ptr->std_commands[i].reg != NULL) {
                status = serviceRegistration_unregister(instance_ptr->std_commands[i].reg);
                instance_ptr->std_commands[i].reg = NULL;
                instance_ptr->std_commands[i].props = NULL;
            }
        }

        if (instance_ptr->tracker != NULL) {
            serviceTracker_close(instance_ptr->tracker);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleActivator_destroy(void *_ptr, bundle_context_pt __attribute__((__unused__)) context_ptr) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_instance_pt instance_ptr = (bundle_instance_pt) _ptr;

    if (instance_ptr) {
        serviceRegistration_unregister(instance_ptr->registration);

        for (unsigned int i = 0; instance_ptr->std_commands[i].exec != NULL; i++) {
            free(instance_ptr->std_commands[i].service);
        }

        shell_destroy(&instance_ptr->shellService);

        if (instance_ptr->tracker != NULL) {
            serviceTracker_destroy(instance_ptr->tracker);
        }

        free(instance_ptr);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}
