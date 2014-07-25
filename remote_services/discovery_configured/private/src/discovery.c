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
 *  \date       Sep 1, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <apr_thread_proc.h>
#include <apr_strings.h>
#include <netdb.h>
#include <netinet/in.h>

#include <dns_sd.h>

#include "constants.h"
#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"
#include "remote_constants.h"

#include "discovery.h"
#include "endpoint_discovery_poller.h"

static const char * const OSGI_DISCOVERY_TYPE = "_osgid._udp";
static const char * const OSGI_DISCOVERY_NAME = "Amdatu Remote Service Endpoint (Bonjour)";
static void *APR_THREAD_FUNC discovery_pollDiscovery(apr_thread_t *thd, void *data);

static celix_status_t discovery_start(discovery_pt discovery);
static void discovery_browseDiscoveryCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
        uint32_t interfaceIndex, DNSServiceErrorType errorCode,
        const char *serviceName, const char *regtype, const char *replyDomain,
        void *context);


static void discovery_browseCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
		uint32_t interfaceIndex, DNSServiceErrorType errorCode,
		const char *serviceName, const char *regtype, const char *replyDomain,
		void *context);
static void discovery_resolveAddCallback(DNSServiceRef sdRef,
		DNSServiceFlags flags, uint32_t interfaceIndex,
		DNSServiceErrorType errorCode, const char *fullname,
		const char *hosttarget, uint16_t port, /* In network byte order */
		uint16_t txtLen, const unsigned char *txtRecord, void *context);
static void discovery_resolveRemoveCallback(DNSServiceRef sdRef,
		DNSServiceFlags flags, uint32_t interfaceIndex,
		DNSServiceErrorType errorCode, const char *fullname,
		const char *hosttarget, uint16_t port, /* In network byte order */
		uint16_t txtLen, const unsigned char *txtRecord, void *context);
static celix_status_t discovery_informEndpointListeners(discovery_pt discovery, endpoint_description_pt endpoint, bool addingService);

static const char * const DEFAULT_DISCOVERY_PORT = "8889";
static const char * const OSGI_SERVICE_TYPE = "_osgi._udp";

typedef struct discovered_endpoint_entry {
	apr_pool_t *pool;
	endpoint_description_pt endpointDescription;
} * discovered_endpoint_entry_pt;

typedef struct disclosed_endpoint_entry {
	apr_pool_t *pool;
	endpoint_description_pt endpointDescription;
	TXTRecordRef *txtRecord;
	DNSServiceRef dnsServiceRef;
} * disclosed_endpoint_entry_pt;


struct discovery {
	bundle_context_pt context;
	apr_pool_t *pool;


	apr_thread_mutex_t *listenerReferencesMutex;
	apr_thread_mutex_t *discoveredServicesMutex;
	apr_thread_mutex_t *disclosedServicesMutex;

	hash_map_pt listenerReferences; //key=serviceReference, value=?? TODO
	hash_map_pt discoveredServices; //key=endpointId (string), value=discovered_endpoint_entry_pt;
	hash_map_pt disclosedServices; //key=endpointId (string), value=disclosed_endpoint_entry_pt;

	volatile bool running;
	apr_thread_t *poll;
	apr_thread_t *pollDiscovery;
	DNSServiceRef browseRef;
	DNSServiceRef browseDiscoveryRef;
	DNSServiceRef discoveryRef;

	char *discoveryPort;
	char *frameworkUuid;
	endpoint_discovery_poller_pt poller;
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
		(*discovery)->discoveredServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*discovery)->disclosedServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*discovery)->running = true;
		(*discovery)->browseRef = NULL;
		(*discovery)->discoveryPort = NULL;
		(*discovery)->listenerReferencesMutex = NULL;
		(*discovery)->discoveredServicesMutex = NULL;
		(*discovery)->disclosedServicesMutex = NULL;
		(*discovery)->frameworkUuid = NULL;
		(*discovery)->poller = NULL;

		bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &(*discovery)->frameworkUuid);

		CELIX_DO_IF(status, status = apr_thread_mutex_create(&(*discovery)->listenerReferencesMutex, APR_THREAD_MUTEX_DEFAULT, pool));
		CELIX_DO_IF(status, status = apr_thread_mutex_create(&(*discovery)->discoveredServicesMutex, APR_THREAD_MUTEX_DEFAULT, pool));
		CELIX_DO_IF(status, status = apr_thread_mutex_create(&(*discovery)->disclosedServicesMutex, APR_THREAD_MUTEX_DEFAULT, pool));

		char *port = NULL;
		bundleContext_getProperty(context, "DISCOVERY_PORT", &port);
		if (port == NULL) {
			(*discovery)->discoveryPort = (char *) DEFAULT_DISCOVERY_PORT;
		} else {
			(*discovery)->discoveryPort = apr_pstrdup(pool, port);
		}

		discovery_start(*discovery);
	}

	return status;
}

