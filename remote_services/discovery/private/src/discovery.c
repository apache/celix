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
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"

#include "discovery.h"

struct discovery {
	BUNDLE_CONTEXT context;
	apr_pool_t *pool;

	HASH_MAP listenerReferences;

	bool running;
	apr_thread_t *slpPoll;

	HASH_MAP slpServices;

	char *rsaPort;

	ARRAY_LIST handled;
	ARRAY_LIST registered;
};

struct slp_service {
	char *serviceUrl;
	char *attributes;
};

typedef struct slp_service *slp_service_t;

celix_status_t discovery_informListener(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint);
celix_status_t discovery_informListenerOfRemoval(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint);

celix_status_t discovery_addService(discovery_t discovery, endpoint_description_t endpoint);
celix_status_t discovery_removeService(discovery_t discovery, endpoint_description_t endpoint);

static void *APR_THREAD_FUNC discovery_pollSLP(apr_thread_t *thd, void *data);
SLPBoolean discovery_pollSLPCallback(SLPHandle hslp, const char* srvurl, unsigned short lifetime, SLPError errcode, void* cookie);
SLPBoolean discovery_attributesCallback(SLPHandle hslp, const char *attributes, SLPError error, void *cookie);

celix_status_t discovery_deregisterEndpoint(discovery_t discovery, const char *serviceUrl);
void discovery_deregistrationReport(SLPHandle hslp, SLPError errcode, void* cookie);

celix_status_t discovery_create(apr_pool_t *pool, BUNDLE_CONTEXT context, discovery_t *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	*discovery = apr_palloc(pool, sizeof(**discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	} else {
		(*discovery)->context = context;
		(*discovery)->pool = pool;
		(*discovery)->listenerReferences = hashMap_create(NULL, NULL, NULL, NULL);
		(*discovery)->slpServices = hashMap_create(string_hash, NULL, string_equals, NULL);
		(*discovery)->running = true;
		(*discovery)->rsaPort = getenv("RSA_PORT");
		(*discovery)->handled = arrayList_create();
		(*discovery)->registered = arrayList_create();

		apr_thread_create(&(*discovery)->slpPoll, NULL, discovery_pollSLP, *discovery, (*discovery)->pool);
	}

	return status;
}

celix_status_t discovery_deregisterEndpoint(discovery_t discovery, const char *serviceUrl) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Remove endpoint: %s\n", serviceUrl);

	SLPError err;
	SLPError callbackerr;
	SLPHandle slp;

	err = SLPOpen("en", SLP_FALSE, &slp);
	if (err != SLP_OK) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		err = SLPDereg(slp, serviceUrl, discovery_deregistrationReport, &callbackerr);
		if ((err != SLP_OK) || (callbackerr != SLP_OK)) {
			printf("DISCOVERY: Error deregistering service (%s) with slp %i\n", serviceUrl, err);
			status = CELIX_BUNDLE_EXCEPTION;
		}
		SLPClose(slp);
	}

	return status;
}

celix_status_t discovery_stop(discovery_t discovery) {
	celix_status_t status = CELIX_SUCCESS;

	apr_status_t tstat;
	discovery->running = false;
	apr_status_t stat = apr_thread_join(&tstat, discovery->slpPoll);
	if (stat != APR_SUCCESS && tstat != APR_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	}

	int i;
	for (i = 0; i < arrayList_size(discovery->registered); i++) {
		char *url = arrayList_get(discovery->registered, i);
		discovery_deregisterEndpoint(discovery, url);
	}

	return status;
}

celix_status_t discovery_removeService(discovery_t discovery, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Remove service (%s)\n", endpoint->service);

	// Inform listeners of new endpoint
	HASH_MAP_ITERATOR iter = hashMapIterator_create(discovery->listenerReferences);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		SERVICE_REFERENCE reference = hashMapEntry_getKey(entry);
		endpoint_listener_t listener = NULL;
		bundleContext_getService(discovery->context, reference, (void**)&listener);
		discovery_informListenerOfRemoval(discovery, listener, endpoint);
	}

	return status;
}

