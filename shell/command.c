/*
 * command.c
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#include <stdio.h>

#include "command_private.h"

char * command_getName(COMMAND command) {
	return command->name;
}

char * command_getUsage(COMMAND command) {
	return command->usage;
}

char * command_getShortDescription(COMMAND command) {
	return command->shortDescription;
}

void command_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *)) {
	command->executeCommand(command, line, out, err);
}
