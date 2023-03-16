/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include "bundle_command.h"
#include "celix_bundle_context.h"
#include "celix_convert_utils.h"
#include "celix_utils.h"
#include <stdlib.h>
#include <string.h>

bool bundleCommand_execute(void *handle, const char *constCommandLine, FILE *outStream, FILE *errStream, bundle_control_fpt ctrl) {
    celix_bundle_context_t *ctx = handle;

    char* sub = NULL;
    char* savePtr = NULL;
    char* command = celix_utils_strdup(constCommandLine);
    strtok_r(command, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr); //ignore command name
    sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr);
    celix_array_list_t* bundleIds = celix_arrayList_create();
    bool validArgs = true;

    if (sub == NULL) {
        fprintf(errStream, "Incorrect number of arguments.\n");
        validArgs = false;
    }

    for (; sub != NULL; sub = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &savePtr)) {
        bool converted;
        long bndId = celix_utils_convertStringToLong(sub, 0, &converted);
        if (!converted) {
            validArgs = false;
            fprintf(errStream, "Cannot convert '%s' to long (bundle id).\n", sub);
            continue;
        }
        if (!celix_bundleContext_isBundleInstalled(ctx, bndId)) {
            validArgs = false;
            fprintf(outStream, "No bundle with id %li.\n", bndId);
            continue;
        }
        celix_arrayList_addLong(bundleIds, bndId);
    }
    free(command);

    bool succeeded = false;
    if (!validArgs) {
        succeeded = false;
        goto cleanup;
    }
    for (int i = 0; i < celix_arrayList_size(bundleIds); i++) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        celix_framework_t* fw = celix_bundleContext_getFramework(ctx);
        ctrl(fw, bndId);
        succeeded = true;
    }

cleanup:
    celix_arrayList_destroy(bundleIds);
    return succeeded;
}
