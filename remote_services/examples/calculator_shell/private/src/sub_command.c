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
 * sub_command.c
 *
 *  \date       Oct 13, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"
#include "sub_command.h"
#include "calculator_service.h"


void subCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));
celix_status_t subCommand_isNumeric(command_pt command, char *number, bool *ret);

command_pt subCommand_create(bundle_context_pt context) {
    command_pt command = (command_pt) calloc(1, sizeof(*command));
    if (command) {
		command->bundleContext = context;
		command->name = "sub";
		command->shortDescription = "subtract the given doubles";
		command->usage = "sub <double> <double>";
		command->executeCommand = subCommand_execute;
    }
    return command;
}

void subCommand_destroy(command_pt command) {
	free(command);
}

void subCommand_execute(command_pt command, char *line, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
    service_reference_pt calculatorService = NULL;

    status = bundleContext_getServiceReference(command->bundleContext, (char *) CALCULATOR_SERVICE, &calculatorService);
    if (status == CELIX_SUCCESS) {
    	char *token = line;
    	strtok_r(line, " ", &token);
		char *aStr = strtok_r(NULL, " ", &token);
		bool numeric;
		subCommand_isNumeric(command, aStr, &numeric);
		if (aStr != NULL && numeric) {
			char *bStr = strtok_r(NULL, " ", &token);
			subCommand_isNumeric(command, bStr, &numeric);
			if (bStr != NULL && numeric) {
				calculator_service_pt calculator = NULL;
				status = bundleContext_getService(command->bundleContext, calculatorService, (void *) &calculator);
				if (status == CELIX_SUCCESS) {
					double a = atof(aStr);
					double b = atof(bStr);
					double result = 0;
					status = calculator->sub(calculator->calculator, a, b, &result);
					if (status == CELIX_SUCCESS) {
						char line[256];
						sprintf(line, "CALCULATOR_SHELL: Sub: %f - %f = %f\n", a, b, result);
						out(line);
					} else {
						out("SUB: Unexpected exception in Calc service\n");
					}
				} else {
					out("No calc service available\n");
				}
			} else {
				out("SUB: Requires 2 numerical parameter\n");
			}
		} else {
			out("SUB: Requires 2 numerical parameter\n");
			status = CELIX_ILLEGAL_ARGUMENT;
		}
    } else {
        out("No calc service available\n");
    }

    //return status;
}

celix_status_t subCommand_isNumeric(command_pt command, char *number, bool *ret) {
	celix_status_t status = CELIX_SUCCESS;
	*ret = true;
	while(*number) {
		if(!isdigit(*number) && *number != '.') {
			*ret = false;
			break;
		}
		number++;
	}
	return status;
}
