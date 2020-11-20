/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include <stdlib.h>
#include <string.h>

#include "shell_private.h"
#include "bundle_activator.h"
#include "std_commands.h"
#include "service_tracker.h"
#include "celix_constants.h"
#include "celix_shell_command.h"

#define NUMBER_OF_COMMANDS 13

struct celix_shell_command_register_entry {
    bool (*exec)(void *handle, const char *commandLine, FILE *out, FILE *err);
    char *name;
    char *description;
    char *usage;
    celix_shell_command_t service;
    celix_properties_t *props;
    long svcId; //used for service (un)registration
};

struct shell_bundle_activator {
    shell_t *shell;
    celix_shell_t shellService;
	long shellSvcId;
	long trackerId;
	long legacyTrackerId;

    struct celix_shell_command_register_entry std_commands[NUMBER_OF_COMMANDS];
};

typedef struct shell_bundle_activator shell_bundle_activator_t;

celix_status_t bundleActivator_create(celix_bundle_context_t* ctx, void **_pptr) {
	celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator = NULL;

    if (!_pptr || !ctx) {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        activator = calloc(1, sizeof(*activator));
        if (!activator) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
        activator->shell = shell_create(ctx);
    }

    if (status == CELIX_SUCCESS) {
        activator->std_commands[0] =
                (struct celix_shell_command_register_entry) {
                        .exec = lbCommand_execute,
                        .name = "celix::lb",
                        .description = "list bundles. Default only the groupless bundles are listed. Use -a to list all bundles." \
                            "\nIf a group string is provided only bundles matching the group string will be listed." \
                            "\nUse -l to print the bundle locations.\nUse -s to print the bundle symbolic names\nUse -u to print the bundle update location.",
                        .usage = "lb [-l | -s | -u | -a] [group]"
                };
        activator->std_commands[1] =
                (struct celix_shell_command_register_entry) {
                        .exec = startCommand_execute,
                        .name = "celix::start",
                        .description = "start bundle(s).",
                        .usage = "start <id> [<id> ...]"
                };
        activator->std_commands[2] =
                (struct celix_shell_command_register_entry) {
                        .exec = stopCommand_execute,
                        .name = "celix::stop",
                        .description = "stop bundle(s).",
                        .usage = "stop <id> [<id> ...]"
                };
        activator->std_commands[3] =
                (struct celix_shell_command_register_entry) {
                        .exec = installCommand_execute,
                        .name = "celix::install",
                        .description = "install bundle(s).",
                        .usage = "install <file> [<file> ...]"
                };
        activator->std_commands[4] =
                (struct celix_shell_command_register_entry) {
                        .exec = uninstallCommand_execute,
                        .name = "celix::uninstall",
                        .description = "uninstall bundle(s).",
                        .usage = "uninstall <file> [<file> ...]"
                };
        activator->std_commands[5] =
                (struct celix_shell_command_register_entry) {
                        .exec = updateCommand_execute,
                        .name = "celix::update",
                        .description = "update bundle(s).",
                        .usage = "update <id> [<URL>]"
                };
        activator->std_commands[6] =
                (struct celix_shell_command_register_entry) {
                        .exec = helpCommand_execute,
                        .name = "celix::help",
                        .description = "display available commands and description.",
                        .usage = "help <command>]"
                };
        activator->std_commands[7] =
                (struct celix_shell_command_register_entry) {
                        .exec = dmListCommand_execute,
                        .name = "celix::dm",
                        .description = "Gives an overview of the component managed by a dependency manager.",
                        .usage = "dm [wtf] [f|full] [<Bundle ID> [<Bundle ID> [...]]]"
                };
        activator->std_commands[8] =
                (struct celix_shell_command_register_entry) {
                    .exec = queryCommand_execute,
                    .name = "celix::query",
                    .description = "Query services. Query for registered and requested services" \
                    "\nIf a query is provided (or multiple), only service with a service name matching the query will be displayed." \
                    "\nIf a query is a (LDAP) filter, filter matching will be used."
                    "\nIf no query is provided all provided and requested services will be listed."
                    "\n\tIf the -v option is provided, also list the service properties." \
                    "\n\tIf the -r option is provided, only query for requested services." \
                    "\n\tIf the -p option is provided, only query for provided services.",
                    .usage = "query [bundleId ...] [-v] [-p] [-r] [query ...]"
                };
        activator->std_commands[9] =
                (struct celix_shell_command_register_entry) {
                        .exec = queryCommand_execute,
                        .name = "celix::q",
                        .description = "Proxy for query command (see 'help query')",
                        .usage = "q [bundleId ...] [-v] [-p] [-r] [query ...]"
                };
        activator->std_commands[10] =
              (struct celix_shell_command_register_entry) {
                      .exec = quitCommand_execute,
                      .name = "celix::quit",
                      .description = "Quit (exit) framework.",
                      .usage = "quit"
              };
        activator->std_commands[11] =
                (struct celix_shell_command_register_entry) {
                        .exec = NULL
                };

        for (unsigned int i = 0; activator->std_commands[i].exec != NULL; ++i) {
            activator->std_commands[i].props = properties_create();
            if (!activator->std_commands[i].props) {
                status = CELIX_BUNDLE_EXCEPTION;
                break;
            }

            celix_properties_set(activator->std_commands[i].props, CELIX_SHELL_COMMAND_NAME, activator->std_commands[i].name);
            celix_properties_set(activator->std_commands[i].props, CELIX_SHELL_COMMAND_USAGE, activator->std_commands[i].usage);
            celix_properties_set(activator->std_commands[i].props, CELIX_SHELL_COMMAND_DESCRIPTION, activator->std_commands[i].description);

            activator->std_commands[i].service.handle = ctx;
            activator->std_commands[i].service.executeCommand = activator->std_commands[i].exec;
        }
    }

    if (status == CELIX_SUCCESS) {
        *_pptr = activator;
    }


    if (status != CELIX_SUCCESS) {
        bundleActivator_destroy(activator, ctx);
    }

	return status;
}

