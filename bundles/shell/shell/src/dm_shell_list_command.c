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
#include "dm_dependency_manager.h"


static const char * const OK_COLOR = "\033[92m";
static const char * const WARNING_COLOR = "\033[93m";
static const char * const NOK_COLOR = "\033[91m";
static const char * const END_COLOR = "\033[m";

static void parseCommandLine(const char*line, array_list_pt *requestedBundleIds, bool *fullInfo, FILE *err) {
    *fullInfo = false;
    char *str = strdup(line);
    // skip first argument since this is the command
    strtok(str," ");
    char* tok = strtok(NULL," ");
    *requestedBundleIds = NULL;
    arrayList_create(requestedBundleIds);
    while (tok) {
        if (tok[0] == 'f') { // f or full argument => show full info
            *fullInfo = true;
        } else if ( (tok[0] >= '0') && (tok[0] <= '9')) { // bundle id
            long *id = malloc(sizeof(*id));
            *id = strtol(tok, NULL, 10);
            arrayList_add(*requestedBundleIds, id);
        } else {
            fprintf (err, "DM: Skipping unknown argument: %s", tok );
        }
        tok = strtok(NULL," ");
    }
    free (str);
}

static void destroyBundleIdList(array_list_pt ids) {
    unsigned int size = arrayList_size(ids);
    for (unsigned int i = 0; i < size; i++) {
        free(arrayList_get(ids, i));
    }
    arrayList_destroy(ids);
}

/*
 * Check if the ID is in the array list. If  arrayist is empty also true is returned so that all
 * bundles are shown
 */
static bool is_bundleId_in_list(array_list_pt ids, long id) {
    unsigned int size = arrayList_size(ids);
    bool result = false;
    if (size == 0) {
        result = true;
    }
    for(unsigned int i = 0; i < size; ++i) {
        if (*((long*)arrayList_get(ids, i))  == id) {
            result = true;
            break;
        }
    }
    return result;
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

celix_status_t dmListCommand_execute(void* handle, char * line, FILE *out, FILE *err) {
    bundle_context_t *ctx = handle;

    array_list_t *bundles = NULL;
    bundleContext_getBundles(ctx, &bundles);

    const char *config = NULL;
    bundleContext_getPropertyWithDefault(ctx, SHELL_USE_ANSI_COLORS, SHELL_USE_ANSI_COLORS_DEFAULT_VALUE, &config);
    bool useColors = config != NULL && strncmp("true", config, 5) == 0;

    array_list_pt bundleIds = NULL;
    bool fullInfo = false;
    parseCommandLine(line, &bundleIds, &fullInfo, err);

    if (bundles != NULL) {
        unsigned int size = arrayList_size(bundles);
        for (unsigned int i = 0; i < size; ++i) {
            long bndId = -1;
            bundle_t *bnd = arrayList_get(bundles, i);
            bundle_getBundleId(bnd, &bndId);
            if (!is_bundleId_in_list(bundleIds, bndId)) {
                continue;
            }
            bundle_context_t *bndCtx = NULL;
            bundle_getContext(bnd, &bndCtx);
            if (bndCtx != NULL) {
                dm_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(bndCtx);
                dm_dependency_manager_info_t *info = NULL;
                dependencyManager_getInfo(mng, &info);
                if (info != NULL) {
                    size_t size = celix_arrayList_size(info->components);
                    if (size > 0) {
                        fprintf(out, "[Bundle: %ld]\n", bndId);
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
                    dependencyManager_destroyInfo(mng, info);
                }

            }
        }
    }

    destroyBundleIdList(bundleIds);

    return CELIX_SUCCESS;
}
