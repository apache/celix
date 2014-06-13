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
#include <string.h>

#include "command_impl.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"
#include "module.h"
#include "constants.h"
#include "service_registration.h"
#include "service_reference.h"
#include "hash_map.h"

#define SERVICE_TYPE "service"
#define CAPABILITY "capability"
#define REQUIREMENT "requirement"

void inspectCommand_execute(command_pt command, char * commandline, void (*out)(char *), void (*err)(char *));
celix_status_t inspectCommand_printExportedServices(command_pt command, array_list_pt ids, void (*out)(char *), void (*err)(char *));
celix_status_t inspectCommand_printImportedServices(command_pt command, array_list_pt ids, void (*out)(char *), void (*err)(char *));

command_pt inspectCommand_create(bundle_context_pt context) {
	command_pt command = (command_pt) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "inspect";
	command->shortDescription = "inspect dependencies";
	command->usage = "inspect (service) (capability|requirement) [<id> ...]";
	command->executeCommand = inspectCommand_execute;
	return command;
}

void inspectCommand_destroy(command_pt command) {
	free(command);
}

void inspectCommand_execute(command_pt command, char * commandline, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
	char outString[256];
	char *token;
	char *commandStr = strtok_r(commandline, " ", &token);
	char *type = strtok_r(NULL, " ", &token);
	if (type != NULL) {
		char *direction = strtok_r(NULL, " ", &token);
		if (direction != NULL) {
			array_list_pt ids = NULL;
			char *id = strtok_r(NULL, " ", &token);

			arrayList_create(&ids);
			while (id != NULL) {
				arrayList_add(ids, id);
				id = strtok_r(NULL, " ", &token);
			}

			if (strcmp(type, SERVICE_TYPE) == 0) {
				if (strcmp(direction, CAPABILITY) == 0) {
					status = inspectCommand_printExportedServices(command, ids, out, err);
					if (status != CELIX_SUCCESS) {
						out("INSPECT: Error\n");
					}
				} else if (strcmp(direction, REQUIREMENT) == 0) {
                    status = inspectCommand_printImportedServices(command, ids, out, err);
                    if (status != CELIX_SUCCESS) {
                        out("INSPECT: Error\n");
                    }
				} else {
				    out("INSPECT: Invalid argument\n");
                    sprintf(outString, "%s\n", command->usage);
                    out(outString);
				}
			} else {
				out("INSPECT: Invalid argument\n");
				sprintf(outString, "%s\n", command->usage);
                out(outString);
			}
		} else {
			out("INSPECT: Too few arguments\n");
			sprintf(outString, "%s\n", command->usage);
			out(outString);
		}
	} else {
		out("INSPECT: Too few arguments\n");
		sprintf(outString, "%s\n", command->usage);
		out(outString);
	}
}

celix_status_t inspectCommand_printExportedServices(command_pt command, array_list_pt ids, void (*out)(char *), void (*err)(char *)) {
	celix_status_t status = CELIX_SUCCESS;
	array_list_pt bundles = NULL;

	if (arrayList_isEmpty(ids)) {
		celix_status_t status = bundleContext_getBundles(command->bundleContext, &bundles);
	} else {
		unsigned int i;

		arrayList_create(&bundles);
		for (i = 0; i < arrayList_size(ids); i++) {
			char *idStr = (char *) arrayList_get(ids, i);
			long id = atol(idStr);
			bundle_pt b = NULL;
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
		unsigned int i = 0;
		for (i = 0; i < arrayList_size(bundles); i++) {
			bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);

			if (i > 0) {
				out("\n");
			}

			if (bundle != NULL) {
				array_list_pt refs = NULL;

				if (bundle_getRegisteredServices(bundle, &refs) == CELIX_SUCCESS) {
					char line[256];
					module_pt module = NULL;
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
								unsigned int j = 0;
								for (j = 0; j < arrayList_size(refs); j++) {
									service_reference_pt ref = (service_reference_pt) arrayList_get(refs, j);
									service_registration_pt reg = NULL;
									properties_pt props = NULL;
									char line[256];
									char *objectClass = NULL;

									serviceReference_getServiceRegistration(ref, &reg);
									
									serviceRegistration_getProperties(reg, &props);
									hash_map_iterator_pt iter = hashMapIterator_create(props);
									while (hashMapIterator_hasNext(iter)) {
									    hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
									    sprintf(line, "%s = %s\n", hashMapEntry_getKey(entry), hashMapEntry_getValue(entry));
									    out(line);
									}
									hashMapIterator_destroy(iter);

//									objectClass = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
//									sprintf(line, "ObjectClass = %s\n", objectClass);
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

celix_status_t inspectCommand_printImportedServices(command_pt command, array_list_pt ids, void (*out)(char *), void (*err)(char *)) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt bundles = NULL;

    if (arrayList_isEmpty(ids)) {
        celix_status_t status = bundleContext_getBundles(command->bundleContext, &bundles);
    } else {
        unsigned int i;

        arrayList_create(&bundles);
        for (i = 0; i < arrayList_size(ids); i++) {
            char *idStr = (char *) arrayList_get(ids, i);
            long id = atol(idStr);
            bundle_pt b = NULL;
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
        unsigned int i = 0;
        for (i = 0; i < arrayList_size(bundles); i++) {
            bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);

            if (i > 0) {
                out("\n");
            }

            if (bundle != NULL) {
                array_list_pt refs = NULL;

                if (bundle_getServicesInUse(bundle, &refs) == CELIX_SUCCESS) {
                    char line[256];
                    module_pt module = NULL;
                    char * name = NULL;
                    status = bundle_getCurrentModule(bundle, &module);
                    if (status == CELIX_SUCCESS) {
                        status = module_getSymbolicName(module, &name);
                        if (status == CELIX_SUCCESS) {
                            sprintf(line, "%s requires services:\n", name);
                            out(line);
                            out("==============\n");

                            if (refs == NULL || arrayList_size(refs) == 0) {
                                out("Nothing\n");
                            } else {
                                unsigned int j = 0;
                                for (j = 0; j < arrayList_size(refs); j++) {
                                    service_reference_pt ref = (service_reference_pt) arrayList_get(refs, j);
                                    service_registration_pt reg = NULL;
                                    properties_pt props = NULL;
                                    char line[256];
                                    bundle_pt usedBundle = NULL;
                                    module_pt usedModule = NULL;
                                    char *usedSymbolicName = NULL;
                                    long usedBundleId;

                                    serviceReference_getBundle(ref, &usedBundle);
                                    bundle_getBundleId(usedBundle, &usedBundleId);
                                    bundle_getCurrentModule(usedBundle, &usedModule);
                                    module_getSymbolicName(usedModule, &usedSymbolicName);

                                    sprintf(line, "%s [%ld]\n", usedSymbolicName, usedBundleId);
                                    out(line);

                                    serviceReference_getServiceRegistration(ref, &reg);
                                    serviceRegistration_getProperties(reg, &props);
                                    hash_map_iterator_pt iter = hashMapIterator_create(props);
                                    while (hashMapIterator_hasNext(iter)) {
                                        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
                                        sprintf(line, "%s = %s\n", hashMapEntry_getKey(entry), hashMapEntry_getValue(entry));
                                        out(line);
                                    }
									hashMapIterator_destroy(iter);

//                                  objectClass = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
//                                  sprintf(line, "ObjectClass = %s\n", objectClass);
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
