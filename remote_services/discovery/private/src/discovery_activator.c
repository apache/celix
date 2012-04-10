/*
 * dependency_activator.c
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */

#include <stdio.h>
#include <stdlib.h>

#include <apr_strings.h>
#include <apr_uuid.h>

#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"
#include "constants.h"

#include "discovery.h"
#include "endpoint_listener.h"
#include "remote_constants.h"

struct activator {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;

	discovery_t discovery;

	SERVICE_TRACKER endpointListenerTracker;
	SERVICE_REGISTRATION endpointListenerService;
};

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator, SERVICE_TRACKER *tracker);
celix_status_t discoveryActivator_getUUID(struct activator *activator, char **uuidStr);

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
		activator->endpointListenerTracker = NULL;

		discovery_create(pool, context, &activator->discovery);

		discoveryActivator_createEPLTracker(activator, &activator->endpointListenerTracker);

		*userData = activator;
	}

	return status;
}

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	SERVICE_TRACKER_CUSTOMIZER custumizer = (SERVICE_TRACKER_CUSTOMIZER) apr_palloc(activator->pool, sizeof(*custumizer));
	if (!custumizer) {
		status = CELIX_ENOMEM;
	} else {
		custumizer->handle = activator->discovery;
		custumizer->addingService = discovery_endpointListenerAdding;
		custumizer->addedService = discovery_endpointListenerAdded;
		custumizer->modifiedService = discovery_endpointListenerModified;
		custumizer->removedService = discovery_endpointListenerRemoved;

		status = serviceTracker_create(activator->pool, activator->context, "endpoint_listener", custumizer, tracker);

		serviceTracker_open(activator->endpointListenerTracker);
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, activator->pool);

	endpoint_listener_t endpointListener = apr_palloc(pool, sizeof(*endpointListener));
	endpointListener->handle = activator->discovery;
	endpointListener->endpointAdded = discovery_endpointAdded;
	endpointListener->endpointRemoved = discovery_endpointRemoved;

	PROPERTIES props = properties_create();
	properties_set(props, "DISCOVERY", "true");
	char *uuid = NULL;
	discoveryActivator_getUUID(activator, &uuid);
	char *scope = apr_pstrcat(activator->pool, "(&(", OBJECTCLASS, "=*)(", ENDPOINT_FRAMEWORK_UUID, "=", uuid, "))", NULL);
	properties_set(props, (char *) ENDPOINT_LISTENER_SCOPE, scope);
	bundleContext_registerService(context, (char *) endpoint_listener_service, endpointListener, props, &activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	discovery_stop(activator->discovery);
	serviceTracker_close(activator->endpointListenerTracker);
	serviceRegistration_unregister(activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t discoveryActivator_getUUID(struct activator *activator, char **uuidStr) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleContext_getProperty(activator->context, ENDPOINT_FRAMEWORK_UUID, uuidStr);
	if (status == CELIX_SUCCESS) {
		if (*uuidStr == NULL) {
			apr_uuid_t uuid;
			apr_uuid_get(&uuid);
			*uuidStr = apr_palloc(activator->pool, APR_UUID_FORMATTED_LENGTH + 1);
			apr_uuid_format(*uuidStr, &uuid);
			setenv(ENDPOINT_FRAMEWORK_UUID, *uuidStr, 1);
		}
	}

	return status;
}
