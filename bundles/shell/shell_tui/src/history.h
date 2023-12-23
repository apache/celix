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

#ifndef SHELL_TUI_HISTORY
#define SHELL_TUI_HISTORY

#include "celix_bundle_context.h"

#define CELIX_SHELL_TUI_HIST_MAX 32

typedef struct celix_shell_tui_history celix_shell_tui_history_t;

celix_shell_tui_history_t* celix_shellTuiHistory_create(celix_bundle_context_t* ctx);
void celix_shellTuiHistory_destroy(celix_shell_tui_history_t* hist);
void celix_shellTuiHistory_addLine(celix_shell_tui_history_t* hist, const char* line);
const char* celix_shellTuiHistory_getPrevLine(celix_shell_tui_history_t* hist);
const char* celix_shellTuiHistory_getNextLine(celix_shell_tui_history_t* hist);
void celix_shellTuiHistory_lineReset(celix_shell_tui_history_t* hist);

#endif // SHELL_TUI_HISTORY
