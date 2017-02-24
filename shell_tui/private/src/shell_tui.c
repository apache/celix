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
 * shell_tui.c
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

#include "bundle_context.h"
#include "bundle_activator.h"
#include "shell.h"
#include "shell_tui.h"
#include "utils.h"
#include <signal.h>
#include <unistd.h>
#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define PROMPT "-> "

#ifdef USE_READLINE
static shell_tui_activator_pt act = NULL;
#endif

#ifdef USE_READLINE
void rl_callback_handler(char *dline) {
    char *line;
    line = utils_stringTrim(dline);
    if (dline == NULL || *dline == 0 || act == NULL || line == NULL || (strlen(line) == 0) || (act->shell == NULL)) {
        return;
    }
    add_history(dline);
    celixThreadMutex_lock(&act->mutex);
    if (act->shell != NULL) {
        act->shell->executeCommand(act->shell->shell, line, stdout, stderr);
    } else {
        fprintf(stderr, "Shell service not available\n");
    }
    celixThreadMutex_unlock(&act->mutex);
}

char ** tab_completion(const char *text, int start, int end) {
    array_list_pt list ;
    act->shell->getCommands(act->shell->shell, &list);
    char **a;
    int size = arrayList_size(list);
    a = calloc(sizeof(char*),  size + 2); // 1 extra for string to be used and one for terminating NULL
    a[0] = strdup("");
    unsigned int j = 1;
    for(unsigned int i = 0; i < size; i++) {
        if(text && strlen(text) > 0) {
            if(strncmp(arrayList_get(list, i), text, strlen(text)) == 0) {
                a[j++] = strdup(arrayList_get(list, i));
             }
        } else {
            a[j++] = strdup(arrayList_get(list, i));
        }
    }
    // In case of one option, use that option
    if(j == 2) {
        free(a[0]);
        a[0] = a[1];
        a[1] = 0;
    }
    arrayList_destroy(list);
    return a;
}

#endif

static void* shellTui_runnable(void *data) {
#ifndef USE_READLINE
    shell_tui_activator_pt act = (shell_tui_activator_pt) data;
#else
    act = (shell_tui_activator_pt) data;
#endif

#ifndef USE_READLINE
    char in[256];
    char dline[256];
    bool needPrompt = true;
#endif

    fd_set rfds;
    struct timeval tv;
#ifdef USE_READLINE
    rl_callback_handler_install(PROMPT, rl_callback_handler);
    rl_attempted_completion_function = tab_completion;
#endif
    while (act->running) {
#ifndef USE_READLINE        
        char * line = NULL;
        if (needPrompt) {
            printf(PROMPT);
            fflush(stdout);
            needPrompt = false;
        }
#endif
   
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        if (select(1, &rfds, NULL, NULL, &tv) > 0) {
#ifdef USE_READLINE
            rl_callback_read_char();
#else
            fgets(in, sizeof(in)-1, stdin);
            needPrompt = true;
            memset(dline, 0, 256);
            strncpy(dline, in, 256);

            line = utils_stringTrim(dline);
            if ((strlen(line) == 0) || (act->shell == NULL)) {
                continue;
            }

            celixThreadMutex_lock(&act->mutex);
            if (act->shell != NULL) {
                act->shell->executeCommand(act->shell->shell, line, stdout, stderr);
            } else {
                fprintf(stderr, "Shell service not available\n");
            }
            celixThreadMutex_unlock(&act->mutex);
#endif
        }
    }
#ifdef USE_READLINE
    rl_callback_handler_remove();
#endif
    return NULL;
}

celix_status_t shellTui_start(shell_tui_activator_pt activator) {

    celix_status_t status = CELIX_SUCCESS;

    activator->running = true;
    celixThread_create(&activator->runnable, NULL, shellTui_runnable, activator);

    return status;
}

celix_status_t shellTui_stop(shell_tui_activator_pt act) {
    celix_status_t status = CELIX_SUCCESS;
    act->running = false;
    celixThread_kill(act->runnable, SIGUSR1);
    celixThread_join(act->runnable, NULL);
    return status;
}

