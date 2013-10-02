/*
 * client_impl.c
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#include <stdlib.h>

#include "client_impl.h"

celix_status_t client_create(apr_pool_t *pool, BUNDLE_CONTEXT context, client_t *client) {
	celix_status_t status = CELIX_SUCCESS;

	*client = apr_palloc(pool, sizeof(**client));
	if (!*client) {
		status = CELIX_ENOMEM;
	} else {
		(*client)->pool = pool;
		(*client)->server = NULL;
		(*client)->context = context;
	}

	return status;
}

celix_status_t client_doo(client_t client, int a, int b, int *reply) {
	if (client->server != NULL) {
		client->server->server_doo(client->server->server, a, b, reply);
	}
	return CELIX_SUCCESS;
}

celix_status_t client_addingService(void *clientP, SERVICE_REFERENCE reference, void **service) {
	client_t client = clientP;

	bundleContext_getService(client->context, reference, service);


	return CELIX_SUCCESS;
}

celix_status_t client_addedService(void *clientP, SERVICE_REFERENCE reference, void *service) {
	client_t client = clientP;

	printf("Added service\n");
	client->server = service;

	return CELIX_SUCCESS;
}

celix_status_t client_modifiedService(void *clientP, SERVICE_REFERENCE reference, void *service) {
	client_t client = clientP;

	client->server = service;

	return CELIX_SUCCESS;
}

celix_status_t client_removedService(void *clientP, SERVICE_REFERENCE reference, void *service) {
	client_t client = clientP;

	printf("Removed service\n");
	client->server = NULL;

	return CELIX_SUCCESS;
}
