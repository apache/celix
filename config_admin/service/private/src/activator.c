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


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* celix.framework.public */
#include "bundle_activator.h"
#include "bundle_context.h"
#include "celix_errno.h"
#include "service_factory.h"
#include "service_registration.h"
/* celix.config_admin.public */
#include "configuration_admin.h"
/* celix.config_admin.private */
#include "configuration_admin_factory.h"


struct config_admin_bundle {
	bundle_context_pt context;
	service_registration_pt configAdminFactoryReg;
};

typedef struct config_admin_bundle *config_admin_bundle_t;


/* -------------------- Implements BundleActivator ---------------------------- */

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {

	celix_status_t status = CELIX_SUCCESS;

	config_admin_bundle_t bi = calloc(1, sizeof(struct config_admin_bundle));

	if (bi == NULL) {

		status = CELIX_ENOMEM;

	} else {

		(*userData) = bi;
		bi->context = context;
		bi->configAdminFactoryReg = NULL;

		status = CELIX_SUCCESS;

	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {

	celix_status_t status;

	config_admin_bundle_t bi = (config_admin_bundle_t) userData;

	service_factory_pt configAdminFactory;
	configuration_admin_factory_pt configAdminFactoryInstance;

	status = configurationAdminFactory_create(bi->context, &configAdminFactory, &configAdminFactoryInstance);
	if (status != CELIX_SUCCESS){
		return status;
	}

	status = bundleContext_registerServiceFactory(bi->context, (char *) CONFIGURATION_ADMIN_SERVICE_NAME, configAdminFactory, NULL, &bi->configAdminFactoryReg);
	if (status != CELIX_SUCCESS){
		return status;
	}
	printf("[ SUCCESS ]: Activator - ConfigAdminFactory Registered \n");

	status = configurationAdminFactory_start(configAdminFactoryInstance);
	if (status != CELIX_SUCCESS){
		return status;
	}

	return CELIX_SUCCESS;

}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {

	celix_status_t status = CELIX_SUCCESS;

	config_admin_bundle_t bi = (config_admin_bundle_t) userData;

	serviceRegistration_unregister(bi->configAdminFactoryReg);

	bi->configAdminFactoryReg = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}







