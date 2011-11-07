/*
 * example_endpoint_activator.c
 *
 *  Created on: Oct 10, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include "bundle_activator.h"

#include "example_proxy_impl.h"

#include "service_registration.h"

struct activator {
	apr_pool_t *pool;

	SERVICE_REGISTRATION proxy;
	SERVICE_REGISTRATION service;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
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

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	example_t example;
	example_service_t exampleService;
	remote_proxy_service_t exampleProxy;

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
	bundleContext_registerService(context, REMOTE_PROXY, exampleProxy, NULL, &activator->proxy);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->proxy);
	serviceRegistration_unregister(activator->service);
	activator->proxy = NULL;
	activator->service = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
