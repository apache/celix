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
 * start_command.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "std_commands.h"
#include "bundle_context.h"

celix_status_t startCommand_execute(void *_ptr, char *command_line_str, FILE *out_ptr, FILE *err_ptr) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_context_pt context_ptr = _ptr;

    if (!context_ptr || !command_line_str || !out_ptr || !err_ptr) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        char *sub_str = NULL;
        char *save_ptr = NULL;

        strtok_r(command_line_str, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);

        if (sub_str == NULL) {
            fprintf(out_ptr, "Incorrect number of arguments.\n");
        } else {
            while (sub_str != NULL) {
                celix_status_t sub_status = CELIX_SUCCESS;

                bundle_pt bundle_ptr = NULL;

                char *end_str = NULL;
                long id = strtol(sub_str, &end_str, 10);
                if (*end_str) {
                    sub_status = CELIX_ILLEGAL_ARGUMENT;
                    fprintf(err_ptr, "Bundle id '%s' is invalid, problem at %s\n", sub_str, end_str);
                }

                if (sub_status == CELIX_SUCCESS) {
                    sub_status = bundleContext_getBundleById(context_ptr, id, &bundle_ptr);
                }

                if (sub_status == CELIX_SUCCESS) {
                    bundle_startWithOptions(bundle_ptr, 0);
                }

                if (sub_status != CELIX_SUCCESS) {
                    status = sub_status;
                    break;
                }

                sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
            }
        }

    }

    return status;
}
