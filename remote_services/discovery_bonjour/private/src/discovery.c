/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * discovery.c
 *
 *  \date       Oct 4, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <unistd.h>
#include <stdbool.h>

#include <dns_sd.h>

#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"

#include "discovery.h"

static void *APR_THREAD_FUNC discovery_poll(apr_thread_t *thd, void *data);
static void discovery_browseCallback(
		    DNSServiceRef sdRef,
		    DNSServiceFlags flags,
		    uint32_t interfaceIndex,
		    DNSServiceErrorType errorCode,
		    const char                          *serviceName,
		    const char                          *regtype,
		    const char                          *replyDomain,
		    void                                *context
		);
static void discovery_resolveCallback
(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char                          *fullname,
    const char                          *hosttarget,
    uint16_t port,                                   /* In network byte order */
    uint16_t txtLen,
    const unsigned char                 *txtRecord,
    void                                *context
);
static celix_status_t discovery_deregisterEndpoint(discovery_pt discovery, const char *serviceUrl);
static celix_status_t discovery_addService(discovery_pt discovery, endpoint_description_pt endpoint);
static celix_status_t discovery_removeService(discovery_pt discovery, endpoint_description_pt endpoint);

static const char * const DEFAULT_RSA_PORT = "555";
static const char * const OSGI_SERVICE_TYPE = "_osgi._udp";

struct discovery {
	bundle_context_pt context;
	apr_pool_t *pool;

	hash_map_pt listenerReferences;

	bool running;
	apr_thread_t *poll;
	DNSServiceRef browseRef;

	hash_map_pt slpServices;

	char *rsaPort;

	array_list_pt handled;
	array_list_pt registered;
};

