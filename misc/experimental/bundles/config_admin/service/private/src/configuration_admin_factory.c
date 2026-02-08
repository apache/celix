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
 * configuration_admin_factory.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* celix.config_admin.ConfigAdminFactory */
#include "configuration_admin_factory.h"

/* celix.framework.Patch*/
#include "framework_patch.h"
/* celix.config_admin.public*/
#include "configuration_admin.h"
/* celix.config_admin.private*/
#include "configuration_admin_impl.h"
#include "managed_service_tracker.h"
#include "configuration_store.h"


struct configuration_admin_factory{
	bundle_context_pt 	context;

//	configuration_permission_t configurationPermission;
//	event_dispatcher_t eventDispatcher;
//	plugin_manager_t pluginManager;
	managed_service_tracker_pt managedServiceTrackerHandle;
	service_tracker_pt managedServiceTracker;
//	managed_service_factory_ptracker_t managedServiceFactoryTracker;
	configuration_store_pt configurationStore;

};


celix_status_t configurationAdminFactory_getService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service);
celix_status_t configurationAdminFactory_ungetService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service);


/* ========== CONSTRUCTOR =========== */

/* ---------- public ---------- */

celix_status_t configurationAdminFactory_create( bundle_context_pt context, service_factory_pt *factory, configuration_admin_factory_pt *instance){

	celix_status_t status;

	configuration_admin_factory_pt this = calloc(1, sizeof(*this));
	configuration_store_pt configurationStore;
	managed_service_tracker_pt managedServiceTrackerHandle;
	service_tracker_pt managedServiceTracker;

	// (1) SERVICE FACTORY
	*factory = calloc(1, sizeof(**factory));
    if (!*factory) {
        return CELIX_ENOMEM;
    }
    // (2) FACTORY DATA
	if (!this){
		printf("[ ERROR ]: ConfigAdminFactory - Not initialized \n");
		return CELIX_ENOMEM;
	}
	// (2.1) CONFIGURATION STORE
	status = configurationStore_create(context, this, &configurationStore);
	if (status != CELIX_SUCCESS){
		return status;
	}
	// (2.2) SERVICE TRACKER
	status = managedServiceTracker_create(context, this, configurationStore, &managedServiceTrackerHandle, &managedServiceTracker);
	if (status != CELIX_SUCCESS){
		return status;
	}
	// (3) INITIALIZATION
	this->context = context;

	this->managedServiceTrackerHandle = managedServiceTrackerHandle;
	this->managedServiceTracker = managedServiceTracker;
	this->configurationStore = configurationStore;

	(*factory)->handle = this;
	(*factory)->getService = configurationAdminFactory_getService;
	(*factory)->ungetService = configurationAdminFactory_ungetService;

	*instance = this;
	return CELIX_SUCCESS;

}

celix_status_t configurationAdminFactory_destroy( bundle_context_pt context, configuration_admin_factory_pt instance){
	managedServiceTracker_destroy(context, instance->managedServiceTrackerHandle, instance->managedServiceTracker);
	configurationStore_destroy(instance->configurationStore);
	free(instance);
	return CELIX_SUCCESS;
}
/* ========== IMPLEMENTS SERVICE FACTORY ========== */

/* ---------- public ---------- */

celix_status_t configurationAdminFactory_getService(void *handle, bundle_pt bundle, service_registration_pt registration, void **service){

	celix_status_t status;

	configuration_admin_factory_pt configAdminFactory = (configuration_admin_factory_pt)handle;
	configuration_admin_service_pt confAdminService;

	// TODO
	// (1) reference = registration.getServiceReference
	// (2) eventDispatcher.setServiceReference(reference);

	// (3) return new ConfigurationAdminImpl(this, configurationStore, bundle);
	status = configurationAdmin_create(configAdminFactory, configAdminFactory->configurationStore, bundle, &confAdminService);
	if (status != CELIX_SUCCESS){
		return status;
	}

	/* DEBUG CODE *
		char* location;
		bundle_getBundleLocation(bundle, &location);
		printf("[ SUCCESS ]: ConfigAdminFactory - get configAdminService (bundle=%s) \n ",location);
	* END DEBUG CODE */

	(*service) = confAdminService;

	return CELIX_SUCCESS;

}

celix_status_t configurationAdminFactory_ungetService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service) {

    configuration_admin_service_pt confAdminService = (*service);

    configurationAdmin_destroy(&confAdminService);

    return CELIX_SUCCESS;
}


/* ========== IMPLEMENTATION ========== */

/* ---------- public ---------- */

celix_status_t configurationAdminFactory_start(configuration_admin_factory_pt factory){

	celix_status_t status;
	printf("%s\n", __func__);
	status = serviceTracker_open(factory->managedServiceTracker);
	if( status!=CELIX_SUCCESS ){
		printf("[ ERROR ]: ConfigAdminFactory - ManagedServiceTracker not opened \n");
		return status;
	}

	return CELIX_SUCCESS;
}

celix_status_t configurationAdminFactory_stop(configuration_admin_factory_pt factory){
	celix_status_t status = serviceTracker_close(factory->managedServiceTracker);
	return status;;
}

celix_status_t configurationAdminFactory_checkConfigurationPermission(configuration_admin_factory_pt factory){
	return CELIX_SUCCESS;
}

celix_status_t configurationAdminFactory_dispatchEvent(configuration_admin_factory_pt factory, int type, char *factoryPid, char *pid){
	return CELIX_SUCCESS;
}

celix_status_t configurationAdminFactory_notifyConfigurationUpdated(configuration_admin_factory_pt factory, configuration_pt configuration, bool isFactory){

	if (isFactory == true){

		return CELIX_SUCCESS;

	}else{

		return managedServiceTracker_notifyUpdated(factory->managedServiceTrackerHandle,configuration);

	}

}

celix_status_t configurationAdminFactory_notifyConfigurationDeleted(configuration_admin_factory_pt factory, configuration_pt configuration, bool isFactory){
	return CELIX_SUCCESS;
}

celix_status_t configurationAdminFactory_modifyConfiguration(configuration_admin_factory_pt factory, service_reference_pt reference, properties_pt properties){
	return CELIX_SUCCESS;
}
