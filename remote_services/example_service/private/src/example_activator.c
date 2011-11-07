/*
 * example_activator.c
 *
 *  Created on: Oct 5, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "example_impl.h"
#include "remote_constants.h"

struct activator {
	apr_pool_t *pool;
	SERVICE_REGISTRATION exampleReg;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *parentpool = NULL;
	apr_pool_t *pool = NULL;
	struct activator *activator;

	status = bundleContext_getMemoryPool(context, &parentpool);
	if (status == CELIX_SUCCESS) {
		if (apr_pool_create(&pool, parentpool) != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			activator = apr_palloc(pool, sizeof(*activator));
			activator->pool = pool;
			activator->exampleReg = NULL;

			*userData = activator;
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	example_t example = NULL;
	example_service_t service = NULL;
	PROPERTIES properties = NULL;

	status = example_create(activator->pool, &example);
	if (status == CELIX_SUCCESS) {
		service = apr_palloc(activator->pool, sizeof(*service));
		if (!service) {
			status = CELIX_ENOMEM;
		} else {
			service->example = example;
			service->add = example_add;
			service->sub = example_sub;
			service->sqrt = example_sqrt;

			properties = properties_create();
			properties_set(properties, (char *) SERVICE_EXPORTED_INTERFACES, (char *) EXAMPLE_SERVICE);

			bundleContext_registerService(context, (char *) EXAMPLE_SERVICE, service, properties, &activator->exampleReg);
		}
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->exampleReg);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;



	return status;
}
