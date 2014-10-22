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
 * activator.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* celix.framework */
#include "bundle_activator.h"
#include "bundle_context.h"
#include "constants.h"
#include "properties.h"
#include "utils.h"
/* celix.utils */
#include "hash_map.h"
/* celix.configadmin */
#include "configuration_admin.h"
#include "configuration.h"
#include "managed_service.h"
/* celix.config_admin.examples.private */
#include "example_managed_service_impl.h"


static void test_debugConfiguration(configuration_t configuration, char* pid);


celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	*userData = NULL;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT ctx) {

	SERVICE_REFERENCE ref = NULL;
	celix_status_t status = bundleContext_getServiceReference(ctx, (char *) CONFIGURATION_ADMIN_SERVICE_NAME, &ref);

	if (status == CELIX_SUCCESS) {

		if (ref == NULL) {

			printf("[configAdminClient]: ConfigAdminService reference not available\n");

		} else {

			printf("------------------- DEMO ---------------------- \n");

			configuration_admin_service_t confAdminServ = NULL;
			bundleContext_getService(ctx, ref, (void *) &confAdminServ);

			if (confAdminServ == NULL){

				printf("[ TEST ]: ConfigAdminService not available\n");

			} else {

				printf("/////////////////// TESTS ///////////////////// \n");

				char *pid = "base.device1";

				/* ------------------ get Configuration -------------------*/

				printf("------------------- TEST01 -------------------- \n");
				printf("[ TEST ]: getConfiguration(pid=%s) - It's new \n",pid);
				configuration_t configuration;
				(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,pid, &configuration);

				test_debugConfiguration(configuration, pid);

				/* ------------------ get Configuration -------------------*/

				char *pid2 = "base.device1";

				printf("------------------- TEST02--------------------- \n");
				printf("[ TEST ]: getConfiguration(pid=%s) - Looking for it in Cache \n",pid2);
				configuration_t configuration2;
				(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,pid2, &configuration2);

				test_debugConfiguration(configuration2, pid2);

				/* ------------------ register ManagedService -------------*/

				printf("------------------- TEST03 -------------------- \n");
				printf("[ TEST ]: register ManagedService(pid=%s) \n",pid);


				managed_service_t instance;
				status = managedServiceImpl_create(ctx, &instance);
				if (status != CELIX_SUCCESS){
					return status;
				}

				managed_service_service_t managedService;
				status = managedService_create(ctx, &managedService);
				if (status != CELIX_SUCCESS){
					return status;
				}

				managedService->managedService = instance;
				managedService->updated = managedServiceImpl_updated;

				PROPERTIES dictionary;
				dictionary = properties_create();
				properties_set(dictionary, (char *) SERVICE_PID, pid);

				status = bundleContext_registerService(ctx, (char *) MANAGED_SERVICE_SERVICE_NAME,
						managedService, dictionary, &instance->registration);
				if (status != CELIX_SUCCESS){
					printf("[ ERROR ]: Managed Service not registered \n");
					return status;
				}

				printf("[ TEST ]: ManagedService(pid=%s) registered  \n",pid);

				/* ------------------ update Configuration ----------------*/

				printf("------------------- TEST04 -------------------- \n");
				printf("[ TEST ]: configuration update NULL \n");
				configuration_update(configuration , NULL);

				/* ------------------ update Configuration ----------------*/

				printf("------------------- TEST05 -------------------- \n");
				printf("[ TEST ]: configuration update New Properties \n");
				char *prop1 = "type";
				char *value1 = "printer";
				PROPERTIES properties = properties_create();
				properties_set(properties, prop1, value1);
				configuration_update(configuration , properties);

				/* ------------------ Configuration get Props ----------------*/

				printf("------------------- TEST06 -------------------- \n");
				printf("[ TEST ]: configuration get properties \n");

				PROPERTIES propsRx = properties_create();
				configuration_getProperties(configuration, &propsRx);

				printf("[ TEST ]: PROP=%s - VALUE=%s \n", (char*)SERVICE_PID, properties_get(propsRx,(char*)SERVICE_PID));
				printf("[ TEST ]: PROP=%s - VALUE=%s \n", prop1, properties_get(propsRx,prop1));

				printf("/////////////////// END TESTS ///////////////// \n");

			}
		}
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

void test_debugConfiguration(configuration_t configuration, char* pid){

	if (configuration == NULL){
		printf("[ TEST ]: Configuration(pid=%s) is NULL \n", pid);
	} else{
		printf("[ TEST ]: Configuration(pid=%s) OK \n", pid);
	}

}
