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

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include "celix_bundle_context.h"
#include "celix_shell.h"
#include "hash_map.h"
#include "celix_shell_command.h"
#include "celix_log_helper.h"

#ifdef CELIX_INSTALL_DEPRECATED_API
#include "command.h"
#endif

typedef struct celix_shell_command_entry {
    long svcId;
    celix_shell_command_t *svc;
    const celix_properties_t *props;
    char *localName;
    char *namespace;
} celix_shell_command_entry_t;


#ifdef CELIX_INSTALL_DEPRECATED_API
typedef struct celix_legacy_command_entry {
    long svcId;
    command_service_t *svc;
    const celix_properties_t *props;
} celix_legacy_command_entry_t;
#else
//NOTE dummy entry and defines to prevent compile issues when deprecated command service are not used
#define OSGI_SHELL_COMMAND_USAGE "command.usage"
#define OSGI_SHELL_COMMAND_DESCRIPTION "command.description"
struct celix_dummy_shell {
    void *handle;
    celix_status_t (*executeCommand)(void *, char*, FILE*, FILE*);
};
typedef struct celix_legacy_command_entry {
    long svcId;
    struct celix_dummy_shell *svc;
    const celix_properties_t *props;
} celix_legacy_command_entry_t;
#endif

struct shell {
	celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    celix_thread_rwlock_t lock; //protects below
    hash_map_t *commandServices; //key = char* (fully qualified command name), value = celix_shell_command_entry_t*
    hash_map_t *legacyCommandServices; //key = char* (command name), value = celix_legacy_command_entry_t*
};
typedef struct shell shell_t;

shell_t* shell_create(celix_bundle_context_t *ctx);
void shell_destroy(shell_t *shell);
celix_status_t shell_addCommand(shell_t *shell, celix_shell_command_t *svc, const celix_properties_t *props);
celix_status_t shell_removeCommand(shell_t *shell, celix_shell_command_t *svc, const celix_properties_t *props);

celix_status_t shell_executeCommand(shell_t *shell, const char *commandLine, FILE *out, FILE *err);

celix_status_t shell_getCommands(shell_t *shell, celix_array_list_t **commands);
celix_status_t shell_getCommandUsage(shell_t *shell, const char *commandName, char **outUsage);
celix_status_t shell_getCommandDescription(shell_t *shell, const char *commandName, char **outDescription);

#ifdef CELIX_INSTALL_DEPRECATED_API
celix_status_t shell_addLegacyCommand(shell_t *shell, command_service_t *svc, const celix_properties_t *props);
celix_status_t shell_removeLegacyCommand(shell_t *shell, command_service_t *svc, const celix_properties_t *props);
#endif

#endif /* SHELL_PRIVATE_H_ */
