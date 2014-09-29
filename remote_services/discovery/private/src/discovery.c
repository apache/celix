/**
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
/*
 * discovery.c
 *
 * \date        Aug 8, 2014
 * \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>

#include "constants.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"
#include "remote_constants.h"
#include "celix_log.h"
#include "discovery.h"
#include "discovery_impl.h"
#include "endpoint_discovery_poller.h"
#include "endpoint_discovery_server.h"


celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Endpoint for %s, with filter \"%s\" added...", endpoint->service, matchedFilter);

	status = endpointDiscoveryServer_addEndpoint(discovery->server, endpoint);

	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *matchedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Endpoint for %s, with filter \"%s\" removed...", endpoint->service, matchedFilter);

	status = endpointDiscoveryServer_removeEndpoint(discovery->server, endpoint);

	return status;
}

celix_status_t discovery_endpointListenerAdding(void* handle, service_reference_pt reference, void** service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	bundleContext_getService(discovery->context, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerAdded(void* handle, service_reference_pt reference, void* service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);

	properties_pt serviceProperties = NULL;
	serviceRegistration_getProperties(registration, &serviceProperties);

	char *discoveryListener = properties_get(serviceProperties, "DISCOVERY");
	char *scope = properties_get(serviceProperties, (char *) OSGI_ENDPOINT_LISTENER_SCOPE);
	filter_pt filter = filter_create(scope);

	if (discoveryListener != NULL && strcmp(discoveryListener, "true") == 0) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "EndpointListener Ignored - Discovery listener");
	} else {
		celixThreadMutex_lock(&discovery->discoveredServicesMutex);

		hash_map_iterator_pt iter = hashMapIterator_create(discovery->discoveredServices);
		while (hashMapIterator_hasNext(iter)) {
			endpoint_description_pt endpoint = hashMapIterator_nextKey(iter);

			bool matchResult = false;
			filter_match(filter, endpoint->properties, &matchResult);
			if (matchResult) {
				endpoint_listener_pt listener = service;

				fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "EndpointListener Added - Add Scope");

				listener->endpointAdded(listener, endpoint, NULL);
			}
		}
		hashMapIterator_destroy(iter);

		celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

		celixThreadMutex_lock(&discovery->listenerReferencesMutex);

		hashMap_put(discovery->listenerReferences, reference, NULL);

		celixThreadMutex_unlock(&discovery->listenerReferencesMutex);
	}

	filter_destroy(filter);

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	status = discovery_endpointListenerRemoved(handle, reference, service);
	status = discovery_endpointListenerAdded(handle, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	status = celixThreadMutex_lock(&discovery->listenerReferencesMutex);

	if (discovery->listenerReferences != NULL) {
		if (hashMap_remove(discovery->listenerReferences, reference)) {
			fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "EndpointListener Removed");
		}
	}

	status = celixThreadMutex_unlock(&discovery->listenerReferencesMutex);

	return status;
}

celix_status_t discovery_informEndpointListeners(discovery_pt discovery, endpoint_description_pt endpoint, bool endpointAdded) {
	celix_status_t status = CELIX_SUCCESS;

	// Inform listeners of new endpoint
	status = celixThreadMutex_lock(&discovery->listenerReferencesMutex);

	if (discovery->listenerReferences != NULL) {
		hash_map_iterator_pt iter = hashMapIterator_create(discovery->listenerReferences);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

			service_reference_pt reference = hashMapEntry_getKey(entry);
			endpoint_listener_pt listener = NULL;

			service_registration_pt registration = NULL;
			serviceReference_getServiceRegistration(reference, &registration);

			properties_pt serviceProperties = NULL;
			serviceRegistration_getProperties(registration, &serviceProperties);
			char *scope = properties_get(serviceProperties, (char *) OSGI_ENDPOINT_LISTENER_SCOPE);

			filter_pt filter = filter_create(scope);
			bool matchResult = false;

			status = filter_match(filter, endpoint->properties, &matchResult);
			if (matchResult) {
				bundleContext_getService(discovery->context, reference, (void**) &listener);
				if (endpointAdded) {
					fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Adding service (%s)", endpoint->service);

					listener->endpointAdded(listener->handle, endpoint, scope);
				} else {
					fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Removing service (%s)", endpoint->service);

					listener->endpointRemoved(listener->handle, endpoint, scope);
				}
			}
		}
		hashMapIterator_destroy(iter);
	}

	status = celixThreadMutex_unlock(&discovery->listenerReferencesMutex);

	return status;
}

celix_status_t discovery_addDiscoveredEndpoint(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	status = celixThreadMutex_lock(&discovery->discoveredServicesMutex);

	char* endpointId = endpoint->id;
	bool exists = hashMap_get(discovery->discoveredServices, endpointId) != NULL;
	if (!exists) {
		hashMap_put(discovery->discoveredServices, endpointId, endpoint);
	}

	status = celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

	if (!exists) {
		// notify our listeners that a new endpoint is available...
		discovery_informEndpointListeners(discovery, endpoint, true /* addingService */);
	}

	return status;
}

celix_status_t discovery_removeDiscoveredEndpoint(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	status = celixThreadMutex_lock(&discovery->discoveredServicesMutex);

	char* endpointId = endpoint->id;
	void* oldValue = hashMap_remove(discovery->discoveredServices, endpointId);

	status = celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

	if (oldValue) {
		status = discovery_informEndpointListeners(discovery, endpoint, false /* addingService */);
	}

	return status;
}
