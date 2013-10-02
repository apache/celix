/*
 * server.h
 *
 *  Created on: Feb 2, 2012
 *      Author: alexander
 */

#ifndef SERVER_H_
#define SERVER_H_

#define SERVER_NAME "server"

typedef struct server *server_t;

typedef struct server_service *server_service_t;

struct server_service {
	server_t server;
	celix_status_t (*server_doo)(server_t server, int a, int b, int *reply);
};

#endif /* SERVER_H_ */
