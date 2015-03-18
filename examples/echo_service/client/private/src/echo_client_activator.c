#include <sys/cdefs.h>/**
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
 * echo_client_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "service_tracker.h"

#include "echo_server.h"
#include "echo_client_private.h"

struct echoActivator {
	echo_client_pt client;
	service_tracker_pt tracker;
};

celix_status_t bundleActivator_create(bundle_context_pt __attribute__((unused)) context, void **userData) {
	struct echoActivator * act = malloc(sizeof(*act));
	act->client = NULL;
	act->tracker = NULL;
	*userData = act;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	service_tracker_pt tracker = NULL;
	echo_client_pt client = NULL;

	serviceTracker_create(context, ECHO_SERVICE_NAME, NULL, &tracker);

	act->tracker = tracker;

	client = echoClient_create(tracker);
	act->client = client;

	echoClient_start(act->client);
	serviceTracker_open(act->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt __attribute__((unused)) context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	serviceTracker_close(act->tracker);
	echoClient_stop(act->client);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	serviceTracker_destroy(act->tracker);
	echoClient_destroy(act->client);

	free(act);

	return CELIX_SUCCESS;
}
