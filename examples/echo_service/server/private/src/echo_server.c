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
 * echo_server.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "echo_server_private.h"

struct echoServer {
	char * name;
};

echo_server_pt echoServer_create() {
	echo_server_pt server = malloc(sizeof(*server));
	server->name = "MacBook Pro";
	return server;
}

void echoServer_echo(echo_server_pt server, char * text) {
	printf("Server %s says %s\n", server->name, text);
}

void echoServer_destroy(echo_server_pt server) {
	server->name = NULL;
	free(server);
	server = NULL;
}
