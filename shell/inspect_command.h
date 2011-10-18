/*
 * inspect_command.h
 *
 *  Created on: Oct 13, 2011
 *      Author: alexander
 */

#ifndef INSPECT_COMMAND_H_
#define INSPECT_COMMAND_H_

COMMAND inspectCommand_create(BUNDLE_CONTEXT context);
void inspectCommand_destroy(COMMAND command);

#endif /* INSPECT_COMMAND_H_ */
