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
#include "celix_utils.h"
#include "celix_log_helper.h"

#include "discovery.h"
#include "discovery_impl.h"


celix_status_t discovery_create(celix_bundle_context_t *context, discovery_t **discovery) {
	celix_status_t status = CELIX_SUCCESS;

	*discovery = malloc(sizeof(struct discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	}
	else {
		(*discovery)->context = context;
		(*discovery)->poller = NULL;
		(*discovery)->server = NULL;
		(*discovery)->stopped = false;

		(*discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		(*discovery)->discoveredServices = hashMap_create(
            (unsigned int (*)(const void*))celix_utils_stringHash,
            NULL,
            (int (*)(const void*, const void*))celix_utils_stringEquals,
            NULL);

		celixThreadMutex_create(&(*discovery)->mutex, NULL);

        (*discovery)->loghelper = celix_logHelper_create(context, "celix_rsa_discovery");
	}

	return status;
}

celix_status_t discovery_start(discovery_t *discovery) {
    celix_status_t status;

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

	celixThreadMutex_lock(&discovery->mutex);
	discovery->stopped = true;
	celixThreadMutex_unlock(&discovery->mutex);

	status = endpointDiscoveryServer_destroy(discovery->server);
	status = endpointDiscoveryPoller_destroy(discovery->poller);

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

    celix_logHelper_destroy(discovery->loghelper);

	free(discovery);

	return status;
}
