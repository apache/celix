/*
 * client_activator.c
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"

#include "client_impl.h"

struct userData {
	apr_pool_t *pool;
	SERVICE_TRACKER tracker;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct userData));
		((struct userData *)(*userData))->pool = pool;
		((struct userData *)(*userData))->tracker = NULL;
	} else {
		status = CELIX_START_ERROR;
	}
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	client_t client = NULL;
	client_create(data->pool, context, &client);

	SERVICE_TRACKER_CUSTOMIZER cust = (SERVICE_TRACKER_CUSTOMIZER) apr_palloc(data->pool, sizeof(*cust));
	cust->handle = client;
	cust->addedService = client_addedService;
	cust->addingService = client_addingService;
	cust->modifiedService = client_modifiedService;
	cust->removedService = client_removedService;
	serviceTracker_create(data->pool, context, "server", cust, &data->tracker);
	serviceTracker_open(data->tracker);

	int reply;
	client_doo(client, 1, 2, &reply);
	printf("Reply: %d\n", reply);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;

	serviceTracker_close(data->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
