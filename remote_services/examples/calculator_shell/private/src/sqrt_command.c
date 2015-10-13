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
 * sqrt_command.c
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
#include "sqrt_command.h"
#include "calculator_service.h"


void sqrtCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));
celix_status_t sqrtCommand_isNumeric(command_pt command, char *number, bool *ret);

command_pt sqrtCommand_create(bundle_context_pt context) {
    command_pt command = (command_pt) calloc(1, sizeof(*command));
    if (command) {
		command->bundleContext = context;
		command->name = "sqrt";
		command->shortDescription = "calculates the square root of the given double";
		command->usage = "sqrt <double>";
		command->executeCommand = sqrtCommand_execute;
    }
    return command;
}

void sqrtCommand_destroy(command_pt command) {
	free(command);
}

void sqrtCommand_execute(command_pt command, char *line, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
    service_reference_pt calculatorService = NULL;

    status = bundleContext_getServiceReference(command->bundleContext, (char *) CALCULATOR_SERVICE, &calculatorService);
    if (status == CELIX_SUCCESS) {
    	char *token = line;
    	strtok_r(line, " ", &token);
		char *aStr = strtok_r(NULL, " ", &token);
		bool numeric;
		sqrtCommand_isNumeric(command, aStr, &numeric);
		if (aStr != NULL && numeric) {
			calculator_service_pt calculator = NULL;
			status = bundleContext_getService(command->bundleContext, calculatorService, (void *) &calculator);
			if (status == CELIX_SUCCESS) {
				double a = atof(aStr);
				double result = 0;
				status = calculator->sqrt(calculator->calculator, a, &result);
				if (status == CELIX_SUCCESS) {
					char line[256];
					sprintf(line, "CALCULATOR_SHELL: Sqrt: %f = %f\n", a, result);
					out(line);
				} else {
					out("SQRT: Unexpected exception in Calc service\n");
				}
			} else {
				out("No calc service available\n");
			}
		} else {
			out("SQRT: Requires 1 numerical parameter\n");
			status = CELIX_ILLEGAL_ARGUMENT;
		}
    } else {
        out("No calc service available\n");
    }

    //return status;
}

celix_status_t sqrtCommand_isNumeric(command_pt command, char *number, bool *ret) {
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
