/*
 * remote_proxy.h
 *
 *  Created on: Oct 13, 2011
 *      Author: alexander
 */

#ifndef REMOTE_PROXY_H_
#define REMOTE_PROXY_H_

#include "endpoint_listener.h"

#define REMOTE_PROXY "remote_proxy"

struct remote_proxy_service {
	void *proxy;
	celix_status_t (*setEndpointDescription)(void *proxy, endpoint_description_t service);
};

typedef struct remote_proxy_service *remote_proxy_service_t;

#endif /* REMOTE_PROXY_H_ */
