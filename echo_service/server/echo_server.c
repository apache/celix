/*
 * echo_server.c
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>

#include "echo_server_private.h"

struct echoServer {
	char * name;
};

ECHO_SERVER echoServer_create() {
	ECHO_SERVER server = malloc(sizeof(*server));
	server->name = "MacBook Pro";
	return server;
}

void echoServer_echo(ECHO_SERVER server, char * text) {
	printf("Server %s says %s\n", server->name, text);
}

void echoServer_destroy(ECHO_SERVER server) {
	server->name = NULL;
	free(server);
	server = NULL;
}
