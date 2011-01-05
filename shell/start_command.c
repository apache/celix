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
 *  Created on: Aug 20, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "command_private.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"

void startCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *));

COMMAND startCommand_create(BUNDLE_CONTEXT context) {
	COMMAND command = (COMMAND) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "start";
	command->shortDescription = "start bundle(s).";
	command->usage = "start <id> [<id> ...]";
	command->executeCommand = startCommand_execute;
	return command;
}


void startCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *)) {
	char delims[] = " ";
	char * sub = NULL;
	sub = strtok(line, delims);
	sub = strtok(NULL, delims);
	while (sub != NULL) {
		long id = atol(sub);
		BUNDLE bundle = bundleContext_getBundleById(command->bundleContext, id);
		if (bundle != NULL) {
			startBundle(bundle, 0);
		} else {
			err("Bundle id is invalid.");
		}
		sub = strtok(NULL, delims);
	}
}
