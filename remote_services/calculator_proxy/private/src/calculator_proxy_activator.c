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
 * calculator_proxy_activator.c
 *
 *  \date       Oct 10, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "service_registration.h"
#include "remote_proxy.h"

#include "calculator_proxy_impl.h"

struct activator {
	bundle_context_pt context;
	remote_proxy_factory_pt factory_ptr;
};

static celix_status_t calculatorProxyFactory_create(void *handle, endpoint_description_pt endpointDescription, remote_service_admin_pt rsa, sendToHandle sendToCallback, properties_pt properties, void **service);
static celix_status_t calculatorProxyFactory_destroy(void *handle, void *service);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator;

	activator = calloc(1, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	} else {
		activator->factory_ptr = NULL;
		activator->context = context;

		*userData = activator;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	remoteProxyFactory_create(context, (char *) CALCULATOR_SERVICE, activator,
			calculatorProxyFactory_create, calculatorProxyFactory_destroy,
			&activator->factory_ptr);
	remoteProxyFactory_register(activator->factory_ptr);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	remoteProxyFactory_unregister(activator->factory_ptr);
	remoteProxyFactory_destroy(&activator->factory_ptr);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	free(activator);

	return status;
}

static celix_status_t calculatorProxyFactory_create(void *handle, endpoint_description_pt endpointDescription, remote_service_admin_pt rsa, sendToHandle sendToCallback, properties_pt properties, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = handle;

	calculator_service_pt calculatorService = calloc(1, sizeof(*calculatorService));
	calculatorProxy_create(activator->context, &calculatorService->calculator);
	calculatorService->add = calculatorProxy_add;
	calculatorService->sub = calculatorProxy_sub;
	calculatorService->sqrt = calculatorProxy_sqrt;

	calculatorService->calculator->endpoint = endpointDescription;
	calculatorService->calculator->sendToHandler = rsa;
	calculatorService->calculator->sendToCallback = sendToCallback;

	*service = calculatorService;

	return status;
}

static celix_status_t calculatorProxyFactory_destroy(void *handle, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	calculator_service_pt calculatorService = service;

	if (!calculatorService) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		calculatorProxy_destroy(&calculatorService->calculator);
		free(calculatorService);
	}

	return status;
}
