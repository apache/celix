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
#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "bundle_context.h"
#include "shell.h"
#include "shell_tui.h"
#include "utils.h"
#include <signal.h>
#include <fcntl.h>
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

struct shell_tui {
    celix_thread_mutex_t mutex; //protects shell
    shell_service_t* shell;
    celix_thread_t thread;

    int readPipeFd;
    int writePipeFd;

    bool useAnsiControlSequences;
};

typedef struct shell_context {
    char in[LINE_SIZE+1];
    char buffer[LINE_SIZE+1];
    char dline[LINE_SIZE+1];
    int pos;
    history_t* hist;
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
static void clearLine();
static void cursorLeft(int n);
static void writeLine(const char*line, int pos);
static int autoComplete(shell_service_pt shellSvc, char *in, int cursorPos, size_t maxLen);
static void shellSigHandler(int sig, siginfo_t *info, void* ptr);
static void* shellTui_runnable(void *data);
static void shellTui_parseInputForControl(shell_tui_t* shellTui, shell_context_t* ctx);
static void shellTui_parseInput(shell_tui_t* shellTui, shell_context_t* ctx);
static void writePrompt(void);

// Unfortunately has to be static, it is not possible to pass user defined data to the handler
static struct OriginalSettings originalSettings;

shell_tui_t* shellTui_create(bool useAnsiControlSequences) {
    shell_tui_t* result = calloc(1, sizeof(*result));
    if (result != NULL) {
        result->useAnsiControlSequences = useAnsiControlSequences;
        celixThreadMutex_create(&result->mutex, NULL);
    }
    return result;
}

celix_status_t shellTui_start(shell_tui_t* shellTui) {
    celix_status_t status = CELIX_SUCCESS;

    int fds[2];
    int rc  = pipe(fds);
    if (rc == 0) {
        shellTui->readPipeFd = fds[0];
        shellTui->writePipeFd = fds[1];
        if(fcntl(shellTui->writePipeFd, F_SETFL, O_NONBLOCK) == 0){
        	celixThread_create(&shellTui->thread, NULL, shellTui_runnable, shellTui);
        }
        else{
        	fprintf(stderr,"fcntl on pipe failed");
        	status = CELIX_FILE_IO_EXCEPTION;
        }
    } else {
        fprintf(stderr, "Cannot create pipe");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t shellTui_stop(shell_tui_t* shellTui) {
    celix_status_t status = CELIX_SUCCESS;
    write(shellTui->writePipeFd, "\0", 1); //trigger select to stop
    celixThread_join(shellTui->thread, NULL);
    close(shellTui->writePipeFd);
    close(shellTui->readPipeFd);
    return status;
}

void shellTui_destroy(shell_tui_t* shellTui) {
    if (shellTui == NULL) return;

    celixThreadMutex_destroy(&shellTui->mutex);
    free(shellTui);
}

celix_status_t shellTui_setShell(shell_tui_t* shellTui, shell_service_t* svc) {
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
    ctx.hist = historyCreate();

    struct termios term_new;
    if (shellTui->useAnsiControlSequences) {
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

    //setup file descriptors
    fd_set rfds;
    int nfds = shellTui->writePipeFd > STDIN_FILENO ? (shellTui->writePipeFd +1) : (STDIN_FILENO + 1);

    for (;;) {
        if (shellTui->useAnsiControlSequences) {
            writeLine(ctx.in, ctx.pos);
        } else {
            writePrompt();
        }
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(shellTui->readPipeFd, &rfds);

        if (select(nfds, &rfds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(shellTui->readPipeFd, &rfds)) {
                break; //something is written to the pipe -> exit thread
            } else if (FD_ISSET(STDIN_FILENO, &rfds)) {
                if (shellTui->useAnsiControlSequences) {
                    shellTui_parseInputForControl(shellTui, &ctx);
                } else {
                    shellTui_parseInput(shellTui, &ctx);
                }
            }
        }
    }

    historyDestroy(ctx.hist);
    if (shellTui->useAnsiControlSequences) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings.term_org);
        sigaction(SIGINT, &originalSettings.oldSigIntAction, NULL);
        sigaction(SIGSEGV, &originalSettings.oldSigSegvAction, NULL);
        sigaction(SIGABRT, &originalSettings.oldSigAbrtAction, NULL);
        sigaction(SIGQUIT, &originalSettings.oldSigQuitAction, NULL);
    }


    return NULL;
}

static void shellTui_parseInput(shell_tui_t* shellTui, shell_context_t* ctx) {
    char* buffer = ctx->buffer;
    char* in = ctx->in;
    int pos = ctx->pos;

    char* line = NULL;


    int nr_chars = read(STDIN_FILENO, buffer, LINE_SIZE-pos-1);
    for(int bufpos = 0; bufpos < nr_chars; bufpos++) {
        if (buffer[bufpos] == KEY_ENTER) { //end of line -> forward command
            line = utils_stringTrim(in);
            celixThreadMutex_lock(&shellTui->mutex);
            if (shellTui->shell != NULL) {
                printf("Providing command '%s' from in '%s'\n", line, in);
                shellTui->shell->executeCommand(shellTui->shell->shell, line, stdout, stderr);
            } else {
                fprintf(stderr, "Shell service not available\n");
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
}

static void shellTui_parseInputForControl(shell_tui_t* shellTui, shell_context_t* ctx) {
    char* buffer = ctx->buffer;
    char* in = ctx->in;
    char* dline = ctx->dline;
    history_t* hist = ctx->hist;
    int pos = ctx->pos;

    char* line = NULL;

    if (shellTui == NULL) {
    	return;
    }

    int nr_chars = read(STDIN_FILENO, buffer, LINE_SIZE-pos-1);
    for(int bufpos = 0; bufpos < nr_chars; bufpos++) {
        if (buffer[bufpos] == KEY_ESC1 && buffer[bufpos+1] == KEY_ESC2) {
            switch (buffer[bufpos+2]) {
                case KEY_UP:
                    if(historySize(hist) > 0) {
                        strncpy(in, historyGetPrevLine(hist), LINE_SIZE);
                        pos = strlen(in);
                        writeLine(in, pos);
                    }
                    break;
                case KEY_DOWN:
                    if(historySize(hist) > 0) {
                        strncpy(in, historyGetNextLine(hist), LINE_SIZE);
                        pos = strlen(in);
                        writeLine(in, pos);
                    }
                    break;
                case KEY_RIGHT:
                    if (pos < strlen(in)) {
                        pos++;
                    }
                    writeLine(in, pos);
                    break;
                case KEY_LEFT:
                    if (pos > 0) {
                        pos--;
                    }
                    writeLine(in, pos);
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
                        writeLine(in, pos);
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
            writeLine(in, pos);
            continue;
        } else if(buffer[bufpos] == KEY_TAB) {
            celixThreadMutex_lock(&shellTui->mutex);
            pos = autoComplete(shellTui->shell, in, pos, LINE_SIZE);
            celixThreadMutex_unlock(&shellTui->mutex);
            continue;
        } else if (buffer[bufpos] != KEY_ENTER) { //not end of line -> text
            if (in[pos] == '\0') {
                in[pos+1] = '\0';
            }
            in[pos] = buffer[bufpos];
            pos++;
            writeLine(in, pos);
            fflush(stdout);
            continue;
        }

        //parse enter
        writeLine(in, pos);
        write(STDOUT_FILENO, "\n", 1);
        remove_newlines(in);
        history_addLine(hist, in);

        memset(dline, 0, LINE_SIZE);
        strncpy(dline, in, LINE_SIZE);

        pos = 0;
        in[pos] = '\0';

        line = utils_stringTrim(dline);
        if ((strlen(line) == 0)) {
            continue;
        }
        historyLineReset(hist);
        celixThreadMutex_lock(&shellTui->mutex);
        if (shellTui->shell != NULL) {
            shellTui->shell->executeCommand(shellTui->shell->shell, line, stdout, stderr);
            pos = 0;
            nr_chars = 0;
        } else {
            fprintf(stderr, "Shell service not available\n");
        }
        celixThreadMutex_unlock(&shellTui->mutex);
    } // for
    ctx->pos = pos;
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

static void clearLine() {
	printf("\033[2K\r");
	fflush(stdout);
}

static void cursorLeft(int n) {
	if(n>0) {
		printf("\033[%dD", n);
		fflush(stdout);
	}
}

static void writePrompt(void) {
    write(STDIN_FILENO, PROMPT, strlen(PROMPT));
}

static void writeLine(const char* line, int pos) {
    clearLine();
    write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
    write(STDOUT_FILENO, line, strlen(line));
	cursorLeft(strlen(line)-pos);
}

static int autoComplete(shell_service_t* shellSvc, char *in, int cursorPos, size_t maxLen) {
	array_list_pt commandList = NULL;
	array_list_pt possibleCmdList = NULL;
	shellSvc->getCommands(shellSvc->shell, &commandList);
	int nrCmds = arrayList_size(commandList);
	arrayList_create(&possibleCmdList);

	for (int i = 0; i < nrCmds; i++) {
		char *cmd = arrayList_get(commandList, i);
		if (strncmp(in, cmd, cursorPos) == 0) {
			arrayList_add(possibleCmdList, cmd);
		}
	}

	int nrPossibleCmds = arrayList_size(possibleCmdList);
	if (nrPossibleCmds == 0) {
		// Check if complete command with space is entered: show usage if this is the case
		if(in[strlen(in) - 1] == ' ') {
			for (int i = 0; i < nrCmds; i++) {
				char *cmd = arrayList_get(commandList, i);
				if (strncmp(in, cmd, strlen(cmd)) == 0) {
					clearLine();
					char* usage = NULL;
					shellSvc->getCommandUsage(shellSvc->shell, cmd, &usage);
					printf("Usage:\n %s\n", usage);
				}
			}
		}
	} else if (nrPossibleCmds == 1) {
		//Replace input string with the only possibility
		snprintf(in, maxLen, "%s ", (char*)arrayList_get(possibleCmdList, 0));
		cursorPos = strlen(in);
	} else {
		// Show possibilities
		clearLine();
		for(int i = 0; i < nrPossibleCmds; i++) {
			printf("%s ", (char*)arrayList_get(possibleCmdList, i));
		}
		printf("\n");
	}
	arrayList_destroy(commandList);
	arrayList_destroy(possibleCmdList);
	return cursorPos;
}

