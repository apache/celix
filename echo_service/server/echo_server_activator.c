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
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "echo_server_private.h"

struct echoActivator {
	SERVICE_REGISTRATION reg;
	ECHO_SERVICE es;
};

void * bundleActivator_create() {
	struct echoActivator * act = malloc(sizeof(*act));
	act->reg = NULL;
	return act;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	ECHO_SERVICE es = malloc(sizeof(*es));
	ECHO_SERVER server = echoServer_create();
	es->server = server;
	es->echo = echoServer_echo;

	act->es = es;

	SERVICE_REGISTRATION reg = bundleContext_registerService(context, ECHO_SERVICE_NAME, es, NULL);
	act->reg = reg;
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	serviceRegistration_unregister(act->reg);
	act->reg = NULL;
}

void bundleActivator_destroy(void * userData) {
	struct echoActivator * act = (struct echoActivator *) userData;
	act->es->echo = NULL;
	echoServer_destroy(act->es->server);
	act->es->server = NULL;
	free(act->es);
	act->es = NULL;
	act->reg = NULL;
	free(act);
}
