/*
 * sqrt_command.c
 *
 *  Created on: Oct 13, 2011
 *      Author: alexander
 */

#include <stdlib.h>
#include <ctype.h>

#include <apr_strings.h>

#include "command_private.h"
#include "array_list.h"
#include "bundle_context.h"
#include "sqrt_command.h"
#include "example_service.h"


void sqrtCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *));
celix_status_t sqrtCommand_isNumeric(COMMAND command, char *number, bool *ret);

COMMAND sqrtCommand_create(BUNDLE_CONTEXT context) {
	apr_pool_t *pool;
	bundleContext_getMemoryPool(context, &pool);

    COMMAND command = (COMMAND) apr_palloc(pool, sizeof(*command));
    if (command) {
		command->bundleContext = context;
		command->name = "sqrt";
		command->shortDescription = "calculates the square root of the given double";
		command->usage = "sqrt <double>";
		command->executeCommand = sqrtCommand_execute;
    }
    return command;
}

void sqrtCommand_destroy(COMMAND command) {
}

void sqrtCommand_execute(COMMAND command, char *line, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
    SERVICE_REFERENCE exampleService = NULL;
    apr_pool_t *memory_pool = NULL;
    apr_pool_t *bundle_memory_pool = NULL;

    status = bundleContext_getServiceReference(command->bundleContext, (char *) EXAMPLE_SERVICE, &exampleService);
    if (status == CELIX_SUCCESS) {
    	char *token;
		char *commandStr = apr_strtok(line, " ", &token);
		char *aStr = apr_strtok(NULL, " ", &token);
		bool numeric;
		sqrtCommand_isNumeric(command, aStr, &numeric);
		if (aStr != NULL && numeric) {
			example_service_t example = NULL;
			status = bundleContext_getService(command->bundleContext, exampleService, (void *) &example);
			if (status == CELIX_SUCCESS) {
				double a = atof(aStr);
				double result = 0;
				status = example->sqrt(example->example, a, &result);
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

celix_status_t sqrtCommand_isNumeric(COMMAND command, char *number, bool *ret) {
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
