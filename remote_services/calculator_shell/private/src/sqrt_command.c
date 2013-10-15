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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <ctype.h>

#include <apr_strings.h>

#include "array_list.h"
#include "bundle_context.h"
#include "sqrt_command.h"
#include "calculator_service.h"


void sqrtCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));
celix_status_t sqrtCommand_isNumeric(command_pt command, char *number, bool *ret);

command_pt sqrtCommand_create(bundle_context_pt context) {
	apr_pool_t *pool;
	bundleContext_getMemoryPool(context, &pool);

    command_pt command = (command_pt) apr_palloc(pool, sizeof(*command));
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
}

void sqrtCommand_execute(command_pt command, char *line, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
    service_reference_pt calculatorService = NULL;
    apr_pool_t *memory_pool = NULL;
    apr_pool_t *bundle_memory_pool = NULL;

    status = bundleContext_getServiceReference(command->bundleContext, (char *) CALCULATOR_SERVICE, &calculatorService);
    if (status == CELIX_SUCCESS) {
    	char *token;
		char *commandStr = apr_strtok(line, " ", &token);
		char *aStr = apr_strtok(NULL, " ", &token);
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

        double a;
        double b;
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