celix_status_t bundleActivator_start(void *activatorData, celix_bundle_context_t* ctx) {
	celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator  = (shell_bundle_activator_t*) activatorData;

    if (!activator || !ctx) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        activator->shellService.handle = activator->shell;
        activator->shellService.executeCommand = (void*)shell_executeCommand;
        activator->shellService.getCommandDescription = (void*)shell_getCommandDescription;
        activator->shellService.getCommandUsage = (void*)shell_getCommandUsage;
        activator->shellService.getCommands = (void*)shell_getCommands;

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_SHELL_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_SERVICE_VERSION;
        opts.svc = &activator->shellService;

        activator->shellSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

	if (status == CELIX_SUCCESS) {
	    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
	    opts.callbackHandle = activator->shell;
	    opts.addWithProperties = (void*) shell_addCommand;
	    opts.removeWithProperties = (void*) shell_removeCommand;
	    opts.filter.ignoreServiceLanguage = true;
	    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
	    activator->trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    activator->legacyTrackerId = -1L;
#ifdef CELIX_INSTALL_DEPRECATED_API
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = activator->shell;
        opts.addWithProperties = (void*) shell_addLegacyCommand;
        opts.removeWithProperties = (void*) shell_removeLegacyCommand;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = OSGI_SHELL_COMMAND_SERVICE_NAME;
        activator->legacyTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }
#endif


    if (status == CELIX_SUCCESS) {
        for (unsigned int i = 0; activator->std_commands[i].exec != NULL; ++i) {
            celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
            opts.svc = &activator->std_commands[i].service;
            opts.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
            opts.serviceVersion = CELIX_SHELL_COMMAND_SERVICE_VERSION;
            opts.properties = activator->std_commands[i].props;
            activator->std_commands[i].svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
        }
	}

	return status;
}

celix_status_t bundleActivator_stop(void *activatorData, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator = activatorData;

    if (activator) {
        for (unsigned int i = 0; activator->std_commands[i].exec != NULL; i++) {
            if (activator->std_commands[i].svcId >= 0) {
                celix_bundleContext_unregisterService(ctx, activator->std_commands[i].svcId);
                activator->std_commands[i].props = NULL;
            }
        }

        celix_bundleContext_unregisterService(ctx, activator->shellSvcId);
        celix_bundleContext_stopTracker(ctx, activator->trackerId);
        celix_bundleContext_stopTracker(ctx, activator->legacyTrackerId);

    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleActivator_destroy(void *activatorData, celix_bundle_context_t* __attribute__((__unused__)) ctx) {
    celix_status_t status = CELIX_SUCCESS;
    shell_bundle_activator_t* activator = activatorData;


    if (activator) {
        shell_destroy(activator->shell);
        free(activator);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}
