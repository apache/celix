/*
 * discovery.c
 *
 *  Created on: Oct 4, 2011
 *      Author: alexander
 */
#include <stdio.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <slp.h>
#include <unistd.h>

#include "headers.h"
#include "bundle_context.h"
#include "array_list.h"

#include "discovery.h"

struct discovery {
	BUNDLE_CONTEXT context;
	apr_pool_t *pool;

	HASH_MAP listenerReferences;
	ARRAY_LIST endpoints;

	bool running;
	apr_thread_t *slpPoll;
};

celix_status_t discovery_informListener(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint);

celix_status_t discovery_addService(discovery_t discovery, endpoint_description_t endpoint);

static void *APR_THREAD_FUNC discovery_pollSLP(apr_thread_t *thd, void *data);
SLPBoolean discovery_pollSLPCallback(SLPHandle hslp, const char* srvurl, unsigned short lifetime, SLPError errcode, void* cookie);

celix_status_t discovery_create(apr_pool_t *pool, BUNDLE_CONTEXT context, discovery_t *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	*discovery = apr_palloc(pool, sizeof(**discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	} else {
		(*discovery)->context = context;
		(*discovery)->pool = pool;
		(*discovery)->listenerReferences = hashMap_create(NULL, NULL, NULL, NULL);
		(*discovery)->endpoints = arrayList_create();
		(*discovery)->running = true;

		endpoint_description_t endpoint = apr_palloc(pool, sizeof(*endpoint));
		endpoint->id = apr_pstrdup(pool, "http://localhost:8080/services/example/");
		endpoint->serviceId = 42;
		endpoint->service = "example";
		discovery_addService(*discovery, endpoint);

		apr_thread_create(&(*discovery)->slpPoll, NULL, discovery_pollSLP, *discovery, (*discovery)->pool);
	}

	return status;
}

celix_status_t discovery_addService(discovery_t discovery, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Service added\n");

	arrayList_add(discovery->endpoints, endpoint);

	// Inform listeners of new endpoint
	HASH_MAP_ITERATOR iter = hashMapIterator_create(discovery->listenerReferences);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		SERVICE_REFERENCE reference = hashMapEntry_getKey(entry);
		endpoint_listener_t listener = NULL;
		bundleContext_getService(discovery->context, reference, (void**)&listener);
		discovery_informListener(discovery, listener, endpoint);
	}

	return status;
}

celix_status_t discovery_informListener(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	listener->endpointAdded(listener->handle, endpoint, NULL);
	return status;
}

void MySLPRegReport(SLPHandle hslp, SLPError errcode, void* cookie)
  {
      /* return the error code in the cookie */

      *(SLPError*)cookie = errcode;

      printf("Error\n");

      /* You could do something else here like print out    */
      /* the errcode, etc.  Remember, as a general rule,    */
      /* do not try to do too much in a callback because    */
      /* it is being executed by the same thread that is    */
      /* reading slp packets from the wire.                 */
  }

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint added\n");
	discovery_t discovery = handle;
	SLPError err;
	SLPError callbackerr;
	SLPHandle slp;

	//publish endpoint in slp

	SLPOpen("en", SLP_FALSE, &slp);

	err = SLPReg(slp, "service:osgi.remote://host:8081",
			SLP_LIFETIME_MAXIMUM, 0, "", SLP_TRUE, MySLPRegReport, &callbackerr);

	if ((err != SLP_OK) || (callbackerr != SLP_OK)) {
		printf("Error registering service with slp %i\n", err);
		return err;
	}

	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint removed\n");

	//unpublish endpoint

	return status;
}

celix_status_t discovery_endpointListenerAdding(void * handle, SERVICE_REFERENCE reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t discovery = handle;

	bundleContext_getService(discovery->context, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerAdded(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t discovery = handle;
	char *discoveryListener = properties_get(reference->registration->properties, "DISCOVERY");

	if (discoveryListener != NULL && strcmp(discoveryListener, "true") == 0) {
		printf("DISCOVERY: EndpointListener Ignored - Discovery listener\n");
	} else {
		printf("DISCOVERY: EndpointListener Added - Add Scope\n");
		discovery_updateEndpointListener(discovery, reference, (endpoint_listener_t) service);
	}

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t discovery = handle;

	printf("DISCOVERY: EndpointListener Modified - Update Scope\n");
	discovery_updateEndpointListener(discovery, reference, (endpoint_listener_t) service);

	return status;
}

celix_status_t discovery_updateEndpointListener(discovery_t discovery, SERVICE_REFERENCE reference, endpoint_listener_t service) {
	celix_status_t status = CELIX_SUCCESS;
	char *scope = "createScopeHere";

	ARRAY_LIST scopes = hashMap_get(discovery->listenerReferences, reference);
	if (scopes == NULL) {
		scopes = arrayList_create();
		hashMap_put(discovery->listenerReferences, reference, scopes);
	}

	if (!arrayList_contains(scopes, scope)) {
		arrayList_add(scopes, scope);
	}

	int size = arrayList_size(discovery->endpoints);
	int iter = 0;
	for (iter = 0; iter < size; iter++) {
		endpoint_description_t endpoint = arrayList_get(discovery->endpoints, iter);
		discovery_informListener(discovery, service, endpoint);
	}

	return status;
}

celix_status_t discovery_endpointListenerRemoved(void * handle, SERVICE_REFERENCE reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_t discovery = handle;

	printf("DISCOVERY: EndpointListener Removed\n");
	hashMap_remove(discovery->listenerReferences, reference);

	return status;
}

static void *APR_THREAD_FUNC discovery_pollSLP(apr_thread_t *thd, void *data) {
	discovery_t discovery = data;
	SLPError err;
	SLPError callbackerr;
	SLPHandle slp;

	SLPOpen("en", SLP_FALSE, &slp);

	while (discovery->running) {
		err = SLPFindSrvs(slp, "osgi.remote", 0, 0, discovery_pollSLPCallback, &callbackerr);
		sleep(5);
	}

	return NULL;
}

SLPBoolean discovery_pollSLPCallback(SLPHandle hslp, const char* srvurl, unsigned short lifetime, SLPError errcode, void* cookie) {
	if (errcode == SLP_OK || errcode == SLP_LAST_CALL) {
		printf("Service URL     = %s\n", srvurl);
		printf("Service Timeout = %i\n", lifetime);
		*(SLPError*) cookie = SLP_OK;
	} else {
		*(SLPError*) cookie = errcode;
	}

	return SLP_TRUE;
}
