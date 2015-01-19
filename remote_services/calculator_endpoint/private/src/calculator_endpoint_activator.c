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
 * calculator_endpoint_activator.c
 *
 *  \date       Oct 10, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"

#include "calculator_endpoint_impl.h"
#include "service_registration.h"

struct activator {
	remote_endpoint_service_pt endpointService;
	service_registration_pt endpointServiceRegistration;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator;

	activator = calloc(1, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	} else {
		activator->endpointService = NULL;
		activator->endpointServiceRegistration = NULL;

		*userData = activator;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	remote_endpoint_pt endpoint = NULL;
	remote_endpoint_service_pt endpointService = NULL;

	calculatorEndpoint_create(&endpoint);
	endpointService = calloc(1, sizeof(*endpointService));
	endpointService->endpoint = endpoint;
	endpointService->handleRequest = calculatorEndpoint_handleRequest;
	endpointService->setService = calculatorEndpoint_setService;

	properties_pt props = properties_create();
	properties_set(props, (char *) "remote.interface", (char *) CALCULATOR_SERVICE);

	bundleContext_registerService(context, OSGI_RSA_REMOTE_ENDPOINT, endpointService, props, &activator->endpointServiceRegistration);

	activator->endpointService = endpointService;

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->endpointServiceRegistration);

	free(activator->endpointService->endpoint);
	free(activator->endpointService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	free(activator);
	return status;
}
