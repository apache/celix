/*
 * echo_client.c
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "service_tracker.h"

#include "echo_client_private.h"
#include "echo_server.h"

void * trk_send(void * handle) {
	ECHO_CLIENT client = (ECHO_CLIENT) handle;
	while (client->running) {
		ECHO_SERVICE service = (ECHO_SERVICE) tracker_getService(client->tracker);
		if (service != NULL) {
			service->echo(service->server, "hi");
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

ECHO_CLIENT echoClient_create(SERVICE_TRACKER echoServiceTracker) {
	ECHO_CLIENT client = malloc(sizeof(*client));

	client->tracker = echoServiceTracker;
	client->running = false;

	return client;
}

void echoClient_start(ECHO_CLIENT client) {
	client->running = true;
	pthread_create(&client->sender, NULL, trk_send, client);
}

void echoClient_stop(ECHO_CLIENT client) {
	client->running = false;
	pthread_join(client->sender, NULL);
}

void echoClient_destroy(ECHO_CLIENT client) {
	client->tracker = NULL;
	client->sender = NULL;
	free(client);
	client = NULL;
}

