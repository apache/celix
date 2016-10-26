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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"

#include "std_commands.h"

#define SERVICE_TYPE "service"
#define CAPABILITY "capability"
#define REQUIREMENT "requirement"

celix_status_t inspectCommand_printExportedServices(bundle_context_pt context, array_list_pt ids, FILE *outStream, FILE *errStream);
celix_status_t inspectCommand_printImportedServices(bundle_context_pt context, array_list_pt ids, FILE *outStream, FILE *errStream);

celix_status_t inspectCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_context_pt context = handle;

	char *token;
	strtok_r(commandline, " ", &token);
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
					status = inspectCommand_printExportedServices(context, ids, outStream, errStream);
					if (status != CELIX_SUCCESS) {
						fprintf(errStream, "INSPECT: Error\n");
					}
				} else if (strcmp(direction, REQUIREMENT) == 0) {
                    status = inspectCommand_printImportedServices(context, ids, outStream, errStream);
                    if (status != CELIX_SUCCESS) {
						fprintf(errStream, "INSPECT: Error\n");
                    }
				} else {
					fprintf(errStream, "INSPECT: Invalid argument\n");
				}
			} else {
				fprintf(errStream, "INSPECT: Invalid argument\n");
			}
			arrayList_destroy(ids);
		} else {
			fprintf(errStream, "INSPECT: Too few arguments\n");
		}
	} else {
		fprintf(errStream, "INSPECT: Too few arguments\n");
	}
	return status;
}

celix_status_t inspectCommand_printExportedServices(bundle_context_pt context, array_list_pt ids, FILE *outStream, FILE *errStream) {
	celix_status_t status = CELIX_SUCCESS;
	array_list_pt bundles = NULL;

	if (arrayList_isEmpty(ids)) {
		status = bundleContext_getBundles(context, &bundles);
	} else {
		unsigned int i;

		arrayList_create(&bundles);
		for (i = 0; i < arrayList_size(ids); i++) {
			char *idStr = (char *) arrayList_get(ids, i);
			long id = atol(idStr);
			bundle_pt b = NULL;
			celix_status_t st = bundleContext_getBundleById(context, id, &b);
			if (st == CELIX_SUCCESS) {
				arrayList_add(bundles, b);
			} else {
				fprintf(outStream, "INSPECT: Invalid bundle ID: %ld\n", id);
			}
		}
	}

	if (status == CELIX_SUCCESS) {
		unsigned int i = 0;
		for (i = 0; i < arrayList_size(bundles); i++) {
			bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);

			if (i > 0) {
				fprintf(outStream, "\n");
			}

			if (bundle != NULL) {
				array_list_pt refs = NULL;

				if (bundle_getRegisteredServices(bundle, &refs) == CELIX_SUCCESS) {
					module_pt module = NULL;
					const char * name = NULL;
					status = bundle_getCurrentModule(bundle, &module);
					if (status == CELIX_SUCCESS) {
						status = module_getSymbolicName(module, &name);
						if (status == CELIX_SUCCESS) {
							fprintf(outStream, "%s provides services:\n", name);
							fprintf(outStream, "==============\n");

							if (refs == NULL || arrayList_size(refs) == 0) {
								fprintf(outStream, "Nothing\n");
							} else {
								unsigned int j = 0;
								for (j = 0; j < arrayList_size(refs); j++) {
									service_reference_pt ref = (service_reference_pt) arrayList_get(refs, j);
									unsigned int size = 0;
									char **keys;

									serviceReference_getPropertyKeys(ref, &keys, &size);
									for (int k = 0; k < size; k++) {
									    char *key = keys[k];
									    const char *value = NULL;
									    serviceReference_getProperty(ref, key, &value);

										fprintf(outStream, "%s = %s\n", key, value);
									}

									free(keys);

//									objectClass = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
//									sprintf(line, "ObjectClass = %s\n", objectClass);
									if ((j + 1) < arrayList_size(refs)) {
										fprintf(outStream, "----\n");
									}
								}
							}
						}
					}
				}

				if(refs!=NULL){
					arrayList_destroy(refs);
				}
			}
		}
	}

	if (bundles != NULL) {
	    arrayList_destroy(bundles);
	}

	return status;
}

celix_status_t inspectCommand_printImportedServices(bundle_context_pt context, array_list_pt ids, FILE *outStream, FILE *errStream) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt bundles = NULL;

    if (arrayList_isEmpty(ids)) {
        status = bundleContext_getBundles(context, &bundles);
    } else {
        unsigned int i;

        arrayList_create(&bundles);
        for (i = 0; i < arrayList_size(ids); i++) {
            char *idStr = (char *) arrayList_get(ids, i);
            long id = atol(idStr);
            bundle_pt b = NULL;
            celix_status_t st = bundleContext_getBundleById(context, id, &b);
            if (st == CELIX_SUCCESS) {
                arrayList_add(bundles, b);
            } else {
				fprintf(outStream, "INSPECT: Invalid bundle ID: %ld\n", id);
            }
        }
    }

    if (status == CELIX_SUCCESS) {
        unsigned int i = 0;
        for (i = 0; i < arrayList_size(bundles); i++) {
            bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);

            if (i > 0) {
				fprintf(outStream, "\n");
            }

            if (bundle != NULL) {
                array_list_pt refs = NULL;

                if (bundle_getServicesInUse(bundle, &refs) == CELIX_SUCCESS) {
                    module_pt module = NULL;
                    const char * name = NULL;
                    status = bundle_getCurrentModule(bundle, &module);
                    if (status == CELIX_SUCCESS) {
                        status = module_getSymbolicName(module, &name);
                        if (status == CELIX_SUCCESS) {
							fprintf(outStream, "%s requires services:\n", name);
							fprintf(outStream, "==============\n");

                            if (refs == NULL || arrayList_size(refs) == 0) {
								fprintf(outStream, "Nothing\n");
                            } else {
                                unsigned int j = 0;
                                for (j = 0; j < arrayList_size(refs); j++) {
                                    service_reference_pt ref = (service_reference_pt) arrayList_get(refs, j);
                                    bundle_pt usedBundle = NULL;
                                    module_pt usedModule = NULL;
                                    const char *usedSymbolicName = NULL;
                                    long usedBundleId;

                                    serviceReference_getBundle(ref, &usedBundle);
                                    bundle_getBundleId(usedBundle, &usedBundleId);
                                    bundle_getCurrentModule(usedBundle, &usedModule);
                                    module_getSymbolicName(usedModule, &usedSymbolicName);

									fprintf(outStream, "%s [%ld]\n", usedSymbolicName, usedBundleId);

                                    unsigned int size = 0;
                                    char **keys;

                                    serviceReference_getPropertyKeys(ref, &keys, &size);
                                    for (int k = 0; k < size; k++) {
                                        char *key = keys[k];
                                        const char *value = NULL;
                                        serviceReference_getProperty(ref, key, &value);

										fprintf(outStream, "%s = %s\n", key, value);
                                    }
                                    free(keys);

//                                  objectClass = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
//                                  sprintf(line, "ObjectClass = %s\n", objectClass);
                                    if ((j + 1) < arrayList_size(refs)) {
										fprintf(outStream, "----\n");
                                    }
                                }
                            }
                        }
                    }
                }

                if(refs!=NULL){
                	arrayList_destroy(refs);
                }
            }
        }
    }

    arrayList_destroy(bundles);


    return status;
}
