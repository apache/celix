/*
 * shell_private.h
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include "headers.h"
#include "shell.h"
#include "hash_map.h"
#include "command.h"

struct shell {
	BUNDLE_CONTEXT bundleContext;
	HASH_MAP commandReferenceMap;
	HASH_MAP commandNameMap;
};

SHELL shell_create();
char * shell_getCommandUsage(SHELL shell, char * commandName);
char * shell_getCommandDescription(SHELL shell, char * commandName);
SERVICE_REFERENCE shell_getCommandReference(SHELL shell, char * command);
void shell_executeCommand(SHELL shell, char * commandLine, void (*out)(char *), void (*error)(char *));

COMMAND shell_getCommand(SHELL shell, char * commandName);


#endif /* SHELL_PRIVATE_H_ */
