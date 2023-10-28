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

#include "celix_constants.h"
#include "celix_utils.h"
#include "celix_bundle_context.h"
#include "celix_bundle.h"
#include "std_commands.h"
#include "celix_shell_constants.h"

static const char * const HEAD_COLOR = "\033[4m"; //underline
static const char * const EVEN_COLOR = "\033[1m"; //bold
static const char * const ODD_COLOR = "\033[3m";  //italic
static const char * const END_COLOR = "\033[0m";


#define NONE_GROUP ""

typedef struct lb_options {
    //details
    bool show_location;
    bool show_symbolic_name;

    //use color
    bool useColors;

    //group
    char *listGroup;
} lb_options_t;

typedef struct lb_command_bundle_info {
    long id;
    char* name;
    char* symbolicName;
    celix_version_t* version;
    char* group;
    char* location;
    bundle_state_e state;
} lb_command_bundle_info_t;

static int lbCommand_bundleInfoCmp(celix_array_list_entry_t a, celix_array_list_entry_t b) {
    lb_command_bundle_info_t* infoA = a.voidPtrVal;
    lb_command_bundle_info_t* infoB = b.voidPtrVal;
    if (infoA->id < infoB->id) {
        return -1;
    } else if (infoA->id > infoB->id) {
        return 1;
    } else {
        return 0;
    }
}

static void lbCommand_collectBundleInfo_callback(void* data, const celix_bundle_t* bnd) {
    celix_array_list_t* infoEntries = data;
    lb_command_bundle_info_t* info = malloc(sizeof(*info));
    if (info != NULL) {
        info->id = celix_bundle_getId(bnd);
        info->name = celix_utils_strdup(celix_bundle_getName(bnd));
        info->location = celix_bundle_getLocation(bnd);
        info->symbolicName = celix_utils_strdup(celix_bundle_getSymbolicName(bnd));
        info->version = celix_version_copy(celix_bundle_getVersion(bnd));
        info->group = celix_utils_strdup(celix_bundle_getGroup(bnd));
        info->state = celix_bundle_getState(bnd);
        celix_arrayList_add(infoEntries, info);
    }
}

static void lbCommand_freeBundleInfoEntry(void* entry) {
    lb_command_bundle_info_t* info = entry;
    free(info->name);
    free(info->symbolicName);
    free(info->location);
    free(info->group);
    celix_version_destroy(info->version);
    free(info);
}

static celix_array_list_t* lbCommand_collectBundleInfo(celix_bundle_context_t *ctx) {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.simpleRemovedCallback = lbCommand_freeBundleInfoEntry;
    celix_array_list_t* infoEntries = celix_arrayList_createWithOptions(&opts);
    if (infoEntries != NULL) {
        celix_bundleContext_useBundles(ctx, (void*)infoEntries, lbCommand_collectBundleInfo_callback);
        celix_bundleContext_useBundle(ctx, CELIX_FRAMEWORK_BUNDLE_ID, (void*)infoEntries, lbCommand_collectBundleInfo_callback);
        celix_arrayList_sortEntries(infoEntries, lbCommand_bundleInfoCmp);
    }
    return infoEntries;
}

static void lbCommand_listBundles(celix_bundle_context_t *ctx, const lb_options_t *opts, FILE *out) {
    celix_array_list_t* infoEntries = lbCommand_collectBundleInfo(ctx);

    const char* messageStr = "Name";
    if (opts->show_location) {
        messageStr = "Location";
    } else if (opts->show_symbolic_name) {
        messageStr = "Symbolic name";
    }

    const char* startColor = "";
    const char* endColor = "";
    if (opts->useColors) {
        startColor = HEAD_COLOR;
        endColor = END_COLOR;
    }
    fprintf(out, "%s  Bundles:%s\n", startColor, endColor);
    fprintf(out, "%s  %-5s %-12s %-40s %-20s%s\n", startColor, "ID", "State", messageStr, "Group", endColor);

    for (int i = 0; infoEntries != NULL && i < celix_arrayList_size(infoEntries); i++) {
        lb_command_bundle_info_t* info = celix_arrayList_get(infoEntries, i);

        const char* printValue = info->name;
        if (opts->show_location) {
            printValue = info->location;
        } else if (opts->show_symbolic_name) {
            printValue = info->symbolicName;
        }

        startColor = "";
        endColor = "";
        if (opts->useColors) {
            startColor = i % 2 == 0 ? EVEN_COLOR : ODD_COLOR;
            endColor = END_COLOR;
        }
        bool print = true;
        if (opts->listGroup != NULL) {
            print = info->group != NULL && strstr(info->group, opts->listGroup) != NULL;
        }

        if (print) {
            const char* printGroup = celix_utils_isStringNullOrEmpty(info->group) ? NONE_GROUP : info->group;
            fprintf(out, "%s  %-5li %-12s %-40s %-20s%s\n",
                    startColor,
                    info->id,
                    celix_bundleState_getName(info->state),
                    printValue,
                    printGroup,
                    endColor);
        }
    }
    fprintf(out, "\n\n");

    celix_arrayList_destroy(infoEntries);
}

bool lbCommand_execute(void *handle, const char *const_command_line_str, FILE *out_ptr, FILE *err_ptr) {
    bundle_context_t* ctx = handle;

    char *command_line_str = celix_utils_strdup(const_command_line_str);

    lb_options_t opts;
    memset(&opts, 0, sizeof(opts));

    opts.useColors = celix_bundleContext_getPropertyAsBool(ctx, CELIX_SHELL_USE_ANSI_COLORS, CELIX_SHELL_USE_ANSI_COLORS_DEFAULT_VALUE);
    opts.show_location        = false;
    opts.show_symbolic_name   = false;

    char *sub_str = NULL;
    char *save_ptr = NULL;

    strtok_r(command_line_str, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    sub_str = strtok_r(NULL, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    while (sub_str != NULL) {
        if (strcmp(sub_str, "-l") == 0) {
            opts.show_location = true;
        } else if (strcmp(sub_str, "-s") == 0) {
            opts.show_symbolic_name = true;
        } else if (strncmp(sub_str, "-", 1) == 0) {
            fprintf(out_ptr, "Ignoring unknown lb option '%s'\n", sub_str);
        } else {
            opts.listGroup = strdup(sub_str);
        }
        sub_str = strtok_r(NULL, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    }

    lbCommand_listBundles(ctx, &opts, out_ptr);

    free(opts.listGroup);
    free(command_line_str);

    return true;
}
