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
 * help_command.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "array_list.h"
#include "bundle_context.h"
#include "shell.h"
#include "std_commands.h"

celix_status_t helpCommand_execute(void *_ptr, char *line_str, FILE *out_ptr, FILE *err_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_context_pt context_ptr = _ptr;
	service_reference_pt shell_service_reference_ptr = NULL;
	shell_service_pt shell_ptr = NULL;

	if (!context_ptr || !line_str || !out_ptr || !err_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_getServiceReference(context_ptr, (char *) OSGI_SHELL_SERVICE_NAME, &shell_service_reference_ptr);
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_getService(context_ptr, shell_service_reference_ptr, (void **) &shell_ptr);
	}

	if (status == CELIX_SUCCESS) {
        uint32_t out_len = 256;
        char *sub = NULL;
        char *save_ptr = NULL;
        char out_str[out_len];

        memset(out_str, 0, sizeof(out_str));

        strtok_r(line_str, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);

        if (sub == NULL) {
            unsigned int i;
            array_list_pt commands = NULL;

            status = shell_ptr->getCommands(shell_ptr->shell, &commands);
            for (i = 0; i < arrayList_size(commands); i++) {
                char *name = arrayList_get(commands, i);
                fprintf(out_ptr, "%s\n", name);
            }
            fprintf(out_ptr, "\nUse 'help <command-name>' for more information.\n");
            arrayList_destroy(commands);
        } else {
            celix_status_t sub_status_desc;
            celix_status_t sub_status_usage;
            int i;
            array_list_pt commands = NULL;
            shell_ptr->getCommands(shell_ptr->shell, &commands);
            for (i = 0; i < arrayList_size(commands); i++) {
                char *name = arrayList_get(commands, i);
                if (strcmp(sub, name) == 0) {
                    char *usage_str = NULL;
                    char *desc_str = NULL;

                    sub_status_desc = shell_ptr->getCommandDescription(shell_ptr->shell, name, &desc_str);
                    sub_status_usage = shell_ptr->getCommandUsage(shell_ptr->shell, name, &usage_str);

                    if (sub_status_usage == CELIX_SUCCESS && sub_status_desc == CELIX_SUCCESS) {
                        fprintf(out_ptr, "Command     : %s\n", name);
                        fprintf(out_ptr, "Usage       : %s\n", usage_str == NULL ? "" : usage_str);
                        fprintf(out_ptr, "Description : %s\n", desc_str == NULL ? "" : desc_str);
                    } else {
                        fprintf(err_ptr, "Error retreiving help info for command '%s'\n", sub);
                    }

                    if (sub_status_desc != CELIX_SUCCESS && status == CELIX_SUCCESS) {
                        status = sub_status_desc;
                    }
                    if (sub_status_usage != CELIX_SUCCESS && status == CELIX_SUCCESS) {
                        status = sub_status_usage;
                    }
                }
            }
            arrayList_destroy(commands);
        }
    }

    return status;
}
