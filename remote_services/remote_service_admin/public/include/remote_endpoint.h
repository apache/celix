/*
 * endpoint.h
 *
 *  Created on: Oct 7, 2011
 *      Author: alexander
 */

#ifndef REMOTE_ENDPOINT_H_
#define REMOTE_ENDPOINT_H_

#define REMOTE_ENDPOINT "remote_endpoint"

typedef struct remote_endpoint *remote_endpoint_t;

struct remote_endpoint_service {
	remote_endpoint_t endpoint;
	celix_status_t (*setService)(remote_endpoint_t endpoint, void *service);
	celix_status_t (*handleRequest)(remote_endpoint_t endpoint, char *request, char *data, char **reply);
};

typedef struct remote_endpoint_service *remote_endpoint_service_t;


#endif /* REMOTE_ENDPOINT_H_ */
