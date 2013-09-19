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
 * stop_command.c
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
#include "utils.h"

void stopCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));

command_pt stopCommand_create(bundle_context_pt context) {
	command_pt command = (command_pt) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "stop";
	command->shortDescription = "stop bundle(s).";
	command->usage = "start <id> [<id> ...]";
	command->executeCommand = stopCommand_execute;
	return command;
}

void stopCommand_destroy(command_pt command) {
	free(command);
}

void stopCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *)) {
    char delims[] = " ";
	char * sub = NULL;
	char outString[256];

	sub = strtok(line, delims);
	sub = strtok(NULL, delims);

	if (sub == NULL) {
		err("Incorrect number of arguments.\n");
		sprintf(outString, "%s\n", command->usage);
		out(outString);
	} else {
		while (sub != NULL) {
			bool numeric;
			utils_isNumeric(sub, &numeric);
			if (numeric) {
				long id = atol(sub);
				bundle_pt bundle = NULL;
				bundleContext_getBundleById(command->bundleContext, id, &bundle);
				if (bundle != NULL) {
					bundle_stopWithOptions(bundle, 0);
				} else {
					err("Bundle id is invalid.");
				}
			} else {
				err("Bundle id should be a number (bundle id).\n");
			}
			sub = strtok(NULL, delims);
		}
	}
}
