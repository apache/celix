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

#include "calculator_proxy_impl.h"

struct activator {
	apr_pool_t *pool;
	service_registration_pt proxyFactoryService;
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
				activator->proxyFactoryService = NULL;

				*userData = activator;
			}
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	remote_proxy_factory_service_pt calculatorProxyFactoryService;

	calculatorProxyFactoryService = apr_palloc(activator->pool, sizeof(*calculatorProxyFactoryService));
	calculatorProxyFactoryService->pool = activator->pool;
	calculatorProxyFactoryService->context = context;
	calculatorProxyFactoryService->proxy_registrations = hashMap_create(NULL, NULL, NULL, NULL);
	calculatorProxyFactoryService->registerProxyService = calculatorProxy_registerProxyService;
	calculatorProxyFactoryService->unregisterProxyService = calculatorProxy_unregisterProxyService;

	properties_pt props = properties_create();
	properties_set(props, (char *) "proxy.interface", (char *) CALCULATOR_SERVICE);

	if (bundleContext_registerService(context, OSGI_RSA_REMOTE_PROXY_FACTORY, calculatorProxyFactoryService, props, &activator->proxyFactoryService) == CELIX_SUCCESS);
	{
		printf("CALCULATOR_PROXY: Proxy registered OSGI_RSA_REMOTE_PROXY_FACTORY (%s)\n", OSGI_RSA_REMOTE_PROXY_FACTORY);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	// TODO: unregister proxy registrations
	serviceRegistration_unregister(activator->proxyFactoryService);
	activator->proxyFactoryService = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}
