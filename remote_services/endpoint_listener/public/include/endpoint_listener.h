/*
 * endpoint_listener.h
 *
 *  Created on: Sep 29, 2011
 *      Author: alexander
 */

#ifndef ENDPOINT_LISTENER_H_
#define ENDPOINT_LISTENER_H_

#include "array_list.h"
#include "properties.h"

struct endpoint_description {
	ARRAY_LIST configurationTypes;
	char *frameworkUUID;
	char *id;
	// ARRAY_LIST intents;
	char *service;
	// HASH_MAP packageVersions;
	PROPERTIES properties;
	long serviceId;
};

typedef struct endpoint_description *endpoint_description_t;

static const char * const endpoint_listener_service = "endpoint_listener";

static const char * const ENDPOINT_LISTENER_SCOPE = "endpoint.listener.scope";

struct endpoint_listener {
	void *handle;
	celix_status_t (*endpointAdded)(void *handle, endpoint_description_t endpoint, char *machtedFilter);
	celix_status_t (*endpointRemoved)(void *handle, endpoint_description_t endpoint, char *machtedFilter);
};

typedef struct endpoint_listener *endpoint_listener_t;


#endif /* ENDPOINT_LISTENER_H_ */