static celix_status_t discovery_start(discovery_pt discovery) {
    celix_status_t status = CELIX_SUCCESS;

    CELIX_DO_IF(status, status = endpointDiscoveryPoller_create(discovery, &discovery->poller));

    char *path = NULL;
    bundleContext_getProperty(discovery->context, "DISCOVERY_PATH", &path);
    if (path == NULL) {
        path = "path";
    }

    TXTRecordRef txtRecord;

    TXTRecordCreate(&txtRecord, 512, NULL);
    TXTRecordSetValue(&txtRecord, "path", strlen(path), path);

    int port = atoi(discovery->discoveryPort);
    int portInNetworkByteOrder = ((port << 8) & 0xFF00) | ((port >> 8) & 0xFF); //FIXME assuming little endian

    DNSServiceErrorType error = DNSServiceRegister(&discovery->discoveryRef, 0, 0, OSGI_DISCOVERY_NAME, OSGI_DISCOVERY_TYPE, NULL, NULL, portInNetworkByteOrder,
            TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    if (error != kDNSServiceErr_NoError) {
        status = CELIX_ILLEGAL_STATE;
        printf("============= 11 ERROR %d\n", error);
    }

    error = DNSServiceBrowse(&discovery->browseDiscoveryRef, 0, 0, OSGI_DISCOVERY_TYPE, NULL, discovery_browseDiscoveryCallback, discovery);
    if (error != kDNSServiceErr_NoError) {
        status = CELIX_ILLEGAL_STATE;
        printf("============= 22 ERROR %d\n", error);
    }
    status = CELIX_DO_IF(status, apr_thread_create(&discovery->pollDiscovery, NULL, discovery_pollDiscovery, discovery, discovery->pool));

    return status;
}

static void *APR_THREAD_FUNC discovery_pollDiscovery(apr_thread_t *thd, void *data) {
    discovery_pt discovery = data;

    while (discovery->running) {
        DNSServiceProcessResult(discovery->browseDiscoveryRef);
    }
    apr_thread_exit(thd, APR_SUCCESS);

    return NULL;
}

static void discovery_browseDiscoveryCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
        uint32_t interfaceIndex, DNSServiceErrorType errorCode,
        const char *serviceName, const char *regtype, const char *replyDomain,
        void *context) {
    discovery_pt discovery = context;
    if (flags & kDNSServiceFlagsAdd) {
        printf("Added discovery with %s %s %s\n", serviceName, regtype, replyDomain);
        DNSServiceRef resolveRef = NULL;
        DNSServiceErrorType resolveError = DNSServiceResolve(&resolveRef, 0, 0, serviceName, regtype, replyDomain, discovery_resolveAddCallback, context);
        printf("Resolve return with error %i\n", resolveError);
        if (resolveError == kDNSServiceErr_NoError) {
            DNSServiceProcessResult(resolveRef);
        } else {
            //TODO print error / handle error?
        }
    } else {
        printf("Removed discovery with %s %s %s\n", serviceName, regtype,
                replyDomain);
//        DNSServiceRef resolveRef = NULL;
//        DNSServiceErrorType resolveError = DNSServiceResolve(&resolveRef, 0, 0,
//                serviceName, regtype, replyDomain, discovery_resolveRemoveCallback,
//                context);
//        if (resolveError == kDNSServiceErr_NoError) {
//            DNSServiceProcessResult(resolveRef);
//        } else {
//            //TODO print error / handle error?
//        }
    }
}