celix_status_t discovery_create(apr_pool_t *pool, bundle_context_pt context, discovery_pt *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	*discovery = apr_palloc(pool, sizeof(**discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	} else {
		(*discovery)->context = context;
		(*discovery)->pool = pool;
		(*discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		(*discovery)->slpServices = hashMap_create(string_hash, NULL, string_equals, NULL);
		(*discovery)->running = true;
		(*discovery)->browseRef = NULL;
		(*discovery)->rsaPort = getenv("RSA_PORT");
		if ((*discovery)->rsaPort == NULL) {
			printf("No RemoteServiceAdmin port set, set it using RSA_PORT! Using default port (%s)\n", DEFAULT_RSA_PORT);
			(*discovery)->rsaPort = DEFAULT_RSA_PORT;
		}
		(*discovery)->handled = NULL;
		arrayList_create(pool, &(*discovery)->handled);
		(*discovery)->registered = NULL;
		arrayList_create(pool, &(*discovery)->registered);

		DNSServiceErrorType error = DNSServiceBrowse(
				 &(*discovery)->browseRef,
				   0,
				   0,
				   OSGI_SERVICE_TYPE,
				   NULL, /* may be NULL */
				   discovery_browseCallback,
				   (*discovery)/* may be NULL */
				   );

		apr_thread_create(&(*discovery)->poll, NULL, discovery_poll, *discovery, (*discovery)->pool);
	}

	return status;
}

celix_status_t discovery_stop(discovery_pt discovery) {
	celix_status_t status = CELIX_SUCCESS;

	apr_status_t tstat;
	discovery->running = false;
	DNSServiceRefDeallocate(discovery->browseRef);
	apr_status_t stat = apr_thread_join(&tstat, discovery->poll);
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

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("discovery_endpointAdded CALLED\n");

	DNSServiceRef sdRef = NULL;
	DNSServiceErrorType error;
	TXTRecordRef txtRecord;

    TXTRecordCreate (&txtRecord, 256, NULL);//TODO search for correct default record size
    char serviceId[16];
    sprintf(serviceId, "%li", endpoint->serviceId);

    TXTRecordSetValue(&txtRecord, "service", strlen(endpoint->service), endpoint->service);
    TXTRecordSetValue(&txtRecord, "service.id", strlen(serviceId), serviceId);

    hash_map_iterator_pt iter = hashMapIterator_create(endpoint->properties);
    while (hashMapIterator_hasNext(iter))  {
    	hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
    	char *key = hashMapEntry_getKey(entry);
    	char *value = hashMapEntry_getValue(entry);
    	TXTRecordSetValue(&txtRecord, key, strlen(value), value);
    }

    int port = atoi(discovery->rsaPort);
    int portInNetworkByteOrder = ((port << 8) & 0xFF00) | ((port >> 8) & 0xFF); //TODO assuming little endian ?? correct? check with ifdef?

    error = DNSServiceRegister (
       &sdRef,
       0,
       0,
       endpoint->service, /* may be NULL */
       OSGI_SERVICE_TYPE,
       NULL, /* may be NULL */
       NULL, /* may be NULL */
       portInNetworkByteOrder, /* In network byte order */
       TXTRecordGetLength(&txtRecord),
       TXTRecordGetBytesPtr(&txtRecord), /* may be NULL */
       NULL, /* may be NULL */
       NULL /* may be NULL */
       );


    printf("Registered record in dns-sd got error code %i\n", error);

	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t discovery_endpointListenerAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	bundleContext_getService(discovery->context, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerAdded(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	properties_pt serviceProperties = NULL;
	serviceRegistration_getProperties(registration, &serviceProperties);
	char *discoveryListener = properties_get(serviceProperties, "DISCOVERY");

	if (discoveryListener != NULL && strcmp(discoveryListener, "true") == 0) {
		printf("DISCOVERY: EndpointListener Ignored - Discovery listener\n");
	} else {
		printf("DISCOVERY: EndpointListener Added - Add Scope\n");
		discovery_updateEndpointListener(discovery, reference, (endpoint_listener_pt) service);
	}

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("DISCOVERY: EndpointListener Modified - Update Scope TODO\n");

	return status;
}



celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("DISCOVERY: EndpointListener Removed\n");
	hashMap_remove(discovery->listenerReferences, reference);

	return status;
}

celix_status_t discovery_updateEndpointListener(discovery_pt discovery,
		service_reference_pt reference, endpoint_listener_pt service) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

static void *APR_THREAD_FUNC discovery_poll(apr_thread_t *thd, void *data) {
	discovery_pt discovery = data;

	while (discovery->running) {
		DNSServiceProcessResult(discovery->browseRef);
	}
	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;
}

static void discovery_browseCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
		uint32_t interfaceIndex, DNSServiceErrorType errorCode,
		const char *serviceName, const char *regtype, const char *replyDomain,
		void *context) {
	discovery_pt discovery = context;
	if (flags & kDNSServiceFlagsAdd) {
		printf("Added service with %s %s %s\n", serviceName, regtype,
				replyDomain);
		DNSServiceRef resolveRef = NULL;
		DNSServiceErrorType resolveError = DNSServiceResolve(&resolveRef, 0, 0,
				serviceName, regtype, replyDomain, discovery_resolveCallback,
				context);
		printf("Resolve return with error %i\n", resolveError);
		DNSServiceProcessResult(resolveRef);
	} else {
		printf("Removed service with %s %s %s\n", serviceName, regtype,
				replyDomain);
	}
}

static void discovery_resolveCallback(DNSServiceRef sdRef,
		DNSServiceFlags flags, uint32_t interfaceIndex,
		DNSServiceErrorType errorCode, const char *fullname,
		const char *hosttarget, uint16_t port, /* In network byte order */
		uint16_t txtLen, const unsigned char *txtRecord, void *context) {
	printf("In resolve callback!\n");
	int length = TXTRecordGetCount(txtLen, txtRecord);
	printf("Found txt record with item count %i\n|", length);
	for (int i=0; i<length; i+=1) {
		char key[128];
		void *value = NULL;
		int valueSize = 0;
		TXTRecordGetItemAtIndex(txtLen, txtRecord, i, 128, key, &valueSize, &value);
		printf("Found key=value %s=%s\n", key, value);
	}
}

static celix_status_t discovery_deregisterEndpoint(discovery_pt discovery, const char *serviceUrl) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Remove endpoint: %s\n", serviceUrl);
	printf("TODO\n");

	return status;
}

static celix_status_t discovery_addService(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	//TODO should be called when dns-sd find a services, forward to listeners
	return status;
}

static celix_status_t discovery_removeService(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	//TODO should be called when dns-sd notices a removal of a services, forward to listeners
	return status;
}
