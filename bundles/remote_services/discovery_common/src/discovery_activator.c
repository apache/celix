/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * discovery_activator.c
 *
 * \date        Aug 8, 2014
 * \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bundle_activator.h"
#include "service_tracker.h"
#include "celix_constants.h"

#include "celix_log_helper.h"
#include "discovery.h"
#include "remote_constants.h"

struct activator {
	celix_bundle_context_t *context;
	discovery_t *discovery;
    celix_log_helper_t *loghelper;

	service_tracker_t *endpointListenerTracker;
	endpoint_listener_t *endpointListener;
	service_registration_t *endpointListenerService;
};

celix_status_t bundleActivator_createEPLTracker(struct activator *activator, service_tracker_t **tracker) {
	celix_status_t status;

	service_tracker_customizer_t *customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->discovery, discovery_endpointListenerAdding, discovery_endpointListenerAdded, discovery_endpointListenerModified,
			discovery_endpointListenerRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, customizer, tracker);
	}

	return status;
}

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
	celix_status_t status;

	struct activator* activator = calloc(1,sizeof(struct activator));
	if (!activator) {
		return CELIX_ENOMEM;
	}

	status = discovery_create(context, &activator->discovery);
	if (status == CELIX_SUCCESS) {
		activator->context = context;

        activator->loghelper = celix_logHelper_create(context, "celix_rsa_discovery");

		status = bundleActivator_createEPLTracker(activator, &activator->endpointListenerTracker);
		if(status==CELIX_SUCCESS){
			*userData = activator;
		}
		else{
			bundleActivator_destroy(activator,context);
		}
	}
	else{
		free(activator);
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
	celix_status_t status;
	struct activator *activator = userData;
	const char *uuid = NULL;

	status = bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	if (!uuid) {
        celix_logHelper_debug(activator->loghelper, "no framework UUID defined?!");
		return CELIX_ILLEGAL_STATE;
	}

	size_t len = 11 + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(OSGI_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
	char *scope = malloc(len + 1);
	if (!scope) {
		return CELIX_ENOMEM;
	}

	sprintf(scope, "(&(%s=*)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
	scope[len] = 0;

    celix_logHelper_debug(activator->loghelper, "using scope %s.", scope);

	celix_properties_t *props = celix_properties_create();
	celix_properties_set(props, "DISCOVERY", "true");
	celix_properties_set(props, (char *) OSGI_ENDPOINT_LISTENER_SCOPE, scope);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_open(activator->endpointListenerTracker);
	}

	if (status == CELIX_SUCCESS) {
		status = discovery_start(activator->discovery);
	}

	if (status == CELIX_SUCCESS) {
		endpoint_listener_t *endpointListener = calloc(1, sizeof(struct endpoint_listener));

		if (endpointListener) {
			endpointListener->handle = activator->discovery;
			endpointListener->endpointAdded = discovery_endpointAdded;
			endpointListener->endpointRemoved = discovery_endpointRemoved;

			status = bundleContext_registerService(context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, endpointListener, props, &activator->endpointListenerService);

			if (status == CELIX_SUCCESS) {
				activator->endpointListener = endpointListener;
			}
		}
	}
	// We can release the scope, as celix_properties_set makes a copy of the key & value...
	free(scope);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
	celix_status_t status;
	struct activator *activator = userData;

	status = discovery_stop(activator->discovery);

	status = serviceTracker_close(activator->endpointListenerTracker);

	status = serviceRegistration_unregister(activator->endpointListenerService);
	free(activator->endpointListener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
	celix_status_t status;
	struct activator *activator = userData;

	status = serviceTracker_destroy(activator->endpointListenerTracker);

	status = discovery_destroy(activator->discovery);

    celix_logHelper_destroy(activator->loghelper);

	activator->loghelper = NULL;
	activator->endpointListenerTracker = NULL;
	activator->endpointListenerService = NULL;
	activator->discovery = NULL;
	activator->context = NULL;

	free(activator);

	return status;
}
