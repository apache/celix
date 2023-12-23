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

#include <unistd.h>
#include "celix_bundle_activator.h"
#include "celix_shell.h"
#include "shell_tui.h"

/**
 * Whether to use ANSI control sequences to support backspace, left, up, etc key commands in the
 * shell tui. Default is true if a TERM environment is set else false.
 */
#define SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES "SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES"

/**
 * The file descriptor to use as input. Default is `STDIN_FILENO`.
 * This is used for testing.
 */
#define SHELL_TUI_INPUT_FILE_DESCRIPTOR "SHELL_TUI_INPUT_FILE_DESCRIPTOR"

/**
 * The file descriptor to use as output. Default is `STDOUT_FILENO`.
 * This is used for testing.
 */
#define SHELL_TUI_OUTPUT_FILE_DESCRIPTOR "SHELL_TUI_OUTPUT_FILE_DESCRIPTOR"

/**
 * The file descriptor to use as error output. Default is `STDERR_FILENO`.
 * This is used for testing.
 */
#define SHELL_TUI_ERROR_FILE_DESCRIPTOR "SHELL_TUI_ERROR_FILE_DESCRIPTOR"

typedef struct celix_shell_tui_activator {
    shell_tui_t* shellTui;
    long trackerId;
} celix_shell_tui_activator_t;


static celix_status_t celix_shellTuiActivator_start(celix_shell_tui_activator_t* act, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;
    act->trackerId = -1L;

    int inputFd = (int)celix_bundleContext_getPropertyAsLong(ctx, SHELL_TUI_INPUT_FILE_DESCRIPTOR, STDIN_FILENO);
    int outputFd = (int)celix_bundleContext_getPropertyAsLong(ctx, SHELL_TUI_OUTPUT_FILE_DESCRIPTOR, STDOUT_FILENO);
    int errorFd = (int)celix_bundleContext_getPropertyAsLong(ctx, SHELL_TUI_ERROR_FILE_DESCRIPTOR, STDERR_FILENO);

    //Check if tty exists, no tty -> no shell (Expect - for testing - if the input fd is not STDIN_FILENO).
    if (inputFd == STDIN_FILENO && !isatty(STDIN_FILENO)) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_INFO, "[Shell TUI] no tty connected. Shell TUI will not activate.");
        return status;
    }

    bool useCommands = false;
    char *term = getenv("TERM");
    useCommands = term != NULL; //if TERM exist, default is to use commands
    useCommands = celix_bundleContext_getPropertyAsBool(ctx, SHELL_TUI_USE_ANSI_CONTROL_SEQUENCES, useCommands);
    act->shellTui = shellTui_create(ctx, useCommands, inputFd, outputFd, errorFd);

    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = CELIX_SHELL_SERVICE_NAME;
        opts.callbackHandle = act->shellTui;
        opts.set = (void*)shellTui_setShell;
        act->trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    status = shellTui_start(act->shellTui);
    if (status != CELIX_SUCCESS) {
        shellTui_destroy(act->shellTui);
        act->shellTui = NULL;
    }

    return status;
}

static celix_status_t celix_shellTuiActivator_stop(celix_shell_tui_activator_t* act, celix_bundle_context_t* ctx) {
    celix_bundleContext_stopTracker(ctx, act->trackerId);
    if (act->shellTui != NULL) {
        shellTui_stop(act->shellTui);
        shellTui_destroy(act->shellTui);
    }
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_shell_tui_activator_t, celix_shellTuiActivator_start, celix_shellTuiActivator_stop)
