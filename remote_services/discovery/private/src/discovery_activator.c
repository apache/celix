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
 * dependency_activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
	bundle_context_t context;

	discovery_t discovery;

	service_tracker_t endpointListenerTracker;
	service_registration_t endpointListenerService;
};

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator, service_tracker_t *tracker);
celix_status_t discoveryActivator_getUUID(struct activator *activator, char **uuidStr);

celix_status_t bundleActivator_create(bundle_context_t context, void **userData) {
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

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator,  service_tracker_t *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_t customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->pool, activator->discovery, discovery_endpointListenerAdding,
			discovery_endpointListenerAdded, discovery_endpointListenerModified, discovery_endpointListenerRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->pool, activator->context, "endpoint_listener", customizer, tracker);

		serviceTracker_open(activator->endpointListenerTracker);
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_t context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, activator->pool);

	endpoint_listener_t endpointListener = apr_palloc(pool, sizeof(*endpointListener));
	endpointListener->handle = activator->discovery;
	endpointListener->endpointAdded = discovery_endpointAdded;
	endpointListener->endpointRemoved = discovery_endpointRemoved;

	properties_t props = properties_create();
	properties_set(props, "DISCOVERY", "true");
	char *uuid = NULL;
	discoveryActivator_getUUID(activator, &uuid);
	char *scope = apr_pstrcat(activator->pool, "(&(", OBJECTCLASS, "=*)(", ENDPOINT_FRAMEWORK_UUID, "=", uuid, "))", NULL);
	properties_set(props, (char *) ENDPOINT_LISTENER_SCOPE, scope);
	bundleContext_registerService(context, (char *) endpoint_listener_service, endpointListener, props, &activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_t context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	discovery_stop(activator->discovery);
	serviceTracker_close(activator->endpointListenerTracker);
	serviceRegistration_unregister(activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_t context) {
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
