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
 * start_command.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "command_impl.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"
#include "shell.h"

void helpCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));

command_pt helpCommand_create(bundle_context_pt context) {
	command_pt command = (command_pt) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "help";
	command->shortDescription = "display available command usage and description.";
	command->usage = "start [<command> ...]";
	command->executeCommand = helpCommand_execute;
	return command;
}

void helpCommand_destroy(command_pt command) {
	free(command);
}


void helpCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *)) {
	service_reference_pt shellService = NULL;
	bundleContext_getServiceReference(command->bundleContext, (char *) OSGI_SHELL_SERVICE_NAME, &shellService);

	if (shellService != NULL) {
		shell_service_pt shell = NULL;
		bundleContext_getService(command->bundleContext, shellService, (void **) &shell);

		if (shell != NULL) {
			char delims[] = " ";
			char * sub = NULL;
			char outString[256];

			sub = strtok(line, delims);
			sub = strtok(NULL, delims);

			if (sub == NULL) {
				int i;
				array_list_pt commands = shell->getCommands(shell->shell);
				for (i = 0; i < arrayList_size(commands); i++) {
					char *name = arrayList_get(commands, i);
					sprintf(outString, "%s\n", name);
					out(outString);
				}
				out("\nUse 'help <command-name>' for more information.\n");
			} else {
				bool found = false;
				while (sub != NULL) {
					int i;
					array_list_pt commands = shell->getCommands(shell->shell);
					for (i = 0; i < arrayList_size(commands); i++) {
						char *name = arrayList_get(commands, i);
						if (strcmp(sub, name) == 0) {
							char *desc = shell->getCommandDescription(shell->shell, name);
							char *usage = shell->getCommandUsage(shell->shell, name);

							if (found) {
								out("---\n");
							}
							found = true;
							sprintf(outString, "Command     : %s\n", name);
							out(outString);
							sprintf(outString, "Usage       : %s\n", usage);
							out(outString);
							sprintf(outString, "Description : %s\n", desc);
							out(outString);
						}
					}
					sub = strtok(NULL, delims);
				}
			}

		}
	}

//	char delims[] = " ";
//	char * sub = NULL;
//	char outString[256];
//	sub = strtok(line, delims);
//	sub = strtok(NULL, delims);
//	if (sub == NULL) {
//		err("Incorrect number of arguments.\n");
//		sprintf(outString, "%s\n", command->usage);
//		out(outString);
//	} else {
//		while (sub != NULL) {
//			long id = atol(sub);
//			bundle_pt bundle = NULL;
//			bundleContext_getBundleById(command->bundleContext, id, &bundle);
//			if (bundle != NULL) {
//				bundle_startWithOptions(bundle, 0);
//			} else {
//				err("Bundle id is invalid.\n");
//			}
//			sub = strtok(NULL, delims);
//		}
//	}
}
