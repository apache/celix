/*
 * echo_server_activator.c
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"

#include "echo_server.h"
#include "echo_client_private.h"

struct echoActivator {
	ECHO_CLIENT client;
	SERVICE_TRACKER tracker;
};

void * bundleActivator_create() {
	struct echoActivator * act = malloc(sizeof(*act));
	act->client = NULL;
	act->tracker = NULL;

	return act;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct echoActivator * act = (struct echoActivator *) userData;

	SERVICE_TRACKER tracker = tracker_create(context, ECHO_SERVICE_NAME, NULL);
	act->tracker = tracker;

	ECHO_CLIENT client = echoClient_create(tracker);
	act->client = client;

	echoClient_start(act->client);
	tracker_open(act->tracker);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct echoActivator * act = (struct echoActivator *) userData;
	tracker_close(act->tracker);
	echoClient_stop(act->client);
}

void bundleActivator_destroy(void * userData) {
	struct echoActivator * act = (struct echoActivator *) userData;
	echoClient_destroy(act->client);
	tracker_destroy(act->tracker);
}
