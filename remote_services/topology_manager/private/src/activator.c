/*
 * dependency_activator.c
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */

#include <stdio.h>
#include <stdlib.h>

#include "headers.h"
#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"

#include "topology_manager.h"
#include "endpoint_listener.h"

struct activator {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;

	topology_manager_t manager;

	SERVICE_TRACKER remoteServiceAdminTracker;
	SERVICE_LISTENER serviceListener;

	SERVICE_REGISTRATION endpointListenerService;
};

celix_status_t bundleActivator_createRSATracker(struct activator *activator, SERVICE_TRACKER *tracker);
celix_status_t bundleActivator_createServiceListener(struct activator *activator, SERVICE_LISTENER *listener);

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *parentPool = NULL;
	apr_pool_t *pool = NULL;
	struct activator *activator = NULL;

	bundleContext_getMemoryPool(context, &parentPool);
	apr_pool_create(&pool, parentPool);
	activator = apr_palloc(pool, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	} else {
		activator->pool = pool;
		activator->context = context;

		topologyManager_create(context, pool, &activator->manager);

		bundleActivator_createRSATracker(activator, &activator->remoteServiceAdminTracker);
		bundleActivator_createServiceListener(activator, &activator->serviceListener);

		*userData = activator;
	}

	return status;
}

celix_status_t bundleActivator_createRSATracker(struct activator *activator, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	SERVICE_TRACKER_CUSTOMIZER custumizer = (SERVICE_TRACKER_CUSTOMIZER) apr_palloc(activator->pool, sizeof(*custumizer));
	if (!custumizer) {
		status = CELIX_ENOMEM;
	} else {
		custumizer->handle = activator->manager;
		custumizer->addingService = topologyManager_rsaAdding;
		custumizer->addedService = topologyManager_rsaAdded;
		custumizer->modifiedService = topologyManager_rsaModified;
		custumizer->removedService = topologyManager_rsaRemoved;

		status = serviceTracker_create(activator->context, "remote_service_admin", custumizer, tracker);
	}

	return status;
}

celix_status_t bundleActivator_createServiceListener(struct activator *activator, SERVICE_LISTENER *listener) {
	celix_status_t status = CELIX_SUCCESS;

	*listener = apr_palloc(activator->pool, sizeof(*listener));
	if (!*listener) {
		status = CELIX_ENOMEM;
	} else {
		(*listener)->handle = activator->manager;
		(*listener)->serviceChanged = topologyManager_serviceChanged;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, activator->pool);

	endpoint_listener_t endpointListener = apr_palloc(pool, sizeof(*endpointListener));
	endpointListener->handle = activator->manager;
	endpointListener->endpointAdded = topologyManager_endpointAdded;
	endpointListener->endpointRemoved = topologyManager_endpointRemoved;
	bundleContext_registerService(context, (char *) endpoint_listener_service, endpointListener, NULL, &activator->endpointListenerService);

	bundleContext_addServiceListener(context, activator->serviceListener, NULL);
	serviceTracker_open(activator->remoteServiceAdminTracker);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->endpointListenerService);
	serviceTracker_close(activator->remoteServiceAdminTracker);
	bundleContext_removeServiceListener(context, activator->serviceListener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
