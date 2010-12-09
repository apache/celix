/*
 * command.h
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#ifndef COMMAND_H_
#define COMMAND_H_

static const char * const COMMAND_SERVICE_NAME = "commandService";

typedef struct command * COMMAND;

struct commandService {
	COMMAND command;
	char * (*getName)(COMMAND command);
	char * (*getUsage)(COMMAND command);
	char * (*getShortDescription)(COMMAND command);
	void (*executeCommand)(COMMAND command, char * commandLine, void (*out)(char *), void (*error)(char *));
};




#endif /* COMMAND_H_ */
