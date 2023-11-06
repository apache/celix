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
#include "example_managed_service_impl.h"


//static void test_debugConfiguration(configuration_pt configuration, char* pid);
struct activator {
	bundle_context_pt context;
	tst_service_pt           tstServ;
	service_registration_pt  tstReg;

    service_registration_pt   mgmReg;
	managed_service_service_pt  mgmServ;

	service_reference_pt	  configAdminServRef;
	configuration_admin_service_pt   configAdminServ;

	char	type_value[125];
	char	second_type_value[125];
};


void store_properties(void *handle, char* type, char* second) {
	struct activator *act = (struct activator *)handle;
	if (type == NULL)
		act->type_value[0] = 0x00;
	else
		strcpy(act->type_value, type);
	if (second == NULL)
		act->second_type_value[0] = 0x00;
	else
		strcpy(act->second_type_value, second);
}
int my_get_type(void *handle, char* value) {
	struct activator *act = (struct activator *)handle;
	strcpy(value, act->type_value);

	return 0;
}

int my_get_second_type(void *handle, char* value) {
	struct activator *act = (struct activator *)handle;
	strcpy(value, act->second_type_value);

	return 0;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator *act = calloc(1, sizeof(struct activator));
	act->mgmReg = NULL;
	act->mgmServ = NULL;
	act->tstServ = calloc(1, sizeof(*act->tstServ));
	act->tstServ->handle = act;
	act->tstServ->get_type = my_get_type;
	act->tstServ->get_second_type = my_get_second_type;
	act->tstReg = NULL;
	act->configAdminServ = NULL;
	act->configAdminServRef = NULL;
	act->type_value[0] = 0x00;
	act->second_type_value[0] = 0x00;
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
				char *pid = "base.device1";
				configuration_pt configuration;
				/* ------------------ get Configuration -------------------*/
				(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,pid, &configuration);
				act->configAdminServ = confAdminServ;
				act->configAdminServRef = ref;
				managed_service_pt managedService;
				status = managedServiceImpl_create(ctx, &managedService);
				if (status != CELIX_SUCCESS){
					return status;
				}
				struct test_managed_service *test_msp = (struct test_managed_service*) managedService;
				test_msp->handle = act;
				test_msp->store_props = store_properties;

				status = managedService_create(ctx, &act->mgmServ);

				if (status != CELIX_SUCCESS){
					return status;
				}

				act->mgmServ->managedService = managedService;
				act->mgmServ->updated = managedServiceImpl_updated;
				properties_pt dictionary;
				configuration->configuration_getProperties(configuration->handle, &dictionary);
				if (dictionary == NULL) {
					dictionary = properties_create();
					properties_set(dictionary, (char *) CELIX_FRAMEWORK_SERVICE_PID, pid);
					properties_set(dictionary, (char *) "type", (char*)"default_value");
				}
				// the service has to be registered with a properties/dictionary structure that at least contains a pid to assure it
				// is picked up by the managedServiceTracker of the configuration Admin
				status = bundleContext_registerService(ctx, (char *) MANAGED_SERVICE_SERVICE_NAME, act->mgmServ, dictionary, &act->mgmReg);
				if (status != CELIX_SUCCESS){
					printf("[ ERROR ]: Managed Service not registered \n");
					return status;
				}
				status = bundleContext_registerService(ctx, (char *)TST_SERVICE_NAME, act->tstServ, NULL, &act->tstReg);

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
