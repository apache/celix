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
 * configuration_admin_impl.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* celix.config_admin.ConfigAdmin */
#include "configuration_admin_impl.h"

/* celix.framework.public */
#include "celix_utils.h"
/* celix.framework_patch*/
#include "framework_patch.h"
/* celix.config_admin.private*/
#include "configuration_impl.h"


static celix_status_t configurationAdmin_checkPid(char *pid);


/* ========== CONSTRUCTOR ========== */

celix_status_t configurationAdmin_create(configuration_admin_factory_pt factory, configuration_store_pt store, bundle_pt bundle,
										 configuration_admin_service_pt *service){

	*service = calloc(1, sizeof(**service));


	if(!*service){
		printf("[ ERROR ]: ConfigAdmin - Not initialized(ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	configuration_admin_pt this = calloc(1, sizeof(*this));
	if (!this){
		printf("[ ERROR ]: ConfigAdmin - Not initialized \n");
		return CELIX_ENOMEM;
	}

	this->bundle = bundle;

	this->configurationAdminFactory = factory;
	this->configurationStore = store;

	(*service)->configAdmin = this;
	(*service)->createFactoryConfiguration = configurationAdmin_createFactoryConfiguration;
	(*service)->createFactoryConfiguration2 = configurationAdmin_createFactoryConfiguration2;
	(*service)->getConfiguration = configurationAdmin_getConfiguration;
	(*service)->getConfiguration2 = configurationAdmin_getConfiguration2;
	(*service)->listConfigurations = configurationAdmin_listConfigurations;

	return CELIX_SUCCESS;

}

celix_status_t configurationAdmin_destroy(configuration_admin_service_pt *service) {
    free((*service)->configAdmin);
    free(*service);

    return CELIX_SUCCESS;
}
/* ========== IMPLEMENTATION ========== */


/* ---------- public ---------- */

celix_status_t configurationAdmin_createFactoryConfiguration(configuration_admin_pt configAdmin, char *factoryPid, configuration_pt *configuration){
	return CELIX_SUCCESS;
}

celix_status_t configurationAdmin_createFactoryConfiguration2(configuration_admin_pt configAdmin, char *factoryPid, char *location, configuration_pt *configuration){
	return CELIX_SUCCESS;
}

celix_status_t configurationAdmin_getConfiguration(configuration_admin_pt configAdmin, char *pid, configuration_pt *configuration){
	configuration_pt config;

	const char* configAdminBundleLocation;
	const char* configBundleLocation;


	// (1) configurationAdmin.checkPid
	if ( configurationAdmin_checkPid(pid) != CELIX_SUCCESS ){
		*configuration = NULL;
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (2) bundle.getLocation
	if ( bundle_getBundleLocation(configAdmin->bundle,&configAdminBundleLocation) != CELIX_SUCCESS ){
		*configuration = NULL;
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (3) delegates to configurationStore.getConfiguration
	if ( configurationStore_getConfiguration(configAdmin->configurationStore, pid, (char*)configAdminBundleLocation, &config) != CELIX_SUCCESS ){
		*configuration = NULL;
		return CELIX_ILLEGAL_ARGUMENT;
	}


	/* ---------- pseudo code ---------- */
	/*

	(4) is the configuration already bound? / configBundleLocation != NULL ?
		YES - are the Configuration and the ConfigAdmin bound to the same bundle?
			NO - check permission

	(5) config.bind(bundle)
		if the Configuration is not bound, then we bind it up with the ConfigAdmin bundle
		in case of be bound, the function does not bind it up with ConfigAdmin bundle

	*/


	// (4) config.getBundleLocation != NULL ?
	if ( configuration_getBundleLocation2(config->handle,false,(char**)&configBundleLocation) == CELIX_SUCCESS ){

		if ( strcmp(configAdminBundleLocation,configBundleLocation) != 0 ){

			if ( configurationAdminFactory_checkConfigurationPermission(configAdmin->configurationAdminFactory) != CELIX_SUCCESS ){
				printf("[ ERROR ]: ConfigAdmin - Config. Permission \n");
				*configuration = NULL;
				return CELIX_ILLEGAL_STATE; //TODO checkConfigurationPermission not yet implemented, everything is allowed
			}
		}
	}

	// (5) config.bind(bundle)
	bool dummy;
	if ( configuration_bind(config->handle, configAdmin->bundle, &dummy) != CELIX_SUCCESS){
		*configuration = NULL;
		printf("[ ERROR]: ConfigAdmin - bind Config.");
		return CELIX_ILLEGAL_STATE;
	}

	*configuration = config;
	return CELIX_SUCCESS;

}

celix_status_t configurationAdmin_getConfiguration2(configuration_admin_pt configAdmin, char *pid, char *location, configuration_pt *configuration){

//	celix_status_t status;
//
//	status = configurationAdmin_checkPid(pid);
//	if (status != CELIX_SUCCESS){
//		*configuration = NULL;
//		return status;
//	}
//
//	status = configurationAdminFactory_checkConfigurationPermission(configAdmin->configurationAdminFactory);
//	if (status != CELIX_SUCCESS){
//		*configuration = NULL;
//		return status;
//	}
//
//	configuration_pt config;
//	status = configurationStore_getConfiguration(configAdmin->configurationStore, pid, location, &config);
//	if (status != CELIX_SUCCESS){
//		*configuration = NULL;
//		return status;
//	}
//
//	printf("SUCCESS get Configuration");
//	*configuration = config;
	return CELIX_SUCCESS;
}

celix_status_t configurationAdmin_listConfigurations(configuration_admin_pt configAdmin, char *filter, array_list_pt *configurations){
	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

celix_status_t configurationAdmin_checkPid(char *pid){

	if (pid == NULL){
		printf("[ ERROR ]: PID cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	} else {
		return CELIX_SUCCESS;
	}
}
