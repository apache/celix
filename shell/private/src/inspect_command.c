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
 * inspect_command.c
 *
 *  \date       Oct 13, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <apr_strings.h>

#include "command_private.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"
#include "module.h"
#include "constants.h"
#include "service_registration.h"
#include "service_reference.h"

#define SERVICE_TYPE "service"
#define CAPABILITY "capability"
#define REQUIREMENT "requirement"

void inspectCommand_execute(COMMAND command, char * commandline, void (*out)(char *), void (*err)(char *));
celix_status_t inspectCommand_printExportedServices(COMMAND command, ARRAY_LIST ids, void (*out)(char *), void (*err)(char *));

COMMAND inspectCommand_create(bundle_context_t context) {
	COMMAND command = (COMMAND) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "inspect";
	command->shortDescription = "inspect dependencies";
	command->usage = "inspect (service) (capability|requirement) [<id> ...]";
	command->executeCommand = inspectCommand_execute;
	return command;
}

void inspectCommand_destroy(COMMAND command) {
	free(command);
}

void inspectCommand_execute(COMMAND command, char * commandline, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
	char *token;
	char *commandStr = apr_strtok(commandline, " ", &token);
	char *type = apr_strtok(NULL, " ", &token);
	if (type != NULL) {
		char *direction = apr_strtok(NULL, " ", &token);
		if (direction != NULL) {
			apr_pool_t *pool = NULL;
			bundleContext_getMemoryPool(command->bundleContext, &pool);
			ARRAY_LIST ids = NULL;
			arrayList_create(pool, &ids);
			char *id = apr_strtok(NULL, " ", &token);
			while (id != NULL) {
				arrayList_add(ids, id);
				id = apr_strtok(NULL, " ", &token);
			}

			if (strcmp(type, SERVICE_TYPE) == 0) {
				if (strcmp(direction, CAPABILITY) == 0) {
					status = inspectCommand_printExportedServices(command, ids, out, err);
					if (status != CELIX_SUCCESS) {
						out("INSPECT: Error\n");
					}
				} else {
					out("INSPECT: Not implemented\n");
				}
			} else {
				out("INSPECT: Invalid argument\n");
			}
		} else {
			out("INSPECT: Too few arguments\n");
		}
	} else {
		out("INSPECT: Too few arguments\n");
	}
}

celix_status_t inspectCommand_printExportedServices(COMMAND command, ARRAY_LIST ids, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
	ARRAY_LIST bundles = NULL;

	if (arrayList_isEmpty(ids)) {
		celix_status_t status = bundleContext_getBundles(command->bundleContext, &bundles);
	} else {
		apr_pool_t *pool = NULL;
		bundleContext_getMemoryPool(command->bundleContext, &pool);
		arrayList_create(pool, &bundles);
		int i;
		for (i = 0; i < arrayList_size(ids); i++) {
			char *idStr = arrayList_get(ids, i);
			long id = atol(idStr);
			BUNDLE b = NULL;
			celix_status_t st = bundleContext_getBundleById(command->bundleContext, id, &b);
			if (st == CELIX_SUCCESS) {
				arrayList_add(bundles, b);
			} else {
				char line[256];
				sprintf(line, "INSPECT: Invalid bundle ID: %ld\n", id);
				out(line);
			}
		}
	}

	if (status == CELIX_SUCCESS) {
		int i = 0;
		for (i = 0; i < arrayList_size(bundles); i++) {
			BUNDLE bundle = arrayList_get(bundles, i);

			if (i > 0) {
				out("\n");
			}

			if (bundle != NULL) {
				apr_pool_t *pool;
				bundleContext_getMemoryPool(command->bundleContext, &pool);
				ARRAY_LIST refs = NULL;
				if (bundle_getRegisteredServices(bundle, pool, &refs) == CELIX_SUCCESS) {
					char line[256];
					MODULE module = NULL;
					char * name = NULL;
					status = bundle_getCurrentModule(bundle, &module);
					if (status == CELIX_SUCCESS) {
						status = module_getSymbolicName(module, &name);
						if (status == CELIX_SUCCESS) {
							sprintf(line, "%s provides services:\n", name);
							out(line);
							out("==============\n");

							if (refs == NULL || arrayList_size(refs) == 0) {
								out("Nothing\n");
							} else {
								int j = 0;
								for (j = 0; j < arrayList_size(refs); j++) {
									SERVICE_REFERENCE ref = arrayList_get(refs, j);
									SERVICE_REGISTRATION reg = NULL;
									PROPERTIES props = NULL;
									serviceReference_getServiceRegistration(ref, &reg);
									char line[256];

									serviceRegistration_getProperties(reg, &props);
									char *objectClass = properties_get(props, (char *) OBJECTCLASS);
									sprintf(line, "ObjectClass = %s\n", objectClass);
									out(line);
									if ((j + 1) < arrayList_size(refs)) {
										out("----\n");
									}
								}
							}
						}
					}
				}
			}
		}
	}


	return status;
}
