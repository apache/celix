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
#include <stdbool.h>
#include <netdb.h>

#include "celix_threads.h"
#include "bundle_context.h"
#include "utils.h"
#include "log_helper.h"

#include "discovery.h"
#include "discovery_impl.h"


celix_status_t discovery_create(celix_bundle_context_t *context, discovery_t **discovery) {
	celix_status_t status;

	*discovery = malloc(sizeof(struct discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	}
	else {
		(*discovery)->context = context;
		(*discovery)->poller = NULL;
		(*discovery)->server = NULL;

		(*discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		(*discovery)->discoveredServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

		status = celixThreadMutex_create(&(*discovery)->listenerReferencesMutex, NULL);
		status = celixThreadMutex_create(&(*discovery)->discoveredServicesMutex, NULL);

		logHelper_create(context, &(*discovery)->loghelper);
	}

	return status;
}

celix_status_t discovery_start(discovery_t *discovery) {
    celix_status_t status;

	logHelper_start(discovery->loghelper);

    status = endpointDiscoveryPoller_create(discovery, discovery->context, DEFAULT_POLL_ENDPOINTS, &discovery->poller);
    if (status != CELIX_SUCCESS) {
    	return CELIX_BUNDLE_EXCEPTION;
    }

    status = endpointDiscoveryServer_create(discovery, discovery->context, DEFAULT_SERVER_PATH, DEFAULT_SERVER_PORT, DEFAULT_SERVER_IP, &discovery->server);
    if (status != CELIX_SUCCESS) {
    	return CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t discovery_stop(discovery_t *discovery) {
	celix_status_t status;

	status = endpointDiscoveryServer_destroy(discovery->server);
	status = endpointDiscoveryPoller_destroy(discovery->poller);

	logHelper_stop(discovery->loghelper);

	return status;
}

celix_status_t discovery_destroy(discovery_t *discovery) {
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