static void discovery_resolveAddCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname,
        const char *hosttarget, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context) {
    discovery_pt discovery = context;

    printf("Added discovery with %s %s %s\n", fullname, hosttarget, txtRecord);

    uint8_t valueLen;
    char *path = (char *) TXTRecordGetValuePtr(txtLen, txtRecord, "path", &valueLen);
    char *host = strdup(gethostbyname(hosttarget)->h_name);
    uint16_t hPort = ntohs(port);

    printf("Path: %s, Host: %s\n", path, host);

    char url[1024];
    snprintf(url, sizeof(url), "%s:%d/%s", host, hPort, path);

    printf("Discovery URL: %s\n", url);
}

celix_status_t discovery_stop(discovery_pt discovery) {
	celix_status_t status;

	apr_status_t tstat;
	discovery->running = false;
	DNSServiceRefDeallocate(discovery->browseRef);
	apr_status_t stat = apr_thread_join(&tstat, discovery->poll);
	if (stat != APR_SUCCESS && tstat != APR_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	}

	apr_thread_mutex_lock(discovery->disclosedServicesMutex);
	hash_map_iterator_pt iter = hashMapIterator_create(discovery->disclosedServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		disclosed_endpoint_entry_pt endpointEntry = hashMapEntry_getValue(entry);
		DNSServiceRefDeallocate(endpointEntry->dnsServiceRef);
	}
	hashMapIterator_destroy(iter);

	iter = hashMapIterator_create(discovery->discoveredServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		discovered_endpoint_entry_pt endpointEntry = hashMapEntry_getValue(entry);
		discovery_informEndpointListeners(discovery, endpointEntry->endpointDescription, false);
	}
	hashMapIterator_destroy(iter);

	hashMap_destroy(discovery->disclosedServices, false, false);

	discovery->disclosedServices = NULL;
	apr_thread_mutex_unlock(discovery->disclosedServicesMutex);

	apr_thread_mutex_lock(discovery->discoveredServicesMutex);
	hashMap_destroy(discovery->discoveredServices, false, false);
	discovery->discoveredServices = NULL;
	apr_thread_mutex_unlock(discovery->discoveredServicesMutex);

	apr_thread_mutex_lock(discovery->listenerReferencesMutex);
	hashMap_destroy(discovery->listenerReferences, false, false);
	discovery->listenerReferences = NULL;
	apr_thread_mutex_unlock(discovery->listenerReferencesMutex);

	return status;
}

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" added\n", endpoint->service, machtedFilter);
	disclosed_endpoint_entry_pt entry = NULL;
	apr_pool_t *childPool = NULL;
	status = apr_pool_create(&childPool, discovery->pool);

	if (status == CELIX_SUCCESS) {
		entry = apr_palloc(childPool, sizeof(*entry));
		if (entry == NULL) {
			status = CELIX_ENOMEM;
			apr_pool_destroy(childPool);
		} else {
			entry->pool = childPool;
			entry->endpointDescription = endpoint;
		}
	}

	if (status == CELIX_SUCCESS) {
		DNSServiceRef sdRef = NULL;
		DNSServiceErrorType error;
		TXTRecordRef txtRecord;

		TXTRecordCreate(&txtRecord, 256, NULL ); //TODO search for correct default record size
		char serviceId[16];
		sprintf(serviceId, "%li", endpoint->serviceId);

		TXTRecordSetValue(&txtRecord, "service", strlen(endpoint->service),
				endpoint->service);
		TXTRecordSetValue(&txtRecord, "service.id", strlen(serviceId),
				serviceId);
		TXTRecordSetValue(&txtRecord, "endpoint.id", strlen(endpoint->id),
				endpoint->id);
		TXTRecordSetValue(&txtRecord, "framework.uuid", strlen(discovery->frameworkUuid), discovery->frameworkUuid);

		hash_map_iterator_pt iter = hashMapIterator_create(
				endpoint->properties);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			char *key = hashMapEntry_getKey(entry);
			char *value = hashMapEntry_getValue(entry);
			TXTRecordSetValue(&txtRecord, key, strlen(value), value);
		}
		hashMapIterator_destroy(iter);

		int port = atoi(discovery->discoveryPort);
		int portInNetworkByteOrder = ((port << 8) & 0xFF00)
				| ((port >> 8) & 0xFF); //FIXME assuming little endian

		error = DNSServiceRegister(&sdRef, 0, 0, endpoint->service,
					OSGI_SERVICE_TYPE, NULL,
					NULL, portInNetworkByteOrder, /* In network byte order */
					TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord),
					NULL, NULL );

		if (error != kDNSServiceErr_NoError) {
			status = CELIX_ILLEGAL_STATE;
//			printf("Registered record in dns-sd got error code %i\n", error);
		} else {
			//entry->txtRecord=txtRecord; TODO
			entry->dnsServiceRef = sdRef;
			apr_thread_mutex_lock(discovery->disclosedServicesMutex);
			if (discovery->disclosedServices != NULL) {
				hashMap_put(discovery->disclosedServices, endpoint->id, entry);
			}
			apr_thread_mutex_unlock(discovery->disclosedServicesMutex);
		}
	}



	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	disclosed_endpoint_entry_pt entry = NULL;
	apr_thread_mutex_lock(discovery->disclosedServicesMutex);
	if (discovery->disclosedServices != NULL) {
		entry = hashMap_remove(discovery->disclosedServices, endpoint->id);
	}
	if (entry != NULL) {
		DNSServiceRefDeallocate(entry->dnsServiceRef);
		apr_pool_destroy(entry->pool);
	} else {
		status = CELIX_ILLEGAL_STATE;
	}
	apr_thread_mutex_unlock(discovery->disclosedServicesMutex);


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

		apr_thread_mutex_lock(discovery->discoveredServicesMutex);
		if (discovery->discoveredServices != NULL) {
			hash_map_iterator_pt iter = hashMapIterator_create(discovery->discoveredServices);
			while (hashMapIterator_hasNext(iter)) {
				endpoint_description_pt endpoint = hashMapIterator_nextKey(iter);
				endpoint_listener_pt listener = service;

				char *scope = properties_get(serviceProperties,
				(char *) OSGI_ENDPOINT_LISTENER_SCOPE);
				filter_pt filter = filter_create(scope); //FIXME memory leak
				bool matchResult = false;
				filter_match(filter, endpoint->properties, &matchResult);
				if (matchResult) {
					listener->endpointAdded(listener, endpoint, NULL);
				}
			}
			hashMapIterator_destroy(iter);
		}
		apr_thread_mutex_unlock(discovery->discoveredServicesMutex);

		apr_thread_mutex_lock(discovery->listenerReferencesMutex);
		if (discovery->listenerReferences != NULL) {
			hashMap_put(discovery->listenerReferences, reference, NULL /*TODO is the scope value needed?*/);
		}
		apr_thread_mutex_unlock(discovery->listenerReferencesMutex);
	}

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

