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

#include "constants.h"
#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"

#include "topology_manager.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "listener_hook_service.h"

#include "remote_services_utils.h"

struct activator {
	apr_pool_t *pool;
	bundle_context_t context;

	topology_manager_t manager;

	service_tracker_t remoteServiceAdminTracker;
	SERVICE_LISTENER serviceListener;

	SERVICE_REGISTRATION endpointListenerService;
	SERVICE_REGISTRATION hook;
};

static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_t *tracker);
static celix_status_t bundleActivator_createServiceListener(struct activator *activator, SERVICE_LISTENER *listener);

celix_status_t bundleActivator_create(bundle_context_t context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *parentPool = NULL;
	apr_pool_t *pool = NULL;
	struct activator *activator = NULL;

	bundleContext_getMemoryPool(context, &parentPool);
	if (apr_pool_create(&pool, parentPool) != APR_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		activator = apr_palloc(pool, sizeof(*activator));
		if (!activator) {
			status = CELIX_ENOMEM;
		} else {
			activator->pool = pool;
			activator->context = context;
			activator->endpointListenerService = NULL;
			activator->hook = NULL;
			activator->manager = NULL;
			activator->remoteServiceAdminTracker = NULL;
			activator->serviceListener = NULL;

			status = topologyManager_create(context, pool, &activator->manager);
			if (status == CELIX_SUCCESS) {
				status = bundleActivator_createRSATracker(activator, &activator->remoteServiceAdminTracker);
				if (status == CELIX_SUCCESS) {
					status = bundleActivator_createServiceListener(activator, &activator->serviceListener);
					if (status == CELIX_SUCCESS) {
						*userData = activator;
					}
				}
			}
		}
	}

	return status;
}

static celix_status_t bundleActivator_createRSATracker(struct activator *activator, service_tracker_t *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_t customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->pool, activator->manager, topologyManager_rsaAdding,
			topologyManager_rsaAdded, topologyManager_rsaModified, topologyManager_rsaRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->pool, activator->context, "remote_service_admin", customizer, tracker);
	}

	return status;
}

static celix_status_t bundleActivator_createServiceListener(struct activator *activator, SERVICE_LISTENER *listener) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool;
	apr_pool_create(&pool, activator->pool);
	*listener = apr_palloc(pool, sizeof(*listener));
	if (!*listener) {
		status = CELIX_ENOMEM;
	} else {
		(*listener)->pool = pool;
		(*listener)->handle = activator->manager;
		(*listener)->serviceChanged = topologyManager_serviceChanged;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_t context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, activator->pool);

	endpoint_listener_t endpointListener = apr_palloc(pool, sizeof(*endpointListener));
	endpointListener->handle = activator->manager;
	endpointListener->endpointAdded = topologyManager_endpointAdded;
	endpointListener->endpointRemoved = topologyManager_endpointRemoved;

	PROPERTIES props = properties_create();
	char *uuid = NULL;
	remoteServicesUtils_getUUID(pool, context, &uuid);
	char *scope = apr_pstrcat(pool, "(&(", OBJECTCLASS, "=*)(!(", ENDPOINT_FRAMEWORK_UUID, "=", uuid, ")))", NULL);
	properties_set(props, (char *) ENDPOINT_LISTENER_SCOPE, scope);

	bundleContext_registerService(context, (char *) endpoint_listener_service, endpointListener, props, &activator->endpointListenerService);

	listener_hook_service_t hook = apr_palloc(pool, sizeof(*hook));
	hook->handle = activator->manager;
	hook->added = topologyManager_listenerAdded;
	hook->removed = topologyManager_listenerRemoved;

	bundleContext_registerService(context, (char *) listener_hook_service_name, hook, NULL, &activator->hook);

	bundleContext_addServiceListener(context, activator->serviceListener, NULL);
	serviceTracker_open(activator->remoteServiceAdminTracker);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_t context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceTracker_close(activator->remoteServiceAdminTracker);
	bundleContext_removeServiceListener(context, activator->serviceListener);
	serviceRegistration_unregister(activator->hook);
	serviceRegistration_unregister(activator->endpointListenerService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_t context) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
