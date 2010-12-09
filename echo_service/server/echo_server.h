/*
 * echo_server.h
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */

#ifndef ECHO_SERVER_H_
#define ECHO_SERVER_H_

#define ECHO_SERVICE_NAME "echo_server"

typedef struct echoServer * ECHO_SERVER;

struct echoService {
	ECHO_SERVER server;
	void (*echo)(ECHO_SERVER server, char * text);
};

typedef struct echoService * ECHO_SERVICE;

#endif /* ECHO_SERVER_H_ */
