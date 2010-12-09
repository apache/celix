/*
 * stop_command.c
 *
 *  Created on: Aug 20, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "command_private.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"

void stopCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *));

COMMAND stopCommand_create(BUNDLE_CONTEXT context) {
	COMMAND command = (COMMAND) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "stop";
	command->shortDescription = "stop bundle(s).";
	command->usage = "start <id> [<id> ...]";
	command->executeCommand = stopCommand_execute;
	return command;
}


void stopCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *)) {
	char delims[] = " ";
	char * sub = NULL;
	sub = strtok(line, delims);
	sub = strtok(NULL, delims);
	while (sub != NULL) {
		long id = atol(sub);
		BUNDLE bundle = bundleContext_getBundleById(command->bundleContext, id);
		if (bundle != NULL) {
			stopBundle(bundle, 0);
		} else {
			err("Bundle id is invalid.");
		}
		sub = strtok(NULL, delims);
	}
}
