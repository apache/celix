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

#include "celix_shell_constants.h"
#include "celix_bundle_context.h"
#include "celix_dependency_manager.h"
#include "celix_dm_component.h"
#include "std_commands.h"

static void parseCommandLine(const char*line, celix_array_list_t **requestedBundleIds, bool *fullInfo, bool *wtf, FILE *err) {
    *fullInfo = false;
    *wtf = false;
    char *str = strdup(line);
    // skip first argument since this is the command
    strtok(str," ");
    char* tok = strtok(NULL," ");
    *requestedBundleIds = celix_arrayList_createLongArray();
    while (tok) {
        if (strncmp("wtf", tok, strlen("wtf")) == 0) {
            *wtf = true;
        } else if (tok[0] == 'f') { // f or full argument => show full info
            *fullInfo = true;
        } else if ( (tok[0] >= '0') && (tok[0] <= '9')) { // bundle id
            long id = strtol(tok, NULL, 10);
            celix_arrayList_addLong(*requestedBundleIds, id);
        } else {
            fprintf (err, "DM: Skipping unknown argument: %s", tok );
        }
        tok = strtok(NULL," ");
    }
    free (str);
}

bool dmListCommand_execute(void* handle, const char* line, FILE *out, FILE *err) {
    celix_bundle_context_t *ctx = handle;
    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);

    bool useColors = celix_bundleContext_getPropertyAsBool(ctx, CELIX_SHELL_USE_ANSI_COLORS, CELIX_SHELL_USE_ANSI_COLORS_DEFAULT_VALUE);

    celix_array_list_t *bundleIds = NULL;
    bool fullInfo = false;
    bool wtf = false;
    parseCommandLine(line, &bundleIds, &fullInfo, &wtf, err);

    if (wtf) {
        //only print dm that are not active
        bool allActive = true;
        int nrOfComponents = 0;
        celix_array_list_t *infos = celix_dependencyManager_createInfos(mng);
        for (int i = 0; i < celix_arrayList_size(infos); ++i) {
            celix_dependency_manager_info_t *info = celix_arrayList_get(infos, i);
            for (int k = 0; k < celix_arrayList_size(info->components); ++k) {
                celix_dm_component_info_t *cmpInfo = celix_arrayList_get(info->components, k);
                nrOfComponents += 1;
                if (!cmpInfo->active) {
                    allActive = false;
                    celix_dmComponent_printComponentInfo(cmpInfo, true, useColors, out);
                }
            }
        }
        celix_dependencyManager_destroyInfos(mng, infos);
        if (allActive) {
            fprintf(out, "No problem all %i dependency manager components are active\n", nrOfComponents);
        }
    } else if (celix_arrayList_size(bundleIds) == 0) {
        celix_dependencyManager_printInfo(mng, fullInfo, useColors, out);
    } else {
        for (int i = 0; i < celix_arrayList_size(bundleIds); ++i) {
            long bndId = celix_arrayList_getLong(bundleIds, i);
            celix_dependencyManager_printInfoForBundle(mng, fullInfo, useColors, bndId, out);
        }
    }

    celix_arrayList_destroy(bundleIds);

    return true;
}
