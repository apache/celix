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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "echo_server_private.h"

struct echoActivator {
	service_registration_pt reg;
	echo_service_pt es;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct echoActivator * act = malloc(sizeof(*act));
	act->reg = NULL;
	*userData = act;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	echo_service_pt es = malloc(sizeof(*es));
	echo_server_pt server = echoServer_create();
	es->server = server;
	es->echo = echoServer_echo;

	act->es = es;

    bundleContext_registerService(context, ECHO_SERVICE_NAME, es, NULL, &act->reg);

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	serviceRegistration_unregister(act->reg);
	act->reg = NULL;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	act->es->echo = NULL;
	echoServer_destroy(act->es->server);
	act->es->server = NULL;
	free(act->es);
	act->es = NULL;
	act->reg = NULL;
	free(act);

	return CELIX_SUCCESS;
}