celix_status_t discovery_addService(discovery_t discovery, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	// Inform listeners of new endpoint
	HASH_MAP_ITERATOR iter = hashMapIterator_create(discovery->listenerReferences);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		SERVICE_REFERENCE reference = hashMapEntry_getKey(entry);
		endpoint_listener_t listener = NULL;

		char *scope = properties_get(reference->registration->properties, (char *) ENDPOINT_LISTENER_SCOPE);
		FILTER filter = filter_create(scope);
		if (filter_match(filter, endpoint->properties)) {
			printf("DISCOVERY: Add service (%s)\n", endpoint->service);
			bundleContext_getService(discovery->context, reference, (void**)&listener);
			discovery_informListener(discovery, listener, endpoint);
		}
	}

	return status;
}

celix_status_t discovery_informListener(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	listener->endpointAdded(listener->handle, endpoint, NULL);
	return status;
}

celix_status_t discovery_informListenerOfRemoval(discovery_t discovery, endpoint_listener_t listener, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	listener->endpointRemoved(listener->handle, endpoint, NULL);
	return status;
}

celix_status_t discovery_constructServiceUrl(discovery_t discovery, endpoint_description_t endpoint, char **serviceUrl) {
	celix_status_t status = CELIX_SUCCESS;

	if (*serviceUrl != NULL || discovery == NULL || endpoint == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		char host[APRMAXHOSTLEN + 1];
		apr_sockaddr_t *sa;
		char *ip;

		apr_status_t stat = apr_gethostname(host, APRMAXHOSTLEN + 1, discovery->pool);
		if (stat != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			stat = apr_sockaddr_info_get(&sa, host, APR_INET, 0, 0, discovery->pool);
			if (stat != APR_SUCCESS) {
				status = CELIX_BUNDLE_EXCEPTION;
			} else {
				stat = apr_sockaddr_ip_get(&ip, sa);
				if (stat != APR_SUCCESS) {
					status = CELIX_BUNDLE_EXCEPTION;
				} else {
					*serviceUrl = apr_pstrcat(discovery->pool, "service:osgi.remote:http://", ip, ":", discovery->rsaPort, "/services/", endpoint->service, NULL);
				}
			}
		}
	}

	return status;
}

void discovery_registrationReport(SLPHandle hslp, SLPError errcode, void* cookie) {
	*(SLPError*)cookie = errcode;
}

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" added\n", endpoint->service, machtedFilter);
	discovery_t discovery = handle;
	SLPError err;
	SLPError callbackerr;
	SLPHandle slp;
	char *serviceUrl = NULL;

	err = SLPOpen("en", SLP_FALSE, &slp);
	if (err != SLP_OK) {
		status = CELIX_ILLEGAL_STATE;
	} else {
		status = discovery_constructServiceUrl(discovery, endpoint, &serviceUrl);
		if (status == CELIX_SUCCESS) {
			char *attributes = "";
			HASH_MAP_ITERATOR iter = hashMapIterator_create(endpoint->properties);
			while (hashMapIterator_hasNext(iter)) {
				HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
				char *key = hashMapEntry_getKey(entry);
				char *value = hashMapEntry_getValue(entry);
				if (strlen(attributes) != 0) {
					attributes = apr_pstrcat(discovery->pool, attributes, ",", NULL);
				}
				attributes = apr_pstrcat(discovery->pool, attributes, "(", key, "=", value, ")", NULL);
			}
			err = SLPReg(slp, serviceUrl, SLP_LIFETIME_MAXIMUM, 0, attributes, SLP_TRUE, discovery_registrationReport, &callbackerr);
			if ((err != SLP_OK) || (callbackerr != SLP_OK)) {
				status = CELIX_ILLEGAL_STATE;
				printf("DISCOVERY: Error registering service (%s) with slp %i\n", serviceUrl, err);
			}
			arrayList_add(discovery->registered, serviceUrl);
		}
		SLPClose(slp);
	}

	return status;
}

