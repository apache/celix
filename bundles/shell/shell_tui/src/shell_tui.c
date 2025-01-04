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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include "celix_array_list.h"
#include "celix_shell.h"
#include "celix_utils.h"
#include "shell_tui.h"
#include "history.h"

#define LINE_SIZE 256
#define PROMPT "-> "

#define KEY_ESC1		'\033'
#define KEY_ESC2		'['
#define KEY_BACKSPACE 	127
#define KEY_TAB			9
#define KEY_ENTER		'\n'
#define KEY_UP 			'A'
#define KEY_DOWN 		'B'
#define KEY_RIGHT		'C'
#define KEY_LEFT		'D'
#define KEY_DEL1		'3'
#define KEY_DEL2		'~'

const char * const SHELL_NOT_AVAILABLE_MSG = "[Shell TUI] Shell service not available.";

struct shell_tui {
    celix_bundle_context_t* ctx;

    celix_thread_mutex_t mutex; //protects shell
    celix_shell_t* shell;
    celix_thread_t thread;

    int readStopPipeFd;
    int writeStopPipeFd;

    int inputFd;
    FILE* output;
    FILE* error;

    bool useAnsiControlSequences;
};

typedef struct shell_context {
    char in[LINE_SIZE+1];
    char buffer[LINE_SIZE+1];
    char dline[LINE_SIZE+1];
    int pos;
    celix_shell_tui_history_t* hist;
} shell_context_t;

struct OriginalSettings {
    struct termios term_org;
    struct sigaction oldSigIntAction;
    struct sigaction oldSigSegvAction;
    struct sigaction oldSigAbrtAction;
    struct sigaction oldSigQuitAction;

};

// static function declarations
static void remove_newlines(char* line);
static void clearLine(shell_tui_t*);
static void cursorLeft(shell_tui_t*, int n);
static void writeLine(shell_tui_t*, const char*line, int pos);
static int autoComplete(shell_tui_t*, celix_shell_t* shellSvc, char *in, int cursorPos, size_t maxLen);
static void shellSigHandler(int sig, siginfo_t *info, void* ptr);
static void* shellTui_runnable(void *data);
static int shellTui_parseInput(shell_tui_t* shellTui, shell_context_t* ctx);
static int shellTui_parseInputForControl(shell_tui_t* shellTui, shell_context_t* ctx);
static int shellTui_parseInputPlain(shell_tui_t* shellTui, shell_context_t* ctx);
static void writePrompt(shell_tui_t*);

// Unfortunately has to be static, it is not possible to pass user defined data to the handler
static struct OriginalSettings originalSettings;

shell_tui_t* shellTui_create(celix_bundle_context_t* ctx, bool useAnsiControlSequences, int inputFd, int outputFd, int errorFd) {
    shell_tui_t* result = calloc(1, sizeof(*result));
    result->ctx = ctx;
    result->inputFd = inputFd;
    result->output = outputFd == STDOUT_FILENO ? stdout : fdopen(outputFd, "a");
    if (result->output == NULL) {
        fprintf(stderr, "Cannot open output fd %i for appending. Falling back to stdout\n", outputFd);
        result->output = stdout;
    }
    result->error = errorFd == STDERR_FILENO ? stderr : fdopen(errorFd, "a");
    if (result->error == NULL) {
        fprintf(stderr, "Cannot open error fd %i for appending. Falling back to stderr\n", errorFd);
        result->error = stderr;
    }
    result->useAnsiControlSequences = useAnsiControlSequences;
    celixThreadMutex_create(&result->mutex, NULL);
    return result;
}

