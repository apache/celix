/*
 * echo_client_private.h
 *
 *  Created on: Sep 21, 2010
 *      Author: alexanderb
 */

#ifndef ECHO_CLIENT_PRIVATE_H_
#define ECHO_CLIENT_PRIVATE_H_

#include <stdbool.h>
#include <pthread.h>

#include "headers.h"

struct echoClient {
	SERVICE_TRACKER tracker;
	bool running;

	pthread_t sender;
};

typedef struct echoClient * ECHO_CLIENT;

ECHO_CLIENT echoClient_create(SERVICE_TRACKER context);

void echoClient_start(ECHO_CLIENT client);
void echoClient_stop(ECHO_CLIENT client);

void echoClient_destroy(ECHO_CLIENT client);


#endif /* ECHO_CLIENT_PRIVATE_H_ */
