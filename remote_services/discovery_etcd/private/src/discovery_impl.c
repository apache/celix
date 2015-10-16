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


#include "discovery.h"
#include "discovery_impl.h"
#include "etcd_watcher.h"
#include "endpoint_discovery_poller.h"
#include "endpoint_discovery_server.h"



celix_status_t discovery_create(bundle_context_pt context, discovery_pt *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	*discovery = malloc(sizeof(struct discovery));
	if (!*discovery) {
		return CELIX_ENOMEM;
	}

	(*discovery)->context = context;
	(*discovery)->poller = NULL;
	(*discovery)->server = NULL;

	(*discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*discovery)->discoveredServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	status = celixThreadMutex_create(&(*discovery)->listenerReferencesMutex, NULL);
	status = celixThreadMutex_create(&(*discovery)->discoveredServicesMutex, NULL);

	logHelper_create(context, &(*discovery)->loghelper);

	return status;
}



celix_status_t discovery_destroy(discovery_pt discovery) {
	celix_status_t status = CELIX_SUCCESS;

	discovery->context = NULL;
	discovery->poller = NULL;
	discovery->server = NULL;

	celixThreadMutex_lock(&discovery->discoveredServicesMutex);

	hashMap_destroy(discovery->discoveredServices, false, false);
	discovery->discoveredServices = NULL;

	celixThreadMutex_unlock(&discovery->discoveredServicesMutex);

	celixThreadMutex_destroy(&discovery->discoveredServicesMutex);

	celixThreadMutex_lock(&discovery->listenerReferencesMutex);

	hashMap_destroy(discovery->listenerReferences, false, false);
	discovery->listenerReferences = NULL;

	celixThreadMutex_unlock(&discovery->listenerReferencesMutex);

	celixThreadMutex_destroy(&discovery->listenerReferencesMutex);

	logHelper_destroy(&discovery->loghelper);

	free(discovery);

	return status;
}

celix_status_t discovery_start(discovery_pt discovery) {
    celix_status_t status = CELIX_SUCCESS;
	char *port = NULL;
	char *path = NULL;

	logHelper_start(discovery->loghelper);

	bundleContext_getProperty(discovery->context, DISCOVERY_SERVER_PORT, &port);
	if (port == NULL) {
		port = DEFAULT_SERVER_PORT;
	}

	bundleContext_getProperty(discovery->context, DISCOVERY_SERVER_PATH, &path);
	if (path == NULL) {
		path = DEFAULT_SERVER_PATH;
	}

    status = endpointDiscoveryPoller_create(discovery, discovery->context, &discovery->poller);
    if (status != CELIX_SUCCESS) {
    	return CELIX_BUNDLE_EXCEPTION;
    }

    status = endpointDiscoveryServer_create(discovery, discovery->context, &discovery->server);
    if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
    }

    status = etcdWatcher_create(discovery, discovery->context, &discovery->watcher);
    if (status != CELIX_SUCCESS) {
    	return CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t discovery_stop(discovery_pt discovery) {
	celix_status_t status;

	status = etcdWatcher_destroy(discovery->watcher);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	status = endpointDiscoveryServer_destroy(discovery->server);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	status = endpointDiscoveryPoller_destroy(discovery->poller);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}
	hash_map_iterator_pt iter;

	celixThreadMutex_lock(&discovery->discoveredServicesMutex);

	iter = hashMapIterator_create(discovery->discoveredServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		endpoint_description_pt endpoint = hashMapEntry_getValue(entry);

		discovery_informEndpointListeners(discovery, endpoint, false);
	}
	hashMapIterator_destroy(iter);

	celixThreadMutex_unlock(&discovery->discoveredServicesMutex);


	logHelper_stop(discovery->loghelper);

	return status;
}