celix_status_t shellTui_start(shell_tui_t* shellTui) {
    celix_status_t status = CELIX_SUCCESS;

    int fds[2];
    int rc  = pipe(fds);
    if (rc == 0) {
        shellTui->readStopPipeFd = fds[0];
        shellTui->writeStopPipeFd = fds[1];
        if (fcntl(shellTui->writeStopPipeFd, F_SETFL, O_NONBLOCK) == 0){
        	celixThread_create(&shellTui->thread, NULL, shellTui_runnable, shellTui);
        } else {
        	fprintf(shellTui->error,"[Shell TUI] fcntl on pipe failed");
        	status = CELIX_FILE_IO_EXCEPTION;
        }
    } else {
        fprintf(shellTui->error, "[Shell TUI] Cannot create pipe");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t shellTui_stop(shell_tui_t* shellTui) {
    write(shellTui->writeStopPipeFd, "", 1); //trigger select to stop
    celixThread_join(shellTui->thread, NULL);
    close(shellTui->writeStopPipeFd);
    close(shellTui->readStopPipeFd);
    if (shellTui->output != stdout) {
        fclose(shellTui->output);
    }
    if (shellTui->error != stderr) {
        fclose(shellTui->error);
    }
    return CELIX_SUCCESS;
}

void shellTui_destroy(shell_tui_t* shellTui) {
    celixThreadMutex_destroy(&shellTui->mutex);
    free(shellTui);
}

celix_status_t shellTui_setShell(shell_tui_t* shellTui, celix_shell_t* svc) {
    celixThreadMutex_lock(&shellTui->mutex);
    shellTui->shell = svc;
    celixThreadMutex_unlock(&shellTui->mutex);
    return CELIX_SUCCESS;
}

static void shellSigHandler(int sig, siginfo_t *info, void* ptr) {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings.term_org);
    if (sig == SIGINT) {
        originalSettings.oldSigIntAction.sa_sigaction(sig, info, ptr);
    } else if (sig == SIGSEGV){
        originalSettings.oldSigSegvAction.sa_sigaction(sig, info, ptr);
    } else if (sig == SIGABRT){
        originalSettings.oldSigAbrtAction.sa_sigaction(sig, info, ptr);
    } else if (sig == SIGQUIT){
        originalSettings.oldSigQuitAction.sa_sigaction(sig, info, ptr);
    }
}

static void* shellTui_runnable(void *data) {
    shell_tui_t* shellTui = (shell_tui_t*) data;

    //setup shell context
    shell_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hist = celix_shellTuiHistory_create(shellTui->ctx);

    struct termios term_new;
    if (shellTui->useAnsiControlSequences && shellTui->inputFd == STDIN_FILENO) {
        sigaction(SIGINT, NULL, &originalSettings.oldSigIntAction);
        sigaction(SIGSEGV, NULL, &originalSettings.oldSigSegvAction);
        sigaction(SIGABRT, NULL, &originalSettings.oldSigAbrtAction);
        sigaction(SIGQUIT, NULL, &originalSettings.oldSigQuitAction);
        struct sigaction newAction;
        memset(&newAction, 0, sizeof(newAction));
        newAction.sa_flags = SA_SIGINFO;
        newAction.sa_sigaction = shellSigHandler;
        sigaction(SIGINT, &newAction, NULL);
        sigaction(SIGSEGV, &newAction, NULL);
        sigaction(SIGABRT, &newAction, NULL);
        sigaction(SIGQUIT, &newAction, NULL);
        tcgetattr(STDIN_FILENO, &originalSettings.term_org);


        term_new = originalSettings.term_org;
        term_new.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &term_new);
    }

    //setup poll
    nfds_t nfds = 2;
    struct pollfd pollfds[2];
    pollfds[0].fd = shellTui->readStopPipeFd;
    pollfds[0].events = POLLIN;
    pollfds[1].fd = shellTui->inputFd;
    pollfds[1].events = POLLIN;

    bool printPrompt = true;
    for (;;) {
        if (printPrompt && shellTui->useAnsiControlSequences) {
            writeLine(shellTui, ctx.in, ctx.pos);
        } else if (printPrompt) {
            writePrompt(shellTui);
        }

        int rc = poll(pollfds, nfds, -1);
        if (rc > 0) {
            int nrOfCharsRead = 0;
            if (pollfds[0].revents & POLLIN) {
                break; //something is written to the stop pipe -> exit thread
            }
            if (pollfds[1].revents & POLLIN) {
                //something is written on the STDIN_FILENO fd
                nrOfCharsRead = shellTui_parseInput(shellTui, &ctx);
            }
            printPrompt = nrOfCharsRead > 0;
            if (shellTui->inputFd == STDIN_FILENO && !isatty(STDIN_FILENO)) {
                //not connected to a tty (anymore)
                //sleep for 1 sec to prevent 100% busy loop when a tty is removed.
                usleep(10000000);
            }
        } else {
            //error or (not configured timeout)
            fprintf(shellTui->error, "[Shell TUI] Error reading stdin: %s\n", strerror(errno));
            break;
        }
    }

    celix_shellTuiHistory_destroy(ctx.hist);
    if (shellTui->useAnsiControlSequences && shellTui->inputFd == STDIN_FILENO) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings.term_org);
        sigaction(SIGINT, &originalSettings.oldSigIntAction, NULL);
        sigaction(SIGSEGV, &originalSettings.oldSigSegvAction, NULL);
        sigaction(SIGABRT, &originalSettings.oldSigAbrtAction, NULL);
        sigaction(SIGQUIT, &originalSettings.oldSigQuitAction, NULL);
    }


    return NULL;
}

static int shellTui_parseInput(shell_tui_t* shellTui, shell_context_t* ctx) {
    if (shellTui->useAnsiControlSequences) {
        return shellTui_parseInputForControl(shellTui, ctx);
    } else {
        return shellTui_parseInputPlain(shellTui, ctx);
    }
}

