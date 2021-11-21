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

#include "celix_api.h"
#include "std_commands.h"

bool stopCommand_execute(void *handle, const char *const_command, FILE *outStream, FILE *errStream) {
    celix_bundle_context_t *ctx = handle;

    char *sub_str = NULL;
    char *save_ptr = NULL;
    char *command = celix_utils_strdup(const_command);

    strtok_r(command, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
    sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);

    bool stoppedCalledAndSucceeded = false;
    if (sub_str == NULL) {
        fprintf(outStream, "Incorrect number of arguments.\n");
    } else {
        while (sub_str != NULL) {

            char *end_str = NULL;
            long bndId = strtol(sub_str, &end_str, 10);
            if (*end_str) {
                fprintf(errStream, "Bundle id '%s' is invalid, problem at %s\n", sub_str, end_str);
            } else {
                bool exists = celix_bundleContext_isBundleInstalled(ctx, bndId);
                if (exists) {
                    celix_framework_t* fw = celix_bundleContext_getFramework(ctx);
                    celix_framework_stopBundleAsync(fw, bndId);
                    stoppedCalledAndSucceeded = true;
                } else {
                    fprintf(outStream, "No bundle with id %li.\n", bndId);
                }
            }

            sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        }
    }

    free(command);

    return stoppedCalledAndSucceeded;
}
