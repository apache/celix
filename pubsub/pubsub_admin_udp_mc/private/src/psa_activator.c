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
 * psa_activator.c
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "bundle_activator.h"
#include "service_registration.h"

#include "pubsub_admin_impl.h"

struct activator {
	pubsub_admin_pt admin;
	pubsub_admin_service_pt adminService;
	service_registration_pt registration;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator;

	activator = calloc(1, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	}
	else{
		*userData = activator;
		status = pubsubAdmin_create(context, &(activator->admin));
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	pubsub_admin_service_pt pubsubAdminSvc = calloc(1, sizeof(*pubsubAdminSvc));

	if (!pubsubAdminSvc) {
		status = CELIX_ENOMEM;
	}
	else{
		pubsubAdminSvc->admin = activator->admin;

		pubsubAdminSvc->addPublication = pubsubAdmin_addPublication;
		pubsubAdminSvc->removePublication = pubsubAdmin_removePublication;

		pubsubAdminSvc->addSubscription = pubsubAdmin_addSubscription;
		pubsubAdminSvc->removeSubscription = pubsubAdmin_removeSubscription;

		pubsubAdminSvc->closeAllPublications = pubsubAdmin_closeAllPublications;
		pubsubAdminSvc->closeAllSubscriptions = pubsubAdmin_closeAllSubscriptions;

		activator->adminService = pubsubAdminSvc;

		status = bundleContext_registerService(context, PUBSUB_ADMIN_SERVICE, pubsubAdminSvc, NULL, &activator->registration);

	}


	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->registration);
	activator->registration = NULL;

	pubsubAdmin_stop(activator->admin);

	free(activator->adminService);
	activator->adminService = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	pubsubAdmin_destroy(activator->admin);
	activator->admin = NULL;

	free(activator);

	return status;
}


