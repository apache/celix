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
/**
 * shell_private.h
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include "bundle_context.h"
#include "celix_shell.h"
#include "hash_map.h"
#include "celix_shell_command.h"
#include "log_helper.h"

typedef struct celix_shell_command_entry {
    long svcId;
    celix_shell_command_t *svc;
    const celix_properties_t *props;
} celix_shell_command_entry_t;

struct shell {
	celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    celix_thread_mutex_t mutex; //protects below
    hash_map_t *commandServices; //key = char* (command name), value = celix_shell_command_entry_t*
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

#endif /* SHELL_PRIVATE_H_ */
