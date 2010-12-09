/*
 * echo_server_private.h
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */

#ifndef ECHO_SERVER_PRIVATE_H_
#define ECHO_SERVER_PRIVATE_H_

#include "echo_server.h"

ECHO_SERVER echoServer_create();
void echoServer_echo(ECHO_SERVER server, char * text);
void echoServer_destroy(ECHO_SERVER server);

#endif /* ECHO_SERVER_PRIVATE_H_ */
