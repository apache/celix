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

#include "std_commands.h"
#include "celix_utils.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"
#include "celix_framework.h"
#include "celix_convert_utils.h"

bool updateCommand_execute(void *handle, const char *constCommandLine, FILE *outStream, FILE *errStream) {
    celix_bundle_context_t *ctx = handle;

    char* sub = NULL;
    char* savePtr = NULL;
    char* command = celix_utils_strdup(constCommandLine);
    strtok_r(command, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr); //ignore command name
    sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr);

    bool updateSucceeded = false;
    if (sub == NULL) {
        fprintf(errStream, "Incorrect number of arguments.\n");
    } else {
        while (sub != NULL) {
            bool converted;
            long bndId = celix_utils_convertStringToLong(sub, 0, &converted);
            bool exists = celix_bundleContext_isBundleInstalled(ctx, bndId);
            if (!converted) {
                fprintf(errStream, "Cannot convert '%s' to long (bundle id).\n", sub);
            } else if (!exists) {
                fprintf(outStream, "No bundle with id %li.\n", bndId);
            } else {
                fprintf(errStream, "Update bundle is not yet fully supported. Use at your own risk.\n");
                celix_framework_t* fw = celix_bundleContext_getFramework(ctx);
                celix_framework_updateBundleAsync(fw, bndId, NULL);
                updateSucceeded = true;
            }
            sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr);
        }
    }
    free(command);
    return updateSucceeded;
}

