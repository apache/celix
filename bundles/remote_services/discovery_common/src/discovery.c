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
 * discovery.c
 *
 * \date        Aug 8, 2014
 * \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>

#include "celix_filter.h"
#include "filter.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include "celix_log_helper.h"
#include "discovery.h"
#include "endpoint_discovery_server.h"


celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status;
	discovery_t *discovery = handle;

	celix_logHelper_info(discovery->loghelper, "Endpoint for %s, with filter \"%s\" added...", endpoint->service, matchedFilter);

	status = endpointDiscoveryServer_addEndpoint(discovery->server, endpoint);

	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
	celix_status_t status;
	discovery_t *discovery = handle;

    celix_logHelper_info(discovery->loghelper, "Endpoint for %s, with filter \"%s\" removed...", endpoint->service, matchedFilter);

	status = endpointDiscoveryServer_removeEndpoint(discovery->server, endpoint);

	return status;
}

celix_status_t discovery_endpointListenerAdding(void* handle, service_reference_pt reference, void** service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t *discovery = handle;

	bundleContext_getService(discovery->context, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerAdded(void* handle, service_reference_pt reference, void* service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t *discovery = handle;

	const char *discoveryListener = NULL;
	serviceReference_getProperty(reference, "DISCOVERY", &discoveryListener);
	const char *scope = NULL;
	serviceReference_getProperty(reference, OSGI_ENDPOINT_LISTENER_SCOPE, &scope);

	celix_filter_t *filter = celix_filter_create(scope);

	if (discoveryListener != NULL && strcmp(discoveryListener, "true") == 0) {
        celix_logHelper_info(discovery->loghelper, "EndpointListener Ignored - Discovery listener");
	} else {
		celixThreadMutex_lock(&discovery->discoveredServicesMutex);

		hash_map_iterator_pt iter = hashMapIterator_create(discovery->discoveredServices);
		while (hashMapIterator_hasNext(iter)) {
			endpoint_description_t *endpoint = hashMapIterator_nextValue(iter);

			bool matchResult = false;
			filter_match(filter, endpoint->properties, &matchResult);
			if (matchResult) {
				endpoint_listener_t *listener = service;

                celix_logHelper_info(discovery->loghelper, "EndpointListener Added - Add Scope");

				listener->endpointAdded(listener->handle, endpoint, NULL);
			}
		}
		hashMapIterator_destroy(iter);

		celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

		celixThreadMutex_lock(&discovery->listenerReferencesMutex);

		hashMap_put(discovery->listenerReferences, reference, NULL);

		celixThreadMutex_unlock(&discovery->listenerReferencesMutex);
	}

	celix_filter_destroy(filter);

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status;

	status = discovery_endpointListenerRemoved(handle, reference, service);
	if (status == CELIX_SUCCESS) {
        status = discovery_endpointListenerAdded(handle, reference, service);
	}

	return status;
}

celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status;
    discovery_t *discovery = handle;

    status = celixThreadMutex_lock(&discovery->listenerReferencesMutex);

    if (status == CELIX_SUCCESS) {
        if (discovery->listenerReferences != NULL) {
            if (hashMap_remove(discovery->listenerReferences, reference)) {
                celix_logHelper_info(discovery->loghelper, "EndpointListener Removed");
            }
        }

        status = celixThreadMutex_unlock(&discovery->listenerReferencesMutex);
    }

	return status;
}

celix_status_t discovery_informEndpointListeners(discovery_t *discovery, endpoint_description_t *endpoint, bool endpointAdded) {
	celix_status_t status;

	// Inform listeners of new endpoint
	status = celixThreadMutex_lock(&discovery->listenerReferencesMutex);

    if (status == CELIX_SUCCESS) {
        if (discovery->listenerReferences != NULL) {
            hash_map_iterator_pt iter = hashMapIterator_create(discovery->listenerReferences);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

                service_reference_pt reference = hashMapEntry_getKey(entry);
                endpoint_listener_t *listener = NULL;

                const char* scope = NULL;
                serviceReference_getProperty(reference, OSGI_ENDPOINT_LISTENER_SCOPE, &scope);

                celix_filter_t *filter = celix_filter_create(scope);
                bool matchResult = celix_filter_match(filter, endpoint->properties);
                if (matchResult) {
                    bundleContext_getService(discovery->context, reference, (void **) &listener);
                    if (endpointAdded) {
                        celix_logHelper_debug(discovery->loghelper, "Adding service (%s)", endpoint->service);

                        listener->endpointAdded(listener->handle, endpoint, (char*)scope);
                    } else {
                        celix_logHelper_debug(discovery->loghelper, "Removing service (%s)", endpoint->service);

                        listener->endpointRemoved(listener->handle, endpoint, (char*)scope);
                    }
                }

                celix_filter_destroy(filter);
            }
            hashMapIterator_destroy(iter);
        }

        status = celixThreadMutex_unlock(&discovery->listenerReferencesMutex);
    }

	return status;
}

celix_status_t discovery_addDiscoveredEndpoint(discovery_t *discovery, endpoint_description_t *endpoint) {
	celix_status_t status;

	status = celixThreadMutex_lock(&discovery->discoveredServicesMutex);

    if (status == CELIX_SUCCESS) {
        char *endpointId = endpoint->id;
        bool exists = hashMap_get(discovery->discoveredServices, endpointId) != NULL;
        if (!exists) {
            hashMap_put(discovery->discoveredServices, endpointId, endpoint);
        }

        status = celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

        if (!exists) {
            // notify our listeners that a new endpoint is available...
            discovery_informEndpointListeners(discovery, endpoint, true /* addingService */);
        }
    }

	return status;
}

celix_status_t discovery_removeDiscoveredEndpoint(discovery_t *discovery, endpoint_description_t *endpoint) {
	celix_status_t status;

	status = celixThreadMutex_lock(&discovery->discoveredServicesMutex);

    if (status == CELIX_SUCCESS) {
        char *endpointId = endpoint->id;
        void *oldValue = hashMap_remove(discovery->discoveredServices, endpointId);

        status = celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

        if (oldValue) {
            status = discovery_informEndpointListeners(discovery, endpoint, false /* removeService */);
        }
    }

	return status;
}
