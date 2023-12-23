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

#ifndef SHELL_TUI_H_
#define SHELL_TUI_H_

#include <stdlib.h>

#include "celix_bundle_context.h"
#include "celix_shell.h"
#include "celix_threads.h"

typedef struct shell_tui shell_tui_t ;


/**
 * @brief Create a new shell tui.
 * @param useAnsiControlSequences Whether to parse ansi control sequences.
 * @param inputFd The input file descriptor to use.
 * @param outputFd The output file descriptor to use.
 * @param errorFd The error output file descriptor to use.
 */
shell_tui_t*
shellTui_create(celix_bundle_context_t* ctx, bool useAnsiControlSequences, int inputFd, int outputFd, int errorFd);

/**
 * @brief Start the shell tui and the thread reading the tty and optional extra read file descriptor.
 */
celix_status_t shellTui_start(shell_tui_t* shellTui);

/**
 * @brief Stop the shell tui.
 */
celix_status_t shellTui_stop(shell_tui_t* shellTui);

/**
 * @brief Free the resources for the shell tui
 */
void shellTui_destroy(shell_tui_t* shellTui);

/**
 * @brief set the shell service.
 */
celix_status_t shellTui_setShell(shell_tui_t* shellTui, celix_shell_t* svc);

#endif /* SHELL_TUI_H_ */
