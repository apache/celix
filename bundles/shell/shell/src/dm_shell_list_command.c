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


static const char * const OK_COLOR = "\033[92m";
static const char * const WARNING_COLOR = "\033[93m";
static const char * const NOK_COLOR = "\033[91m";
static const char * const END_COLOR = "\033[m";

void printInactiveBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors);

void printSpecifiedBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors, bool fullInfo, bool plantUmlInfo);

void printAllBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors, const celix_array_list_t *bundleIds,
                     bool fullInfo, bool plantUmlInfo);

static void printPlantUmlInfo(FILE *out, celix_dependency_manager_t *mng);
static void printUmlComponents(FILE *out, const celix_array_list_t *infos);

static void printUmlDependencies(FILE *out, const celix_array_list_t *infos);


static void parseCommandLine(const char*line, celix_array_list_t **requestedBundleIds, bool *fullInfo, bool *plantUmlInfo, bool *wtf, FILE *err) {
    *fullInfo = false;
    *wtf = false;
    char *str = strdup(line);
    // skip first argument since this is the command
    strtok(str," ");
    char* tok = strtok(NULL," ");
    *requestedBundleIds = celix_arrayList_create();
    while (tok) {
        if (strncmp("wtf", tok, strlen("wtf")) == 0) {
            *wtf = true;
        } else if (tok[0] == 'f') { // f or full argument => show full info
            *fullInfo = true;
        } else if (tok[0] == 'u') { // u or full plant UML syntax output =>copy paste in a file java -jar plantuml.jar <file> gives a nice dependency graph
            *plantUmlInfo = true;
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

static void printFullInfo(FILE *out, bool colors, long bundleId, const char* bndName, dm_component_info_pt compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? OK_COLOR : NOK_COLOR;
        endColors = END_COLOR;
    }
    fprintf(out, "%sComponent: Name=%s%s\n", startColors, compInfo->name, endColors);
    fprintf(out, "|- UUID   = %s\n", compInfo->id);
    fprintf(out, "|- Active = %s\n", compInfo->active ? "true" : "false");
    fprintf(out, "|- State  = %s\n", compInfo->state);
    fprintf(out, "|- Bundle = %li (%s)\n", bundleId, bndName);
    fprintf(out, "|- Nr of times started = %i\n", (int)compInfo->nrOfTimesStarted);
    fprintf(out, "|- Nr of times resumed = %i\n", (int)compInfo->nrOfTimesResumed);

    fprintf(out, "|- Interfaces (%d):\n", celix_arrayList_size(compInfo->interfaces));
    for (int interfCnt = 0; interfCnt < celix_arrayList_size(compInfo->interfaces); interfCnt++) {
        dm_interface_info_pt intfInfo = celix_arrayList_get(compInfo->interfaces, interfCnt);
        fprintf(out, "   |- %sInterface %i: %s%s\n", startColors, (interfCnt+1), intfInfo->name, endColors);

        hash_map_iterator_t iter = hashMapIterator_construct((hash_map_pt) intfInfo->properties);
        char *key = NULL;
        while ((key = hashMapIterator_nextKey(&iter)) != NULL) {
            fprintf(out, "      | %15s = %s\n", key, celix_properties_get(intfInfo->properties, key, "!ERROR!"));
        }
    }

    fprintf(out, "|- Dependencies (%d):\n", celix_arrayList_size(compInfo->dependency_list));
    for (int depCnt = 0; depCnt < celix_arrayList_size(compInfo->dependency_list); ++depCnt) {
        dm_service_dependency_info_pt dependency;
        dependency = celix_arrayList_get(compInfo->dependency_list, depCnt);
        const char *depStartColors = "";
        if (colors) {
            if (dependency->required) {
                depStartColors = dependency->available ? OK_COLOR : NOK_COLOR;
            } else {
                depStartColors = dependency->available ? OK_COLOR : WARNING_COLOR;
            }
        }
        fprintf(out, "   |- %sDependency %i: %s%s\n", depStartColors, (depCnt+1), dependency->serviceName == NULL ? "(any)" : dependency->serviceName, endColors);
        fprintf(out, "      | %15s = %s\n", "Available", dependency->available ? "true " : "false");
        fprintf(out, "      | %15s = %s\n", "Required", dependency->required ? "true " : "false");
        fprintf(out, "      | %15s = %s\n", "Version Range", dependency->versionRange == NULL ? "N/A" : dependency->versionRange);
        fprintf(out, "      | %15s = %s\n", "Filter", dependency->filter == NULL ? "N/A" : dependency->filter);
    }
    fprintf(out, "\n");

}
static void printPlantUmlInfo(FILE *out, celix_dependency_manager_t *mng) {

    celix_array_list_t *infos = celix_dependencyManager_createInfos(mng);

    fprintf(out, "@startuml\n");

    printUmlComponents(out, infos);
    printUmlDependencies(out, infos);

    fprintf(out, "@enduml\n");

    celix_dependencyManager_destroyInfos(mng, infos);

}

static void printUmlComponents(FILE *out, const celix_array_list_t *infos) {
    unsigned int nof_bundles = celix_arrayList_size(infos);
    for (int bnd = 0; bnd < nof_bundles; ++bnd) {
        celix_dependency_manager_info_t *info = celix_arrayList_get(infos, bnd);
        for (int cmpCnt = 0; cmpCnt < celix_arrayList_size(info->components); ++cmpCnt) {
            celix_dm_component_info_t *compInfo = celix_arrayList_get(info->components, cmpCnt);
            fprintf(out, "class %s\n", compInfo->name);
            for (int interfCnt = 0; interfCnt < celix_arrayList_size(compInfo->interfaces); interfCnt++) {
                dm_interface_info_pt intfInfo = celix_arrayList_get(compInfo->interfaces, interfCnt);
                fprintf(out, "interface %s\n", intfInfo->name);
            }
        }
    }
    // depdencies at last, since the type overwrites the interface for a nice graph
    for (int bnd = 0; bnd < nof_bundles; ++bnd) {
        celix_dependency_manager_info_t *info = celix_arrayList_get(infos, bnd);
        for (int cmpCnt = 0; cmpCnt < celix_arrayList_size(info->components); ++cmpCnt) {
            celix_dm_component_info_t *compInfo = celix_arrayList_get(info->components, cmpCnt);
            for (int depCnt = 0; depCnt < celix_arrayList_size(compInfo->dependency_list); ++depCnt) {
                dm_service_dependency_info_pt dependency;
                dependency = celix_arrayList_get(compInfo->dependency_list, depCnt);
                fprintf(out, "interface %s\n", dependency->serviceName);
            }
        }
    }
    fprintf(out, "\n");
}


static void printUmlDependencies(FILE *out, const celix_array_list_t *infos) {
    unsigned int nof_bundles = celix_arrayList_size(infos);

    // Now print the UML relations
    for (int bnd = 0; bnd < nof_bundles; ++bnd) {
        celix_dependency_manager_info_t *info = celix_arrayList_get(infos, bnd);
        for (int cmpCnt = 0; cmpCnt < celix_arrayList_size(info->components); ++cmpCnt) {
            celix_dm_component_info_t *compInfo = celix_arrayList_get(info->components, cmpCnt);
            for (int interfCnt = 0; interfCnt < celix_arrayList_size(compInfo->interfaces); interfCnt++) {
                dm_interface_info_pt intfInfo = celix_arrayList_get(compInfo->interfaces, interfCnt);
                fprintf(out, "%s --|> %s\n", compInfo->name, intfInfo->name);
            }
            for (int depCnt = 0; depCnt < celix_arrayList_size(compInfo->dependency_list); ++depCnt) {
                dm_service_dependency_info_pt dependency;
                dependency = celix_arrayList_get(compInfo->dependency_list, depCnt);
                if (dependency->available) {
                    fprintf(out, "%s *--> %s\n", compInfo->name, dependency->serviceName);
                } else if (dependency->required) {
                    fprintf(out, "%s *..> %s\n", compInfo->name, dependency->serviceName);
                } else {
                    fprintf(out, "%s ..> %s\n", compInfo->name, dependency->serviceName);
                }
            }
        }
    }
}


static void printBasicInfo(FILE *out, bool colors, long bundleId, const char* bndName, dm_component_info_pt compInfo) {
    const char *startColors = "";
    const char *endColors = "";
    if (colors) {
        startColors = compInfo->active ? OK_COLOR : NOK_COLOR;
        endColors = END_COLOR;
    }
    fprintf(out, "Component: Name=%s, ID=%s, %sActive=%s%s, State=%s, Bundle=%li (%s)\n", compInfo->name, compInfo->id,
            startColors, compInfo->active ? "true " : "false", endColors, compInfo->state, bundleId, bndName);

}

static void dm_printInfo(FILE *out, bool useColors, bool fullInfo, celix_dependency_manager_info_t *info) {
    if (info != NULL) {
        int size = celix_arrayList_size(info->components);
        if (size > 0) {
            for ( int cmpCnt = 0; cmpCnt < size; cmpCnt++) {
                dm_component_info_pt compInfo = celix_arrayList_get(info->components, cmpCnt);
                if (fullInfo) {
                    printFullInfo(out, useColors, info->bndId, info->bndSymbolicName, compInfo);
                } else {
                    printBasicInfo(out, useColors, info->bndId, info->bndSymbolicName, compInfo);
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
    bundleContext_getPropertyWithDefault(ctx, CELIX_SHELL_USE_ANSI_COLORS, CELIX_SHELL_USE_ANSI_COLORS_DEFAULT_VALUE, &config);
    bool useColors = config != NULL && strncmp("true", config, 5) == 0;

    celix_array_list_t *bundleIds = NULL;
    bool fullInfo = false;
    bool plantUmlInfo = false;
    bool wtf = false;
    parseCommandLine(line, &bundleIds, &fullInfo, &plantUmlInfo, &wtf, err);

    if (wtf) {
        printInactiveBundles(out, mng, useColors);
    } else if (plantUmlInfo) {
        printPlantUmlInfo(out, mng);
    } else if (celix_arrayList_size(bundleIds) == 0) {
        printSpecifiedBundles(out, mng, useColors, fullInfo, plantUmlInfo);
    } else {
        printAllBundles(out, mng, useColors, bundleIds, fullInfo, plantUmlInfo);
    }

    celix_arrayList_destroy(bundleIds);

    return CELIX_SUCCESS;
}

void printAllBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors, const celix_array_list_t *bundleIds,
                     bool fullInfo, bool plantUmlInfo) {
    for (int i = 0; i < celix_arrayList_size(bundleIds); ++i) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        celix_dependency_manager_info_t *info = celix_dependencyManager_createInfo(mng, bndId);
        if (info != NULL) {
            dm_printInfo(out, useColors, fullInfo, info);
            celix_dependencyManager_destroyInfo(mng, info);
        }
    }
}

void printSpecifiedBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors, bool fullInfo, bool plantUmlInfo) {
    celix_array_list_t *infos = celix_dependencyManager_createInfos(mng);
    if (plantUmlInfo) {
        printPlantUmlInfo(out, mng);
    } else {
        for (int i = 0; i < celix_arrayList_size(infos); ++i) {
            celix_dependency_manager_info_t *info = celix_arrayList_get(infos, i);
            dm_printInfo(out, useColors, fullInfo, info);
        }
    }
    celix_dependencyManager_destroyInfos(mng, infos);
}

void
printInactiveBundles(FILE *out, celix_dependency_manager_t *mng, bool useColors) {//only print dm that are not active
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
                printFullInfo(out, useColors, info->bndId, info->bndSymbolicName, cmpInfo);
            }
        }
    }
    celix_dependencyManager_destroyInfos(mng, infos);
    if (allActive) {
        fprintf(out, "No problem all %i dependency manager components are active\n", nrOfComponents);
    }
}
