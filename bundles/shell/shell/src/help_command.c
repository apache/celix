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
#include <stdint.h>

#include "celix_api.h"
#include "celix_shell.h"
#include "std_commands.h"

struct print_handle {
    char *cmdLine;
    FILE *out;
    FILE *err;
    bool callSucceeded;
};

static void printHelp(void *handle, void *svc) {
    celix_shell_t *shell = svc;
    struct print_handle *p = handle;
    char *cmdLine = p->cmdLine;
    FILE *out = p->out;
    FILE *err = p->err;

    uint32_t out_len = 256;
    char *sub = NULL;
    char *save_ptr = NULL;
    char out_str[out_len];

    memset(out_str, 0, sizeof(out_str));

    strtok_r(cmdLine, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
    sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);

    if (sub == NULL) {
        unsigned int i;
        celix_array_list_t *commands = NULL;

        shell->getCommands(shell->handle, &commands);
        for (i = 0; i < arrayList_size(commands); i++) {
            char *name = arrayList_get(commands, i);
            fprintf(out, "%s\n", name);
        }
        fprintf(out, "\nUse 'help <command-name>' for more information.\n");
        celix_arrayList_destroy(commands);
        p->callSucceeded = true; //main help is used
    } else {
        celix_status_t sub_status_desc;
        celix_status_t sub_status_usage;
        int i;
        celix_array_list_t *commands = NULL;
        shell->getCommands(shell->handle, &commands);
        bool cmdFound = false;
        for (i = 0; i < celix_arrayList_size(commands); i++) {
            char *name = celix_arrayList_get(commands, i);
            if (strcmp(sub, name) == 0) {
                cmdFound = true;
                char *usage_str = NULL;
                char *desc_str = NULL;

                sub_status_desc = shell->getCommandDescription(shell->handle, name, &desc_str);
                sub_status_usage = shell->getCommandUsage(shell->handle, name, &usage_str);

                if (sub_status_usage == CELIX_SUCCESS && sub_status_desc == CELIX_SUCCESS) {
                    fprintf(out, "Command     : %s\n", name);
                    fprintf(out, "Usage       : %s\n", usage_str == NULL ? "" : usage_str);
                    fprintf(out, "Description : %s\n", desc_str == NULL ? "" : desc_str);
                } else {
                    fprintf(err, "Error retrieving help info for command '%s'\n", sub);
                }

                free(usage_str);
                free(desc_str);
            }
            free(name);
        }
        celix_arrayList_destroy(commands);

        if (!cmdFound) {
            fprintf(out, "Command '%s' not found. Type 'help' to get an overview of the available commands\n", sub);
        }

        p->callSucceeded = cmdFound;
    }
}

bool helpCommand_execute(void *handle, const char *const_cmdLine, FILE *out, FILE *err) {
	celix_bundle_context_t *ctx = handle;
	struct print_handle printHandle;
	char *cmdLine = celix_utils_strdup(const_cmdLine);

    printHandle.cmdLine = cmdLine;
    printHandle.out = out;
    printHandle.err = err;
	bool called = celix_bundleContext_useService(ctx, CELIX_SHELL_SERVICE_NAME, &printHandle, printHelp);

	free(cmdLine);
    return called & printHandle.callSucceeded;
}
