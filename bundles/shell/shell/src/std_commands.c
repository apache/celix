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

#include "std_commands.h"

#include <stdlib.h>
#include <string.h>

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

struct celix_std_commands {
    celix_bundle_context_t* ctx;
    struct celix_shell_command_register_entry std_commands[NUMBER_OF_COMMANDS];
};


celix_std_commands_t* celix_stdCommands_create(celix_bundle_context_t* ctx) {
    celix_std_commands_t* commands = calloc(1, sizeof(*commands));
    commands->ctx = ctx;

    commands->std_commands[0] =
            (struct celix_shell_command_register_entry) {
                    .exec = lbCommand_execute,
                    .name = "celix::lb",
                    .description = "list bundles. Default all bundles are listed." \
                            "\nIf a group string is provided only bundles where the bundle group matching group string will be listed." \
                            "\nUse -l to print the bundle locations.\nUse -s to print the bundle symbolic names\nUse -u to print the bundle update location.",
                    .usage = "lb [-l | -s | -u] [group]"
            };
    commands->std_commands[1] =
            (struct celix_shell_command_register_entry) {
                    .exec = startCommand_execute,
                    .name = "celix::start",
                    .description = "start bundle(s).",
                    .usage = "start <id> [<id> ...]"
            };
    commands->std_commands[2] =
            (struct celix_shell_command_register_entry) {
                    .exec = stopCommand_execute,
                    .name = "celix::stop",
                    .description = "stop bundle(s).",
                    .usage = "stop <id> [<id> ...]"
            };
    commands->std_commands[3] =
            (struct celix_shell_command_register_entry) {
                    .exec = installCommand_execute,
                    .name = "celix::install",
                    .description = "install bundle(s).",
                    .usage = "install <file> [<file> ...]"
            };
    commands->std_commands[4] =
            (struct celix_shell_command_register_entry) {
                    .exec = uninstallCommand_execute,
                    .name = "celix::uninstall",
                    .description = "uninstall bundle(s).",
                    .usage = "uninstall <file> [<file> ...]"
            };
    commands->std_commands[5] =
            (struct celix_shell_command_register_entry) {
                    .exec = updateCommand_execute,
                    .name = "celix::update",
                    .description = "update bundle(s).",
                    .usage = "update <id> [<URL>]"
            };
    commands->std_commands[6] =
            (struct celix_shell_command_register_entry) {
                    .exec = helpCommand_execute,
                    .name = "celix::help",
                    .description = "display available commands or detail info if a command argument is provided",
                    .usage = "help [<command>]"
            };
    commands->std_commands[7] =
            (struct celix_shell_command_register_entry) {
                    .exec = dmListCommand_execute,
                    .name = "celix::dm",
                    .description = "Gives an overview of the component managed by a dependency manager.",
                    .usage = "dm [wtf] [f|full] [<Bundle ID> [<Bundle ID> [...]]]"
            };
    commands->std_commands[8] =
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
    commands->std_commands[9] =
            (struct celix_shell_command_register_entry) {
                    .exec = queryCommand_execute,
                    .name = "celix::q",
                    .description = "Proxy for query command (see 'help query')",
                    .usage = "q [bundleId ...] [-v] [-p] [-r] [query ...]"
            };
    commands->std_commands[10] =
            (struct celix_shell_command_register_entry) {
                    .exec = quitCommand_execute,
                    .name = "celix::quit",
                    .description = "Quit (exit) framework.",
                    .usage = "quit"
            };
    commands->std_commands[11] =
            (struct celix_shell_command_register_entry) {
                    .exec = NULL
            };

    for (unsigned int i = 0; commands->std_commands[i].exec != NULL; ++i) {
        commands->std_commands[i].props = celix_properties_create();
        celix_properties_set(commands->std_commands[i].props, CELIX_SHELL_COMMAND_NAME, commands->std_commands[i].name);
        celix_properties_set(commands->std_commands[i].props, CELIX_SHELL_COMMAND_USAGE, commands->std_commands[i].usage);
        celix_properties_set(commands->std_commands[i].props, CELIX_SHELL_COMMAND_DESCRIPTION, commands->std_commands[i].description);
        commands->std_commands[i].service.handle = ctx;
        commands->std_commands[i].service.executeCommand = commands->std_commands[i].exec;

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &commands->std_commands[i].service;
        opts.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_COMMAND_SERVICE_VERSION;
        opts.properties = commands->std_commands[i].props;
        commands->std_commands[i].svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    return commands;
}

void celix_stdCommands_destroy(celix_std_commands_t* commands) {
    if (commands != NULL) {
        for (unsigned int i = 0; commands->std_commands[i].exec != NULL; i++) {
            if (commands->std_commands[i].svcId >= 0) {
                celix_bundleContext_unregisterService(commands->ctx, commands->std_commands[i].svcId);
                commands->std_commands[i].props = NULL;
            }
        }
    }
    free(commands);
}