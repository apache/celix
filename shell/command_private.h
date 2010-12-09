/*
 * command_private.h
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#ifndef COMMAND_PRIVATE_H_
#define COMMAND_PRIVATE_H_

#include "command.h"
#include "headers.h"

struct command {
	char * name;
	char * usage;
	char * shortDescription;

	BUNDLE_CONTEXT bundleContext;

	void (*executeCommand)(COMMAND command, char * commandLine, void (*out)(char *), void (*error)(char *));
};


#endif /* COMMAND_PRIVATE_H_ */
