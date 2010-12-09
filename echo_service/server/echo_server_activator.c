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
