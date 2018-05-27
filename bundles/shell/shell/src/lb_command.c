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
/*
 * std_shell_commands.c
 *
 *  \date       March 27, 2014
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"
#include "std_commands.h"
#include "shell_constants.h"

static const char * const HEAD_COLOR = "\033[4m"; //underline
static const char * const EVEN_COLOR = "\033[1m"; //bold
static const char * const ODD_COLOR = "\033[3m";  //italic
static const char * const END_COLOR = "\033[0m";

static char * psCommand_stateString(bundle_state_e state); 

celix_status_t psCommand_execute(void *_ptr, char *command_line_str, FILE *out_ptr, FILE *err_ptr) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_context_t* ctx = _ptr;

    const char* config = NULL;
    bundleContext_getPropertyWithDefault(ctx, SHELL_USE_ANSI_COLORS, SHELL_USE_ANSI_COLORS_DEFAULT_VALUE, &config);
    bool useColors = config != NULL && strncmp("true", config, 5) == 0;

    bundle_context_pt context_ptr = _ptr;
    array_list_pt bundles_ptr     = NULL;

    bool show_location        = false;
    bool show_symbolic_name   = false;
    bool show_update_location = false;
    char *message_str         = "Name";

    if (!context_ptr || !command_line_str || !out_ptr || !err_ptr) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        status = bundleContext_getBundles(context_ptr, &bundles_ptr);
    }

    if (status == CELIX_SUCCESS) {
        char *sub_str = NULL;
        char *save_ptr = NULL;

        strtok_r(command_line_str, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        while (sub_str != NULL) {
            if (strcmp(sub_str, "-l") == 0) {
                show_location = true;
                message_str = "Location";
            } else if (strcmp(sub_str, "-s") == 0) {
                show_symbolic_name = true;
                message_str = "Symbolic name";
            } else if (strcmp(sub_str, "-u") == 0) {
                show_update_location = true;
                message_str = "Update location";
            }
            sub_str = strtok_r(NULL, OSGI_SHELL_COMMAND_SEPARATOR, &save_ptr);
        }

	const char* startColor = "";
	const char* endColor = "";
	if (useColors) {
		startColor = HEAD_COLOR;
		endColor = END_COLOR;
	} 
        fprintf(out_ptr, "%s  %-5s %-12s %s%s\n", startColor, "ID", "State", message_str, endColor);

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
                if (show_location) {
                    sub_status = bundleArchive_getLocation(archive_ptr, &name_str);
                } else if (show_symbolic_name) {
                    // do nothing
                } else if (show_update_location) {
                    sub_status = bundleArchive_getLocation(archive_ptr, &name_str);
                }
            }

            if (sub_status == CELIX_SUCCESS) {
		startColor = "";
		endColor = "";
                if (useColors) {
                    startColor = i % 2 == 0 ? EVEN_COLOR : ODD_COLOR;
                    endColor = END_COLOR;
                } 
		fprintf(out_ptr, "%s  %-5ld %-12s %s%s\n", startColor, id, state_str, name_str, endColor);

            }

            if (sub_status != CELIX_SUCCESS) {
                status = sub_status;
                break;
            }
        }

        arrayList_destroy(bundles_ptr);
    }

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
