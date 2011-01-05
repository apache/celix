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

