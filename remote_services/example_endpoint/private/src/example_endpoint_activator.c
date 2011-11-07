/*
 * example_endpoint_activator.c
 *
 *  Created on: Oct 10, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include "bundle_activator.h"

#include "example_endpoint_impl.h"
#include "service_registration.h"

struct activator {
	apr_pool_t *pool;

	SERVICE_REGISTRATION endpoint;
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
				activator->endpoint = NULL;

				*userData = activator;
			}
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	remote_endpoint_t endpoint;
	remote_endpoint_service_t endpointService;

	exampleEndpoint_create(activator->pool, &endpoint);
	endpointService = apr_palloc(activator->pool, sizeof(*endpointService));
	endpointService->endpoint = endpoint;
	endpointService->handleRequest = exampleEndpoint_handleRequest;
	endpointService->setService = exampleEndpoint_setService;

	bundleContext_registerService(context, REMOTE_ENDPOINT, endpointService, NULL, &activator->endpoint);


	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->endpoint);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
