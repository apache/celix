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
	bundle_context_pt context;

	discovery_pt discovery;

	service_tracker_pt endpointListenerTracker;
	service_registration_pt endpointListenerService;
};

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator, service_tracker_pt *tracker);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
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
		activator->endpointListenerService = NULL;

		discovery_create(pool, context, &activator->discovery);

		discoveryActivator_createEPLTracker(activator, &activator->endpointListenerTracker);

		*userData = activator;
	}

	return status;
}

celix_status_t discoveryActivator_createEPLTracker(struct activator *activator,  service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->pool, activator->discovery, discovery_endpointListenerAdding,
			discovery_endpointListenerAdded, discovery_endpointListenerModified, discovery_endpointListenerRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->pool, activator->context, "endpoint_listener", customizer, tracker);

		serviceTracker_open(activator->endpointListenerTracker);
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, activator->pool);

	endpoint_listener_pt endpointListener = apr_palloc(pool, sizeof(*endpointListener));
	endpointListener->handle = activator->discovery;
	endpointListener->endpointAdded = discovery_endpointAdded;
	endpointListener->endpointRemoved = discovery_endpointRemoved;

	properties_pt props = properties_create();
	properties_set(props, "DISCOVERY", "true");
	char *uuid = NULL;
	bundleContext_getProperty(activator->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	char *scope = apr_pstrcat(activator->pool, "(&(", OSGI_FRAMEWORK_OBJECTCLASS, "=*)(", OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, "=", uuid, "))", NULL);
	properties_set(props, (char *) OSGI_ENDPOINT_LISTENER_SCOPE, scope);
	status = bundleContext_registerService(context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, endpointListener, props, &activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	discovery_stop(activator->discovery);
	serviceTracker_close(activator->endpointListenerTracker);
	serviceRegistration_unregister(activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
