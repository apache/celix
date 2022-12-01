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
#include <celix_api.h>

#include "utils.h"
#include "celix_bundle_context.h"
#include "celix_bundle.h"
#include "array_list.h"
#include "bundle_context.h"
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
    bool show_update_location;

    //use color
    bool useColors;

    //group
    char *listGroup;
} lb_options_t;

static char * psCommand_stateString(bundle_state_e state);

static void lbCommand_listBundles(celix_bundle_context_t *ctx, const lb_options_t *opts, FILE *out) {
    const char *message_str = "Name";
    if (opts->show_location) {
        message_str = "Location";
    } else if (opts->show_symbolic_name) {
        message_str = "Symbolic name";
    } else if (opts->show_update_location) {
        message_str = "Update location";
    }

    const char* startColor = "";
    const char* endColor = "";
    if (opts->useColors) {
        startColor = HEAD_COLOR;
        endColor = END_COLOR;
    }
    fprintf(out, "%s  Bundles:%s\n", startColor, endColor);
    fprintf(out, "%s  %-5s %-12s %-40s %-20s%s\n", startColor, "ID", "State", message_str, "Group", endColor);

    array_list_t *bundles_ptr = NULL;
    bundleContext_getBundles(ctx, &bundles_ptr);
    unsigned int size = arrayList_size(bundles_ptr);

    bundle_pt bundles_array_ptr[size];

    for (unsigned int i = 0; i < size; i++) {
        bundles_array_ptr[i] = arrayList_get(bundles_ptr, i);
    }

    for (unsigned int i = 0; i < size - 1; i++) {
        for (unsigned int j = i + 1; j < size; j++) {
            bundle_pt first_ptr = bundles_array_ptr[i];
            bundle_pt second_ptr = bundles_array_ptr[j];

            bundle_archive_pt first_archive_ptr = NULL;
            bundle_archive_pt second_archive_ptr = NULL;

            long first_id;
            long second_id;

            bundle_getArchive(first_ptr, &first_archive_ptr);
            bundle_getArchive(second_ptr, &second_archive_ptr);

            bundleArchive_getId(first_archive_ptr, &first_id);
            bundleArchive_getId(second_archive_ptr, &second_id);

            if (first_id > second_id) {
                bundle_pt temp_ptr = bundles_array_ptr[i];
                bundles_array_ptr[i] = bundles_array_ptr[j];
                bundles_array_ptr[j] = temp_ptr;
            }
        }
    }

    for (unsigned int i = 0; i < size; i++) {
        celix_status_t sub_status;

        bundle_pt bundle_ptr = bundles_array_ptr[i];

        bundle_archive_pt archive_ptr = NULL;
        long id = 0;
        bundle_state_e state = CELIX_BUNDLE_STATE_UNKNOWN;
        const char *state_str = NULL;
        module_pt module_ptr = NULL;
        const char *name_str = NULL;
        const char *group_str = NULL;

        sub_status = bundle_getArchive(bundle_ptr, &archive_ptr);
        if (sub_status == CELIX_SUCCESS) {
            sub_status = bundleArchive_getId(archive_ptr, &id);
        }

        if (sub_status == CELIX_SUCCESS) {
            sub_status = bundle_getState(bundle_ptr, &state);
        }

        if (sub_status == CELIX_SUCCESS) {
            state_str = psCommand_stateString(state);

            sub_status = bundle_getCurrentModule(bundle_ptr, &module_ptr);
        }

        if (sub_status == CELIX_SUCCESS) {
            name_str = celix_bundle_getName(bundle_ptr);
        }

        if (sub_status == CELIX_SUCCESS) {
            module_getGroup(module_ptr, &group_str);
        }

        if (sub_status == CELIX_SUCCESS) {
            if (opts->show_location) {
                sub_status = bundleArchive_getLocation(archive_ptr, &name_str);
            } else if (opts->show_symbolic_name) {
                sub_status = module_getSymbolicName(module_ptr, &name_str);
            } else if (opts->show_update_location) {
                sub_status = bundleArchive_getLocation(archive_ptr, &name_str);
            }
        }

        if (sub_status == CELIX_SUCCESS) {
            startColor = "";
            endColor = "";
            if (opts->useColors) {
                startColor = i % 2 == 0 ? EVEN_COLOR : ODD_COLOR;
                endColor = END_COLOR;
            }
            bool print = true;
            if (opts->listGroup != NULL) {
                print = group_str != NULL && strstr(group_str, opts->listGroup) != NULL;
            }

            if (print) {
                group_str = group_str == NULL ? NONE_GROUP : group_str;
                fprintf(out, "%s  %-5li %-12s %-40s %-20s%s\n", startColor, id, state_str, name_str, group_str, endColor);
            }

        }

        if (sub_status != CELIX_SUCCESS) {
            break;
        }
    }
    fprintf(out, "\n\n");

    arrayList_destroy(bundles_ptr);
}

bool lbCommand_execute(void *handle, const char *const_command_line_str, FILE *out_ptr, FILE *err_ptr) {
    bundle_context_t* ctx = handle;

    char *command_line_str = celix_utils_strdup(const_command_line_str);

    lb_options_t opts;
    memset(&opts, 0, sizeof(opts));

    opts.useColors = celix_bundleContext_getPropertyAsBool(ctx, CELIX_SHELL_USE_ANSI_COLORS, CELIX_SHELL_USE_ANSI_COLORS_DEFAULT_VALUE);
    opts.show_location        = false;
    opts.show_symbolic_name   = false;
    opts.show_update_location = false;

    char *sub_str = NULL;
    char *save_ptr = NULL;

    strtok_r(command_line_str, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
    sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
    while (sub_str != NULL) {
        if (strcmp(sub_str, "-l") == 0) {
            opts.show_location = true;
        } else if (strcmp(sub_str, "-s") == 0) {
            opts.show_symbolic_name = true;
        } else if (strcmp(sub_str, "-u") == 0) {
            opts.show_update_location = true;
        } else if (strncmp(sub_str, "-", 1) == 0) {
            fprintf(out_ptr, "Ignoring unknown lb option '%s'\n", sub_str);
        } else {
            opts.listGroup = strdup(sub_str);
        }
        sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
    }

    lbCommand_listBundles(ctx, &opts, out_ptr);

    free(opts.listGroup);
    free(command_line_str);

    return true;
}

static char * psCommand_stateString(bundle_state_e state) {
    switch (state) {
        case CELIX_BUNDLE_STATE_ACTIVE:
            return "Active      ";
        case CELIX_BUNDLE_STATE_INSTALLED:
            return "Installed   ";
        case CELIX_BUNDLE_STATE_RESOLVED:
            return "Resolved    ";
        case CELIX_BUNDLE_STATE_STARTING:
            return "Starting    ";
        case CELIX_BUNDLE_STATE_STOPPING:
            return "Stopping    ";
        default:
            return "Unknown     ";
    }
}
