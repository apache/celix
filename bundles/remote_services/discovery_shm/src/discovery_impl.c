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
 * discovery_impl.c
 *
 * \date        Aug 8, 2014
 * \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>

#include "celix_constants.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"
#include "remote_constants.h"


#include "discovery.h"
#include "discovery_impl.h"
#include "discovery_shmWatcher.h"
#include "endpoint_discovery_poller.h"
#include "endpoint_discovery_server.h"

celix_status_t discovery_create(celix_bundle_context_t *context, discovery_t** out) {
	celix_status_t status = CELIX_SUCCESS;

	discovery_t* discovery = calloc(1, sizeof(*discovery));
    discovery_impl_t* pImpl = calloc(1, sizeof(*pImpl));
	if (discovery != NULL && pImpl != NULL) {
        discovery->pImpl = pImpl;
        discovery->context = context;
        discovery->poller = NULL;
        discovery->server = NULL;
        discovery->stopped = false;

        celixThreadMutex_create(&discovery->mutex, NULL);

        discovery->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
        discovery->discoveredServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        discovery->loghelper = celix_logHelper_create(context, "rsa_discovery");
    } else {
        status = CELIX_ENOMEM;
        free(discovery);
        free(pImpl);
    }

    if (status == CELIX_SUCCESS) {
        *out = discovery;
    }

	return status;
}



celix_status_t discovery_destroy(discovery_t *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	discovery->context = NULL;
	discovery->poller = NULL;
	discovery->server = NULL;

	celixThreadMutex_lock(&discovery->mutex);

	hashMap_destroy(discovery->discoveredServices, false, false);
	discovery->discoveredServices = NULL;

	hashMap_destroy(discovery->listenerReferences, false, false);
	discovery->listenerReferences = NULL;

	celixThreadMutex_unlock(&discovery->mutex);

    celixThreadMutex_destroy(&discovery->mutex);


    free(discovery->pImpl);
	free(discovery);

	return status;
}

celix_status_t discovery_start(discovery_t *discovery) {
    celix_status_t status;

    status = endpointDiscoveryPoller_create(discovery, discovery->context, DEFAULT_POLL_ENDPOINTS, &discovery->poller);
    if (status == CELIX_SUCCESS) {
        status = endpointDiscoveryServer_create(discovery, discovery->context, DEFAULT_SERVER_PATH, DEFAULT_SERVER_PORT, DEFAULT_SERVER_IP, &discovery->server);
    }

    if (status == CELIX_SUCCESS) {
        status = discoveryShmWatcher_create(discovery);
    }

    return status;
}

celix_status_t discovery_stop(discovery_t *discovery) {
	celix_status_t status;

    celixThreadMutex_lock(&discovery->mutex);
    discovery->stopped = true;
    celixThreadMutex_unlock(&discovery->mutex);

    status = discoveryShmWatcher_destroy(discovery);

    if (status == CELIX_SUCCESS) {
        status = endpointDiscoveryServer_destroy(discovery->server);
    }

	endpointDiscoveryPoller_destroy(discovery->poller);

	if (status == CELIX_SUCCESS) {
        hash_map_iterator_pt iter;

        celixThreadMutex_lock(&discovery->mutex);

        iter = hashMapIterator_create(discovery->discoveredServices);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
            endpoint_description_t *endpoint = hashMapEntry_getValue(entry);

            discovery_informEndpointListeners(discovery, endpoint, false);
        }
        hashMapIterator_destroy(iter);

        celixThreadMutex_unlock(&discovery->mutex);

        celix_logHelper_destroy(discovery->loghelper);
	}

	return status;
}