void discovery_deregistrationReport(SLPHandle hslp, SLPError errcode, void* cookie) {
	*(SLPError*)cookie = errcode;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" removed\n", endpoint->service, machtedFilter);

	discovery_t discovery = handle;
	SLPError err;
	SLPError callbackerr;
	SLPHandle slp;
	char *serviceUrl = NULL;

	err = SLPOpen("en", SLP_FALSE, &slp);
	if (err != SLP_OK) {
		status = CELIX_ILLEGAL_STATE;
	} else {
		status = discovery_constructServiceUrl(discovery, endpoint, &serviceUrl);
		if (status == CELIX_SUCCESS) {
			status = discovery_deregisterEndpoint(discovery, serviceUrl);
		}
	}

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

	HASH_MAP_ITERATOR iter = hashMapIterator_create(discovery->slpServices);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		char *key = hashMapEntry_getKey(entry);
		endpoint_description_t value = hashMapEntry_getValue(entry);
		discovery_informListener(discovery, service, value);
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
	SLPHandle slp;
	SLPError err;

	err = SLPOpen("en", SLP_FALSE, &slp);

	while (discovery->running) {
		SLPError err = SLP_TRUE;
		SLPError callbackerr;
		arrayList_clear(discovery->handled);
		while (err == SLP_TRUE) {
			err = SLPFindSrvs(slp, "osgi.remote", 0, 0, discovery_pollSLPCallback, data);
		}

		HASH_MAP_ITERATOR iter = hashMapIterator_create(discovery->slpServices);
		while (hashMapIterator_hasNext(iter)) {
			HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
			char *key = hashMapEntry_getKey(entry);
			endpoint_description_t value = hashMapEntry_getValue(entry);

			bool inUse = false;
			int i;
			for (i = 0; i < arrayList_size(discovery->handled); i++) {
				char *url = arrayList_get(discovery->handled, i);
				if (strcmp(url, key) == 0) {
					inUse = true;
					break;
				}
			}

			if (!inUse) {
				discovery_removeService(discovery, value);

				hashMapIterator_remove(iter);
			}
		}

		sleep(1);
	}
	SLPClose(slp);
	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;
}

SLPBoolean discovery_pollSLPCallback(SLPHandle hslp, const char* srvurl, unsigned short lifetime, SLPError errcode, void *cookie) {
	discovery_t discovery = cookie;
	if (errcode == SLP_OK) {

		arrayList_add(discovery->handled, (void *) srvurl);
		if (!hashMap_containsKey(discovery->slpServices, (void *) srvurl)) {
			// service:osgi.remote:http://10.0.0.21:8080/services/example
			if (strncmp(srvurl, "service:osgi.remote:", 20) == 0) {
				const char *url = srvurl+20;
				const char *srv = strrchr(url, '/')+1;

				SLPHandle handle = NULL;
				SLPError err = SLPOpen("en", SLP_FALSE, &handle);
				err = SLP_TRUE;
				slp_service_t slpService = apr_palloc(discovery->pool, sizeof(*slpService));
				while (err == SLP_TRUE) {
					err = SLPFindAttrs(handle, srvurl, "", "", discovery_attributesCallback, slpService);
				}

				PROPERTIES props = properties_create();
				char *track;
				char *token = apr_strtok(slpService->attributes, ",", &track);
				while (token != NULL) {
					char *track2;
					char *token2 = apr_strtok(token, "=", &track2);
					char *token3 = apr_strtok(NULL, "=", &track2);
					char *key = apr_pstrdup(discovery->pool, token2+1);
					char *value = apr_pstrndup(discovery->pool, token3, strlen(token3) - 1);
					properties_set(props, key, value);
					token = apr_strtok(NULL, ",", &track);
				}

				endpoint_description_t endpoint = apr_palloc(discovery->pool, sizeof(*endpoint));
				endpoint->id = apr_pstrdup(discovery->pool, url);
				endpoint->serviceId = 42;
				endpoint->service = apr_pstrdup(discovery->pool, srv);
				endpoint->properties = props;
				discovery_addService(discovery, endpoint);

				hashMap_put(discovery->slpServices, apr_pstrdup(discovery->pool, srvurl), endpoint);
			}
		}
	} else if (errcode == SLP_LAST_CALL) {
		return SLP_FALSE;
	} else {
	}

	return SLP_TRUE;
}

SLPBoolean discovery_attributesCallback(SLPHandle hslp, const char *attributes, SLPError error, void *cookie) {
	slp_service_t slpService = cookie;
	if (error == SLP_OK) {
		slpService->attributes = strdup(attributes);
	} else if (error == SLP_LAST_CALL) {
		return SLP_FALSE;
	}

	return SLP_TRUE;
}