static int shellTui_parseInputPlain(shell_tui_t* shellTui, shell_context_t* ctx) {
    char* buffer = ctx->buffer;
    char* in = ctx->in;
    int pos = ctx->pos;

    char* line = NULL;


    int nr_chars = read(shellTui->inputFd, buffer, LINE_SIZE-pos-1);
    for(int bufpos = 0; bufpos < nr_chars; bufpos++) {
        if (buffer[bufpos] == KEY_ENTER) { //end of line -> forward command
            line = celix_utils_trimInPlace(in);
            celixThreadMutex_lock(&shellTui->mutex);
            if (shellTui->shell != NULL) {
                shellTui->shell->executeCommand(shellTui->shell->handle, line, shellTui->output, shellTui->error);
            } else {
                fprintf(shellTui->output, "%s\n", SHELL_NOT_AVAILABLE_MSG);
            }
            celixThreadMutex_unlock(&shellTui->mutex);
            pos = 0;
            in[pos] = '\0';
        } else { //text
            in[pos] = buffer[bufpos];
            in[pos+1] = '\0';
            pos++;
            continue;
        }
    } // for
    ctx->pos = pos;
    return nr_chars;
}

static int shellTui_parseInputForControl(shell_tui_t* shellTui, shell_context_t* ctx) {
    char* buffer = ctx->buffer;
    char* in = ctx->in;
    char* dline = ctx->dline;
    celix_shell_tui_history_t* hist = ctx->hist;
    int pos = ctx->pos;
    const char* line = NULL;

    int nr_chars = (int)read(shellTui->inputFd, buffer, LINE_SIZE-pos-1);
    for(int bufpos = 0; bufpos < nr_chars; bufpos++) {
        if (buffer[bufpos] == KEY_ESC1 && buffer[bufpos+1] == KEY_ESC2) {
            switch (buffer[bufpos+2]) {
                case KEY_UP:
                    line = celix_shellTuiHistory_getPrevLine(hist);
                    if (line) {
                        strncpy(in, line, LINE_SIZE);
                        pos = (int)strlen(in);
                        writeLine(shellTui, line, pos);
                    }
                    break;
                case KEY_DOWN:
                    line = celix_shellTuiHistory_getNextLine(hist);
                    if (line) {
                        strncpy(in, line, LINE_SIZE);
                        pos = (int)strlen(in);
                        writeLine(shellTui, line, pos);
                    }
                    break;
                case KEY_RIGHT:
                    if (pos < strlen(in)) {
                        pos++;
                    }
                    writeLine(shellTui, in, pos);
                    break;
                case KEY_LEFT:
                    if (pos > 0) {
                        pos--;
                    }
                    writeLine(shellTui, in, pos);
                    break;
                case KEY_DEL1:
                    if(buffer[bufpos+3] == KEY_DEL2) {
                        bufpos++; // delete cmd takes 4 chars
                        int len = strlen(in);
                        if (pos < len) {
                            for (int i = pos; i <= len; i++) {
                                in[i] = in[i + 1];
                            }
                        }
                        writeLine(shellTui, in, pos);
                    }
                    break;
                default:
                    // Unsupported char, do nothing
                    break;
            }
            bufpos+=2;
            continue;
        } else if (buffer[bufpos] == KEY_BACKSPACE) { // backspace
            if(pos > 0) {
                int len = strlen(in);
                for(int i = pos-1; i <= len; i++) {
                    in[i] = in[i+1];
                }
                pos--;
            }
            writeLine(shellTui, in, pos);
            continue;
        } else if(buffer[bufpos] == KEY_TAB) {
            celixThreadMutex_lock(&shellTui->mutex);
            pos = autoComplete(shellTui, shellTui->shell, in, pos, LINE_SIZE);
            celixThreadMutex_unlock(&shellTui->mutex);
            continue;
        } else if (buffer[bufpos] != KEY_ENTER) { //not end of line -> text
            if (in[pos] == '\0') {
                in[pos+1] = '\0';
            }
            in[pos] = buffer[bufpos];
            pos++;
            writeLine(shellTui, in, pos);
            fflush(shellTui->output);
            continue;
        }

        //parse enter
        writeLine(shellTui, in, pos);
        fprintf(shellTui->output, "\n");
        remove_newlines(in);
        celix_shellTuiHistory_addLine(hist, in);

        memset(dline, 0, LINE_SIZE);
        strncpy(dline, in, LINE_SIZE);

        pos = 0;
        in[pos] = '\0';

        line = celix_utils_trimInPlace(dline);
        if ((strlen(line) == 0)) {
            continue;
        }
        celix_shellTuiHistory_lineReset(hist);
        celixThreadMutex_lock(&shellTui->mutex);
        if (shellTui->shell != NULL) {
            shellTui->shell->executeCommand(shellTui->shell->handle, line, shellTui->output, shellTui->error);
        } else {
            fprintf(shellTui->output, "%s\n", SHELL_NOT_AVAILABLE_MSG);
        }
        celixThreadMutex_unlock(&shellTui->mutex);
        break;
    } // for
    ctx->pos = pos;
    return nr_chars;
}