//	printf("DISCOVERY: EndpointListener Modified - Update Scope TODO\n");

	return status;
}



celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("DISCOVERY: EndpointListener Removed\n");
	apr_thread_mutex_lock(discovery->listenerReferencesMutex);
	if (discovery->listenerReferences != NULL) {
		hashMap_remove(discovery->listenerReferences, reference);
	}
	apr_thread_mutex_unlock(discovery->listenerReferencesMutex);

	return status;
}


static celix_status_t discovery_informEndpointListeners(discovery_pt discovery, endpoint_description_pt endpoint, bool endpointAdded) {
	celix_status_t status = CELIX_SUCCESS;

	// Inform listeners of new endpoint
	apr_thread_mutex_lock(discovery->listenerReferencesMutex);
	if (discovery->listenerReferences != NULL) {
		hash_map_iterator_pt iter = hashMapIterator_create(discovery->listenerReferences);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			service_reference_pt reference = hashMapEntry_getKey(entry);
			endpoint_listener_pt listener = NULL;

			service_registration_pt registration = NULL;
			serviceReference_getServiceRegistration(reference, &registration);
			properties_pt serviceProperties = NULL;
			serviceRegistration_getProperties(registration, &serviceProperties);
			char *scope = properties_get(serviceProperties,
					(char *) OSGI_ENDPOINT_LISTENER_SCOPE);
			filter_pt filter = filter_create(scope);
			bool matchResult = false;
			filter_match(filter, endpoint->properties, &matchResult);
			if (matchResult) {
				printf("DISCOVERY: Add service (%s)\n", endpoint->service);
				bundleContext_getService(discovery->context, reference,
						(void**) &listener);
				if (endpointAdded) {
					listener->endpointAdded(listener->handle, endpoint, NULL );
				} else {
					listener->endpointRemoved(listener->handle, endpoint, NULL );
				}

			}
		}
		hashMapIterator_destroy(iter);
	}
	apr_thread_mutex_unlock(discovery->listenerReferencesMutex);

	return status;
}
