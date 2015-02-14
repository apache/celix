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
 * echo_client.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "service_tracker.h"

#include "echo_client_private.h"
#include "echo_server.h"

static void *trk_send(void *handle) {

	echo_client_pt client = (echo_client_pt) handle;

	while (client->running) {
		echo_service_pt service = (echo_service_pt) serviceTracker_getService(client->tracker);
		if (service != NULL) {
			service->echo(service->server, client->ident);
		}
		sleep(1);
	}

	pthread_exit(NULL);

	return NULL;
}

echo_client_pt echoClient_create(service_tracker_pt echoServiceTracker) {
	echo_client_pt client = malloc(sizeof(*client));

	client->tracker = echoServiceTracker;
	client->running = false;
	client->ident = "OSX rules";

	return client;
}

void echoClient_start(echo_client_pt client) {
	client->running = true;
	pthread_create(&client->sender_thread, NULL, trk_send, client);
}

void echoClient_stop(echo_client_pt client) {
	client->running = false;
	pthread_join(client->sender_thread, NULL);
}

void echoClient_destroy(echo_client_pt client) {
	client->tracker = NULL;
	client->sender_thread = 0;
	free(client);
	client = NULL;
}

