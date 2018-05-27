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
#include "std_commands.h"
#include "service_tracker.h"
#include "constants.h"

#define NUMBER_OF_COMMANDS 11

struct command {
    celix_status_t (*exec)(void *handle, char *commandLine, FILE *out, FILE *err);
    char *name;
    char *description;
    char *usage;
    command_service_pt service;
    properties_pt props;
    long svcId; //used for service (un)registration
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
                (struct command) {
                        .exec = dmListCommand_execute,
                        .name = "dm",
                        .description = "Gives an overview of the component managemed by a dependency manager.",
                        .usage = "dm [f|full] [<Bundle ID> [<Bundle ID> [...]]]"
                };
        instance_ptr->std_commands[10] =
                (struct command) { NULL, NULL, NULL, NULL, NULL, NULL, -1L }; /*marker for last element*/

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
            properties_set(instance_ptr->std_commands[i].props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);

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
        properties_pt props = properties_create();
        properties_set(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
        status = bundleContext_registerService(context_ptr, (char *) OSGI_SHELL_SERVICE_NAME, instance_ptr->shellService, props, &instance_ptr->registration);
    }

	if (status == CELIX_SUCCESS) {
        service_tracker_customizer_pt cust = NULL;
        serviceTrackerCustomizer_create(instance_ptr->shellService->shell, NULL, (void *)shell_addCommand, NULL, (void *)shell_removeCommand, &cust);
        serviceTracker_create(context_ptr, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, cust, &instance_ptr->tracker);
        serviceTracker_open(instance_ptr->tracker);
    }


    if (status == CELIX_SUCCESS) {
        for (unsigned int i = 0; instance_ptr->std_commands[i].exec != NULL; i++) {
            celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
            opts.svc = instance_ptr->std_commands[i].service;
            opts.serviceName = OSGI_SHELL_COMMAND_SERVICE_NAME;
            opts.serviceVersion = OSGI_SHELL_COMMAND_SERVICE_VERSION;
            opts.properties = instance_ptr->std_commands[i].props;
            instance_ptr->std_commands[i].svcId = celix_bundleContext_registerServiceWithOptions(context_ptr, &opts);
        }
	}

	return status;
}

celix_status_t bundleActivator_stop(void *_ptr, bundle_context_pt context_ptr) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_instance_pt instance_ptr = (bundle_instance_pt) _ptr;

    if (instance_ptr) {
        for (unsigned int i = 0; instance_ptr->std_commands[i].exec != NULL; i++) {
            if (instance_ptr->std_commands[i].svcId >= 0) {
                celix_bundleContext_unregisterService(context_ptr, instance_ptr->std_commands[i].svcId);
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
