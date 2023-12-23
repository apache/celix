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

#include "celix_array_list.h"
#include "celix_bundle_context.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "history.h"

struct celix_shell_tui_history {
    celix_bundle_context_t* ctx;
    celix_array_list_t* historyLines;
    int currentLine;
};

celix_shell_tui_history_t* celix_shellTuiHistory_create(celix_bundle_context_t* ctx) {
    celix_autofree celix_shell_tui_history_t* hist = calloc(1, sizeof(*hist));
    if (hist) {
        hist->ctx = ctx;
        celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
        opts.simpleRemovedCallback = free;
        hist->historyLines = celix_arrayList_createWithOptions(&opts);
        hist->currentLine = -1;
    }

    if (!hist || !hist->historyLines) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Error creating history");
        return NULL;
    }

    return celix_steal_ptr(hist);
}

void celix_shellTuiHistory_destroy(celix_shell_tui_history_t* hist) {
    if (hist) {
        celix_arrayList_destroy(hist->historyLines);
        free(hist);
    }
}

void celix_shellTuiHistory_addLine(celix_shell_tui_history_t* hist, const char* line) {
    if (line == NULL || strlen(line) == 0) {
        return; //ignore empty lines
    }
    celix_autofree char* lineCopy = celix_utils_strdup(line);
    celix_status_t status = CELIX_SUCCESS;
    if (lineCopy) {
        status = celix_arrayList_add(hist->historyLines, lineCopy);
        if (celix_arrayList_size(hist->historyLines) > CELIX_SHELL_TUI_HIST_MAX) {
            celix_arrayList_removeAt(hist->historyLines, 0);
        }
    }

    if (!lineCopy || status != CELIX_SUCCESS) {
        celix_bundleContext_log(hist->ctx, CELIX_LOG_LEVEL_ERROR, "Error adding line to history");
    } else {
        hist->currentLine = celix_arrayList_size(hist->historyLines);
        celix_steal_ptr(lineCopy);
    }
}

const char* celix_shellTuiHistory_getPrevLine(celix_shell_tui_history_t* hist) {
    hist->currentLine--;
    if (hist->currentLine < 0) {
        hist->currentLine = 0;
    }
    int size = celix_arrayList_size(hist->historyLines);
    if (size > 0) {
        return celix_arrayList_get(hist->historyLines, hist->currentLine);
    }
    return NULL;
}

const char* celix_shellTuiHistory_getNextLine(celix_shell_tui_history_t* hist) {
    hist->currentLine++;
    int size = celix_arrayList_size(hist->historyLines);
    if (hist->currentLine >= size) {
        hist->currentLine = size - 1;
    }
    if (size > 0) {
        return celix_arrayList_get(hist->historyLines, hist->currentLine);
    }
    return NULL;
}

void celix_shellTuiHistory_lineReset(celix_shell_tui_history_t* hist) {
    hist->currentLine = celix_arrayList_size(hist->historyLines);
}
