/*
 * server_activator.c
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "bundle_context.h"

#include "server_impl.h"

struct userData {
	apr_pool_t *pool;
	SERVICE_REGISTRATION serverReg;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct userData));
		((struct userData *)(*userData))->serverReg = NULL;
		((struct userData *)(*userData))->pool = pool;
	} else {
		status = CELIX_START_ERROR;
	}
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	server_service_t serverService = NULL;
	server_t server = NULL;
	server_create(data->pool, &server);

	serverService = apr_palloc(data->pool, sizeof(*serverService));
	serverService->server = server;
	serverService->server_doo = server_doo;

	bundleContext_registerService(context, SERVER_NAME, serverService, NULL, &data->serverReg);


	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	serviceRegistration_unregister(data->serverReg);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
