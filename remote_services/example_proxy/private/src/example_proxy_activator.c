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
 * example_endpoint_activator.c
 *
 *  \date       Oct 10, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"

#include "example_proxy_impl.h"

#include "service_registration.h"

struct activator {
	apr_pool_t *pool;

	service_registration_pt proxy;
	service_registration_pt service;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *parentPool = NULL;
	apr_pool_t *pool = NULL;
	struct activator *activator;

	status = bundleContext_getMemoryPool(context, &parentPool);
	if (status == CELIX_SUCCESS) {
		if (apr_pool_create(&pool, parentPool) != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			activator = apr_palloc(pool, sizeof(*activator));
			if (!activator) {
				status = CELIX_ENOMEM;
			} else {
				activator->pool = pool;
				activator->proxy = NULL;
				activator->service = NULL;

				*userData = activator;
			}
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	example_pt example;
	example_service_pt exampleService;
	remote_proxy_service_pt exampleProxy;

	exampleProxy_create(activator->pool, &example);
	exampleService = apr_palloc(activator->pool, sizeof(*exampleService));
	exampleService->example = example;
	exampleService->add = exampleProxy_add;
	exampleService->sub = exampleProxy_sub;
	exampleService->sqrt = exampleProxy_sqrt;
	exampleProxy = apr_palloc(activator->pool, sizeof(*exampleProxy));
	exampleProxy->proxy = example;
	exampleProxy->setEndpointDescription = exampleProxy_setEndpointDescription;

	char **services = malloc(2);
	services[0] = EXAMPLE_SERVICE;
	services[1] = REMOTE_PROXY;

	bundleContext_registerService(context, EXAMPLE_SERVICE, exampleService, NULL, &activator->service);

	properties_pt props = properties_create();
	properties_set(props, (char *) "proxy.interface", (char *) EXAMPLE_SERVICE);
	bundleContext_registerService(context, REMOTE_PROXY, exampleProxy, props, &activator->proxy);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->proxy);
	serviceRegistration_unregister(activator->service);
	activator->proxy = NULL;
	activator->service = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
