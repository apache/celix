/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * activator.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "example2_managed_service_impl.h"
/* celix.framework */
#include "bundle_activator.h"
#include "bundle_context.h"
#include "celix_constants.h"
#include "properties.h"
#include "utils.h"
/* celix.utils */
#include "hash_map.h"
/* celix.configadmin */
#include "configuration_admin.h"
#include "configuration.h"
#include "managed_service.h"
#include "service_registration.h"
/* celix.config_admin.examples.private */


//static void test_debugConfiguration(configuration_pt configuration, char* pid);
struct activator {
	bundle_context_pt context;
	tst2_service_pt          tstServ;
	service_registration_pt  tstReg;

    service_registration_pt   mgmReg;
    managed_service_service_pt  mgmServ;


	service_reference_pt	  configAdminServRef;
	configuration_admin_service_pt   configAdminServ;

	configuration_pt configuration;

};


int my_get_type(void *handle, char* value) {
	struct activator *act = (struct activator *)handle;
	if (act->configuration != NULL) {
		properties_pt propsRx;
		act->configuration->configuration_getProperties(act->configuration->handle, &propsRx);
		if (propsRx != NULL) {
			printf("[ TEST ]: PROP=%s - VALUE=%s \n", (char*)CELIX_FRAMEWORK_SERVICE_PID, properties_get(propsRx,(char*)CELIX_FRAMEWORK_SERVICE_PID));
			strcpy(value, properties_get(propsRx,"type"));
		}
		else {
			printf("[ TEST ]: No properties found \n");
			strcpy(value, "");
		}
	}
	else
		value[0] = 0x00;
	return 0;
}
celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator *act = calloc(1, sizeof(struct activator));
	act->mgmReg = NULL;
	act->mgmServ = NULL;
	act->tstServ = calloc(1, sizeof(*act->tstServ));
	act->tstServ->handle = act;
	act->tstServ->get_type = my_get_type;
	act->tstReg = NULL;
	act->configAdminServ = NULL;
	act->configAdminServRef = NULL;
	act->configuration = NULL;
	*userData = act;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt ctx) {

	struct activator *act = (struct activator *)userData;
	service_reference_pt ref;
	celix_status_t status = bundleContext_getServiceReference(ctx, (char *) CONFIGURATION_ADMIN_SERVICE_NAME, &ref);

	if (status == CELIX_SUCCESS) {

		if (ref == NULL) {
			printf("[configAdminClient]: ConfigAdminService reference not available\n");
		} else {
			configuration_admin_service_pt confAdminServ = NULL;
			bundleContext_getService(ctx, ref, (void *) &confAdminServ);

			if (confAdminServ == NULL){
				printf("[ TEST ]: ConfigAdminService not available\n");
			} else {
				char *pid = "test2_pid";
                /* In example_test a managed service is registered for which there already exists a
                 * configuration object. In this case, a managed service is register for which there
                 * is no configuration object in the configAdmin
                 */
				/* ------------------ get Configuration -------------------*
				(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,pid, &act->configuration);
				*/
				act->configAdminServ = confAdminServ;
				act->configAdminServRef = ref;

			    managed_service_pt managedService;
				status = managedServiceImpl_create(ctx, &managedService);
				if (status != CELIX_SUCCESS){
					return status;
				}

				status = managedService_create(ctx, &act->mgmServ);
				if (status != CELIX_SUCCESS){
					return status;
				}

				act->mgmServ->managedService = managedService;
				act->mgmServ->updated = managedServiceImpl_updated;

				properties_pt dictionary;
				dictionary = properties_create();
				properties_set(dictionary, (char *) CELIX_FRAMEWORK_SERVICE_PID, pid);
				properties_set(dictionary, (char *) "type", (char*)"test2_default_value");

				status = bundleContext_registerService(ctx, (char *) MANAGED_SERVICE_SERVICE_NAME,
				        act->mgmServ, dictionary, &act->mgmReg);
				if (status != CELIX_SUCCESS){
					printf("[ ERROR ]: Managed Service not registered \n");
					return status;
				}

				status = bundleContext_registerService(ctx, (char *)TST2_SERVICE_NAME, act->tstServ, NULL, &act->tstReg);

#if 0
				/* ------------------ update Configuration ----------------*/

				printf("------------------- TEST04 -------------------- \n");
				printf("[ TEST ]: configuration update NULL \n");
				configuration_update(configuration , NULL);

				/* ------------------ update Configuration ----------------*/

				printf("------------------- TEST05 -------------------- \n");
				printf("[ TEST ]: configuration update New Properties \n");
				char *prop1 = "type";
				char *value1 = "printer";
				properties_pt properties = properties_create();
				properties_set(properties, prop1, value1);
				configuration_update(configuration , properties);

				/* ------------------ Configuration get Props ----------------*/

				printf("------------------- TEST06 -------------------- \n");
				printf("[ TEST ]: configuration get properties \n");

				properties_pt propsRx = properties_create();
				configuration_getProperties(configuration, &propsRx);

				printf("[ TEST ]: PROP=%s - VALUE=%s \n", (char*)CELIX_FRAMEWORK_SERVICE_PID, properties_get(propsRx,(char*)CELIX_FRAMEWORK_SERVICE_PID));
				printf("[ TEST ]: PROP=%s - VALUE=%s \n", prop1, properties_get(propsRx,prop1));

				printf("/////////////////// END TESTS ///////////////// \n");
#endif
			}
		}
	}
	return status;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status;
	struct activator *act = (struct activator *)userData;
	if (act->mgmReg != NULL)
		status = serviceRegistration_unregister(act->mgmReg);
	if (act->tstReg != NULL)
		status = serviceRegistration_unregister(act->tstReg);
	if (act->configAdminServRef != NULL) {
		status = bundleContext_ungetService(context, act->configAdminServRef, NULL);
		status = bundleContext_ungetServiceReference(context, act->configAdminServRef);
	}
	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct activator *act = (struct activator *)userData;
    managedServiceImpl_destroy(&act->mgmServ->managedService);
    managedService_destroy(act->mgmServ);
	free(act->tstServ);
	free(act);
	return CELIX_SUCCESS;
}

#if 0
void test_debugConfiguration(configuration_pt configuration, char* pid){

	if (configuration == NULL){
		printf("[ TEST ]: Configuration(pid=%s) is NULL \n", pid);
	} else{
		printf("[ TEST ]: Configuration(pid=%s) OK \n", pid);
	}

}
#endif
