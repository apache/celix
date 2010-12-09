/*
 * shell.h
 *
 *  Created on: Aug 12, 2010
 *      Author: alexanderb
 */

#ifndef SHELL_H_
#define SHELL_H_

#include "headers.h"
#include "array_list.h"

static const char * const SHELL_SERVICE_NAME = "shellService";

typedef struct shell * SHELL;

struct shellService {
	SHELL shell;
	ARRAY_LIST (*getCommands)(SHELL shell);
	char * (*getCommandUsage)(SHELL shell, char * commandName);
	char * (*getCommandDescription)(SHELL shell, char * commandName);
	SERVICE_REFERENCE (*getCommandReference)(SHELL shell, char * command);
	void (*executeCommand)(SHELL shell, char * commandLine, void (*out)(char *), void (*error)(char *));
};

typedef struct shellService * SHELL_SERVICE;

#endif /* SHELL_H_ */
