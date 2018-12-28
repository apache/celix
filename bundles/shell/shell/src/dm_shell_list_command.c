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

#include <stdlib.h>
#include <string.h>
#include <shell_constants.h>
#include "celix_bundle_context.h"
#include "celix_dependency_manager.h"


static const char * const OK_COLOR = "\033[92m";
static const char * const WARNING_COLOR = "\033[93m";
static const char * const NOK_COLOR = "\033[91m";
static const char * const END_COLOR = "\033[m";

static void parseCommandLine(const char*line, celix_array_list_t **requestedBundleIds, bool *fullInfo, FILE *err) {
    *fullInfo = false;
    char *str = strdup(line);
    // skip first argument since this is the command
    strtok(str," ");
    char* tok = strtok(NULL," ");
    *requestedBundleIds = celix_arrayList_create();
    while (tok) {
        if (tok[0] == 'f') { // f or full argument => show full info
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

static void printFullInfo(FILE *out, bool colors, dm_component_info_pt compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? OK_COLOR : NOK_COLOR;
        endColors = END_COLOR;
    }
    fprintf(out, "Component: Name=%s\n|- ID=%s, %sActive=%s%s, State=%s\n", compInfo->name, compInfo->id,
            startColors, compInfo->active ? "true " : "false", endColors, compInfo->state);
    fprintf(out, "|- Interfaces (%d):\n", arrayList_size(compInfo->interfaces));
    for (unsigned int interfCnt = 0; interfCnt < arrayList_size(compInfo->interfaces); interfCnt++) {
        dm_interface_info_pt intfInfo = arrayList_get(compInfo->interfaces, interfCnt);
        fprintf(out, "   |- Interface: %s\n", intfInfo->name);

        hash_map_iterator_t iter = hashMapIterator_construct((hash_map_pt) intfInfo->properties);
        char *key = NULL;
        while ((key = hashMapIterator_nextKey(&iter)) != NULL) {
            fprintf(out, "      | %15s = %s\n", key, properties_get(intfInfo->properties, key));
        }
    }

    fprintf(out, "|- Dependencies (%d):\n", arrayList_size(compInfo->dependency_list));
    for (unsigned int depCnt = 0; depCnt < arrayList_size(compInfo->dependency_list); depCnt++) {
        dm_service_dependency_info_pt dependency;
        dependency = arrayList_get(compInfo->dependency_list, depCnt);
        const char *depStartColors = "";
        const char *depEndColors = "";
        if (colors) {
            if (dependency->required) {
                depStartColors = dependency->available ? OK_COLOR : NOK_COLOR;
            } else {
                depStartColors = dependency->available ? OK_COLOR : WARNING_COLOR;
            }

            depEndColors = END_COLOR;
        }
        fprintf(out, "   |- Dependency: %sAvailable = %s%s, Required = %s, Filter = %s\n", depStartColors,
                dependency->available ? "true " : "false", depEndColors,
                dependency->required ? "true " : "false", dependency->filter);
    }
    fprintf(out, "\n");

}

static void printBasicInfo(FILE *out, bool colors, dm_component_info_pt compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? OK_COLOR : NOK_COLOR;
        endColors = END_COLOR;
    }
    fprintf(out, "Component: Name=%s, ID=%s, %sActive=%s%s, State=%s\n", compInfo->name, compInfo->id,
            startColors, compInfo->active ? "true " : "false", endColors, compInfo->state);

}

static void dm_printInfo(FILE *out, bool useColors, bool fullInfo, celix_dependency_manager_info_t *info) {
    if (info != NULL) {
        int size = celix_arrayList_size(info->components);
        if (size > 0) {
            fprintf(out, "[Bundle: %ld]\n", info->bndId);
            for (unsigned int cmpCnt = 0; cmpCnt < size; cmpCnt++) {
                dm_component_info_pt compInfo = celix_arrayList_get(info->components, cmpCnt);
                if (fullInfo) {
                    printFullInfo(out, useColors, compInfo);
                } else {
                    printBasicInfo(out, useColors, compInfo);
                }
            }
            fprintf(out, "\n");
        }
    }
}

celix_status_t dmListCommand_execute(void* handle, char * line, FILE *out, FILE *err) {
    celix_bundle_context_t *ctx = handle;
    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);

    const char *config = NULL;
    bundleContext_getPropertyWithDefault(ctx, SHELL_USE_ANSI_COLORS, SHELL_USE_ANSI_COLORS_DEFAULT_VALUE, &config);
    bool useColors = config != NULL && strncmp("true", config, 5) == 0;

    celix_array_list_t *bundleIds = NULL;
    bool fullInfo = false;
    parseCommandLine(line, &bundleIds, &fullInfo, err);

    if (celix_arrayList_size(bundleIds) == 0) {
        celix_array_list_t *infos = celix_dependencyManager_createInfos(mng);
        for (int i = 0; i < celix_arrayList_size(infos); ++i) {
            celix_dependency_manager_info_t *info = celix_arrayList_get(infos, i);
            dm_printInfo(out, useColors, fullInfo, info);
        }
        celix_dependencyManager_destroyInfos(mng, infos);
    } else {
        for (int i = 0; i < celix_arrayList_size(bundleIds); ++i) {
            long bndId = celix_arrayList_getLong(bundleIds, i);
            celix_dependency_manager_info_t *info = celix_dependencyManager_createInfo(mng, bndId);
            if (info != NULL) {
                dm_printInfo(out, useColors, fullInfo, info);
                celix_dependencyManager_destroyInfo(mng, info);
            }
        }
    }

    celix_arrayList_destroy(bundleIds);

    return CELIX_SUCCESS;
}
