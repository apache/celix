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

#include "utils.h"
#include "celix_bundle_context.h"
#include "celix_bundle.h"
#include "array_list.h"
#include "bundle_context.h"
#include "std_commands.h"
#include "shell_constants.h"

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
    bool listAllGroups;
} lb_options_t;

static char * psCommand_stateString(bundle_state_e state);

static void addToGroup(hash_map_t *map, const char *group, long bndId) {
    celix_array_list_t *ids = hashMap_get(map, group);
    if (ids == NULL) {
        ids = celix_arrayList_create();
        char *key = strndup(group, 1024 * 1024);
        hashMap_put(map, key, ids);
    }
    celix_arrayList_addLong(ids, bndId);
}

static void collectGroups(void *handle, const celix_bundle_t *bnd) {
    hash_map_t *map = handle;
    const char *group = celix_bundle_getGroup(bnd);
    if (group == NULL) {
        group = "-";
        addToGroup(map, group, celix_bundle_getId(bnd));
    } else {
        char *at = strstr(group, "/");
        if (at != NULL) {
            unsigned long size = at-group;
            char buf[size+1];
            strncpy(buf, group, size);
            buf[size] = '\0';
            addToGroup(map, buf, celix_bundle_getId(bnd));
        } else {
            addToGroup(map, group, celix_bundle_getId(bnd));
        }
    }
}

static void lbCommand_showGroups(celix_bundle_context_t *ctx, const lb_options_t *opts, FILE *out) {
    const char* startColor = "";
    const char* endColor = "";
    if (opts->useColors) {
        startColor = HEAD_COLOR;
        endColor = END_COLOR;
    }

    fprintf(out, "%s  Groups:%s\n", startColor, endColor);
    fprintf(out, "%s  %-20s %-20s %s\n", startColor, "Group", "Bundle Ids", endColor);

    hash_map_t *map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    celix_bundleContext_useBundle(ctx, 0, map, collectGroups);
    celix_bundleContext_useBundles(ctx, map, collectGroups);

    hash_map_iterator_t iter = hashMapIterator_construct(map);
    int count = 0;
    while (hashMapIterator_hasNext(&iter)) {
        startColor = "";
        endColor = "";
        if (opts->useColors) {
            startColor = count++ % 2 == 0 ? EVEN_COLOR : ODD_COLOR;
            endColor = END_COLOR;
        }

        hash_map_entry_t *entry = hashMapIterator_nextEntry(&iter);
        char *key = hashMapEntry_getKey(entry);
        fprintf(out, "%s  %-20s ", startColor, key);
        celix_array_list_t *ids = hashMapEntry_getValue(entry);
        int s = celix_arrayList_size(ids);
        for (int i = s-1; i >= 0; --i) { //note reverse to start with lower bundle id first
            long id = celix_arrayList_getLong(ids, (int)i);
            fprintf(out, "%li ", id);
        }
        fprintf(out, "%s\n", endColor);
        free(key);
        celix_arrayList_destroy(ids);
    }
    fprintf(out, "\n\n");
    hashMap_destroy(map, false, false);
}

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
        bundle_state_e state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
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
            sub_status = module_getSymbolicName(module_ptr, &name_str);
        }

        if (sub_status == CELIX_SUCCESS) {
            module_getGroup(module_ptr, &group_str);
        }

        if (sub_status == CELIX_SUCCESS) {
            if (opts->show_location) {
                sub_status = bundleArchive_getLocation(archive_ptr, &name_str);
            } else if (opts->show_symbolic_name) {
                // do nothing
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
            bool print = false;
            if (opts->listAllGroups) {
                print = true;
            } else if (opts->listGroup != NULL && group_str != NULL) {
                print = strncmp(opts->listGroup, group_str, strlen(opts->listGroup)) == 0;
            } else if (opts->listGroup == NULL){
                //listGroup == NULL -> only print not grouped bundles
                print = group_str == NULL;
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

celix_status_t lbCommand_execute(void *_ptr, char *command_line_str, FILE *out_ptr, FILE *err_ptr) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_context_t* ctx = _ptr;

    lb_options_t opts;
    memset(&opts, 0, sizeof(opts));

    const char* config = NULL;
    bundleContext_getPropertyWithDefault(ctx, SHELL_USE_ANSI_COLORS, SHELL_USE_ANSI_COLORS_DEFAULT_VALUE, &config);
    opts.useColors = config != NULL && strncmp("true", config, 5) == 0;


    opts.show_location        = false;
    opts.show_symbolic_name   = false;
    opts.show_update_location = false;

    if (!ctx || !command_line_str || !out_ptr || !err_ptr) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
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
            } else if (strcmp(sub_str, "-a") == 0) {
                opts.listAllGroups = true;
            } else {
                opts.listGroup = strdup(sub_str);
            }
            sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        }
    }

    lbCommand_showGroups(ctx, &opts, out_ptr);
    lbCommand_listBundles(ctx, &opts, out_ptr);

    free(opts.listGroup);

    return status;
}

static char * psCommand_stateString(bundle_state_e state) {
    switch (state) {
        case OSGI_FRAMEWORK_BUNDLE_ACTIVE:
            return "Active      ";
        case OSGI_FRAMEWORK_BUNDLE_INSTALLED:
            return "Installed   ";
        case OSGI_FRAMEWORK_BUNDLE_RESOLVED:
            return "Resolved    ";
        case OSGI_FRAMEWORK_BUNDLE_STARTING:
            return "Starting    ";
        case OSGI_FRAMEWORK_BUNDLE_STOPPING:
            return "Stopping    ";
        default:
            return "Unknown     ";
    }
}
