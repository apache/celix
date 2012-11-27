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
 * echo_server_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"

#include "echo_server.h"
#include "echo_client_private.h"

struct echoActivator {
	ECHO_CLIENT client;
	service_tracker_t tracker;
};

celix_status_t bundleActivator_create(bundle_context_t context, void **userData) {
	struct echoActivator * act = malloc(sizeof(*act));
	act->client = NULL;
	act->tracker = NULL;

	*userData = act;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_t context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	apr_pool_t *pool = NULL;
	service_tracker_t tracker = NULL;
	ECHO_CLIENT client = NULL;

	bundleContext_getMemoryPool(context, &pool);
	serviceTracker_create(pool, context, ECHO_SERVICE_NAME, NULL, &tracker);
	act->tracker = tracker;

	client = echoClient_create(tracker, pool);
	act->client = client;

	echoClient_start(act->client);
	serviceTracker_open(act->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_t context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	serviceTracker_close(act->tracker);
	echoClient_stop(act->client);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_t context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	echoClient_destroy(act->client);

	return CELIX_SUCCESS;
}
