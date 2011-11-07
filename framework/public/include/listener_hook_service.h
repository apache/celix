/*
 * listener_hook_service.h
 *
 *  Created on: Oct 28, 2011
 *      Author: alexander
 */

#ifndef LISTENER_HOOK_SERVICE_H_
#define LISTENER_HOOK_SERVICE_H_

#include "headers.h"

#define listener_hook_service_name "listener_hook_service"

typedef struct listener_hook *listener_hook_t;

struct listener_hook_info {
	BUNDLE_CONTEXT context;
	char *filter;
	bool removed;
};

typedef struct listener_hook_info *listener_hook_info_t;

struct listener_hook_service {
	void *handle;
	celix_status_t (*added)(void *hook, ARRAY_LIST listeners);
	celix_status_t (*removed)(void *hook, ARRAY_LIST listeners);
};

typedef struct listener_hook_service *listener_hook_service_t;

#endif /* LISTENER_HOOK_SERVICE_H_ */