static void remove_newlines(char* line) {
    for(int i = 0; i < strlen(line); i++) {
        if(line[i] == '\n') {
            for(int j = 0; j < strlen(&line[i]); j++) {
                line[i+j] = line[i+j+1];
            }
        }
    }
}

static void clearLine(shell_tui_t* shellTui) {
	fprintf(shellTui->output, "\033[2K\r");
	fflush(shellTui->output);
}

static void cursorLeft(shell_tui_t* shellTui, int n) {
	if (n>0) {
		fprintf(shellTui->output, "\033[%dD", n);
	}
    fflush(shellTui->output);
}

static void writePrompt(shell_tui_t* shellTui) {
    fwrite(PROMPT, 1, strlen(PROMPT), shellTui->output);
    fflush(shellTui->output);
}

static void writeLine(shell_tui_t* shellTui, const char* line, int pos) {
    clearLine(shellTui);
    fwrite(PROMPT, 1, strlen(PROMPT), shellTui->output);
    fwrite(line, 1, strlen(line), shellTui->output);
    cursorLeft(shellTui, strlen(line)-pos);
}

/**
 * @brief Will check if there is a match with the input and the fully qualified cmd name or local cmd name.
 *
 * @return Return cmd or local cmd if there is a match with the input.
 */
static char* isFullQualifiedOrLocalMatch(char *cmd, char *in, int cursorPos) {
    char* matchCmd = NULL;
    if (strncmp(in, cmd, cursorPos) == 0) {
        matchCmd = cmd;
    } else {
        char* namespaceFound = strstr(cmd, "::");
        if (namespaceFound != NULL) {
            //got a command with a namespace, strip namespace for a possible match. E.g celix::lb -> lb
            char *localCmd = namespaceFound + 2; //note :: is 2 char, so forward 2 chars
            if (strncmp(in, localCmd, cursorPos) == 0) {
                matchCmd = localCmd;
            }
        }
    }
    return matchCmd;
}

static int autoComplete(shell_tui_t* shellTui, celix_shell_t* shellSvc, char *in, int cursorPos, size_t maxLen) {
	celix_array_list_t* commandList = NULL;
    celix_array_list_t* possibleCmdList = NULL;
	shellSvc->getCommands(shellSvc->handle, &commandList);
	int nrCmds = celix_arrayList_size(commandList);
    possibleCmdList = celix_arrayList_createPointerArray();

	for (int i = 0; i < nrCmds; i++) {
		char *cmd = celix_arrayList_get(commandList, i);
        char *match = isFullQualifiedOrLocalMatch(cmd, in, cursorPos);
        if (match != NULL) {
            celix_arrayList_add(possibleCmdList, match);
        }
	}

	int nrPossibleCmds = celix_arrayList_size(possibleCmdList);
	if (nrPossibleCmds == 0) {
		// Check if complete command with space is entered: show usage if this is the case
		if(in[strlen(in) - 1] == ' ') {
			for (int i = 0; i < nrCmds; i++) {
				char *cmd = celix_arrayList_get(commandList, i);
                char *match = isFullQualifiedOrLocalMatch(cmd, in, strlen(in) - 1);
                if (match != NULL) {
                    clearLine(shellTui);
                    char* usage = NULL;
                    shellSvc->getCommandUsage(shellSvc->handle, cmd, &usage);
                    fprintf(shellTui->output, "Usage:\n %s\n", usage);
                    free(usage);
                }
			}
		}
	} else if (nrPossibleCmds == 1) {
		//Replace input string with the only possibility
		snprintf(in, maxLen, "%s ", (char*)celix_arrayList_get(possibleCmdList, 0));
		cursorPos = strlen(in);
	} else {
		// Show possibilities
		clearLine(shellTui);
		for(int i = 0; i < nrPossibleCmds; i++) {
			fprintf(shellTui->output,"%s ", (char*)celix_arrayList_get(possibleCmdList, i));
		}
        fprintf(shellTui->output,"\n");
	}

    for (int i = 0; i < celix_arrayList_size(commandList); ++i) {
        char* cmd = celix_arrayList_get(commandList, i);
        free(cmd);
    }
    celix_arrayList_destroy(commandList);
    celix_arrayList_destroy(possibleCmdList);
	return cursorPos;
}

