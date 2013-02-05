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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "service_tracker.h"

#include "echo_client_private.h"
#include "echo_server.h"

static void *APR_THREAD_FUNC trk_send(apr_thread_t *thd, void *handle) {
	ECHO_CLIENT client = (ECHO_CLIENT) handle;
	while (client->running) {
		ECHO_SERVICE service = (ECHO_SERVICE) serviceTracker_getService(client->tracker);
		if (service != NULL) {
			service->echo(service->server, "hi");
		}
		apr_sleep(1000000);
	}
	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

ECHO_CLIENT echoClient_create(service_tracker_t echoServiceTracker, apr_pool_t *pool) {
	ECHO_CLIENT client = malloc(sizeof(*client));

	client->tracker = echoServiceTracker;
	client->running = false;
	client->pool = pool;

	return client;
}

void echoClient_start(ECHO_CLIENT client) {
	client->running = true;
	apr_thread_create(&client->sender, NULL, trk_send, client, client->pool);
}

void echoClient_stop(ECHO_CLIENT client) {
	apr_status_t status;
	client->running = false;
	apr_thread_join(&status, client->sender);
}

void echoClient_destroy(ECHO_CLIENT client) {
	client->tracker = NULL;
	client->sender = 0;
	free(client);
	client = NULL;
}

