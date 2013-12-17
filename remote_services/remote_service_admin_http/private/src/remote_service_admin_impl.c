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
 * remote_service_admin_impl.c
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>

#include <apr_strings.h>
#include <apr_uuid.h>

#include <curl/curl.h>

#include "remote_service_admin_http_impl.h"
#include "export_registration_impl.h"
#include "import_registration_impl.h"
#include "remote_constants.h"
#include "constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "bundle.h"
#include "service_reference.h"
#include "service_registration.h"

struct post {
    const char *readptr;
    int size;
};

struct get {
    char *writeptr;
    int size;
};

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";


static const char *DEFAULT_PORT = "8888";

//void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info);
static int remoteServiceAdmin_callback(struct mg_connection *conn);
celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_pt admin, export_registration_pt registration, service_reference_pt reference, char *interface);
celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, properties_pt serviceProperties, properties_pt endpointProperties, char *interface, endpoint_description_pt *description);
static celix_status_t constructServiceUrl(remote_service_admin_pt admin, char *service, char **serviceUrl);

static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp);

celix_status_t remoteServiceAdmin_create(apr_pool_t *pool, bundle_context_pt context, remote_service_admin_pt *admin) {
	celix_status_t status = CELIX_SUCCESS;

	*admin = apr_palloc(pool, sizeof(**admin));
	if (!*admin) {
		status = CELIX_ENOMEM;
	} else {
		(*admin)->pool = pool;
		(*admin)->context = context;
		(*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
		(*admin)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);

		// Start webserver
		char *port = NULL;
		bundleContext_getProperty(context, "RSA_PORT", &port);
		if (port == NULL) {
			(*admin)->port = (char *)DEFAULT_PORT;
		} else {
			(*admin)->port = apr_pstrdup(pool, port);
		}
		printf("Start webserver: %s\n", (*admin)->port);
		const char *options[] = { "listening_ports", (*admin)->port, NULL};

		// Prepare callbacks structure. We have only one callback, the rest are NULL.
		struct mg_callbacks callbacks;
		memset(&callbacks, 0, sizeof(callbacks));
		callbacks.begin_request = remoteServiceAdmin_callback;

		(*admin)->ctx = mg_start(&callbacks, (*admin), options);
		printf("Start webserver %p\n", (*admin)->ctx);
	}

	return status;
}

celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);
	while (hashMapIterator_hasNext(iter)) {
		array_list_pt exports = hashMapIterator_nextValue(iter);
		int i;
		for (i = 0; i < arrayList_size(exports); i++) {
			export_registration_pt export = arrayList_get(exports, i);
			exportRegistration_stopTracking(export);
		}
	}
	iter = hashMapIterator_create(admin->importedServices);
	while (hashMapIterator_hasNext(iter)) {
		array_list_pt exports = hashMapIterator_nextValue(iter);
		int i;
		for (i = 0; i < arrayList_size(exports); i++) {
			import_registration_pt export = arrayList_get(exports, i);
			importRegistration_stopTracking(export);
		}
	}

	return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
//void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {

static int remoteServiceAdmin_callback(struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);
	if (request_info->uri != NULL) {
		printf("REMOTE_SERVICE_ADMIN: Handle request: %s\n", request_info->uri);
		remote_service_admin_pt rsa = request_info->user_data;

		if (strncmp(request_info->uri, "/service/", 9) == 0) {
			// uri = /services/myservice/call
			const char *uri = request_info->uri;
			// rest = myservice/call
			const char *rest = uri+9;
			int length = strlen(rest);
			char *interfaceStart = strchr(rest, '/');
			char *callStart = strchr(interfaceStart+1, '/');
			int pos = interfaceStart - rest;
			char service[pos+1];
			strncpy(service, rest, pos);
			service[pos] = '\0';

//			printf("Got service %s, interfaceStart is %s and callStart is %s\n", service, interfaceStart, callStart);

			char *request = callStart+1;

			const char *lengthStr = mg_get_header(conn, (const char *) "Content-Length");
			int datalength = apr_atoi64(lengthStr);
			char data[datalength+1];
			mg_read(conn, data, datalength);
			data[datalength] = '\0';

			hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
			while (hashMapIterator_hasNext(iter)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
				array_list_pt exports = hashMapEntry_getValue(entry);
				int expIt = 0;
				for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
					export_registration_pt export = arrayList_get(exports, expIt);
					long serviceId = atol(service);
					if (serviceId == export->endpointDescription->serviceId) {
						char *reply = NULL;
						export->endpoint->handleRequest(export->endpoint->endpoint, request, data, &reply);
						if (reply != NULL) {
							mg_printf(conn, "%s", ajax_reply_start);
							mg_printf(conn, "%s", reply);
						}
					}
				}
			}
		}
	}

	return 1;
}

celix_status_t remoteServiceAdmin_handleRequest(remote_service_admin_pt rsa, char *service, char *request, char *data, char **reply) {
	hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		array_list_pt exports = hashMapEntry_getValue(entry);
		int expIt = 0;
		for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
			export_registration_pt export = arrayList_get(exports, expIt);
			if (strcmp(service, export->endpointDescription->service) == 0) {
				export->endpoint->handleRequest(export->endpoint->endpoint, request, data, reply);
			}
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_pt admin, char *serviceId, properties_pt properties, array_list_pt *registrations) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(registrations);
	array_list_pt references = NULL;
	service_reference_pt reference = NULL;
	service_registration_pt registration = NULL;
	apr_pool_t *tmpPool = NULL;

	apr_pool_create(&tmpPool, admin->pool);
	if (tmpPool == NULL) {
		return CELIX_ENOMEM;
	} else {
		char *filter = apr_pstrcat(admin->pool, "(", (char *)OSGI_FRAMEWORK_SERVICE_ID, "=", serviceId, ")", NULL); /*FIXME memory leak*/
		bundleContext_getServiceReferences(admin->context, NULL, filter, &references);
		apr_pool_destroy(tmpPool);
		if (arrayList_size(references) >= 1) {
			reference = arrayList_get(references, 0);
		}
	}

	if (reference == NULL) {
		printf("ERROR: expected a reference for service id %s\n", serviceId);
		return CELIX_ILLEGAL_STATE;
	}


	serviceReference_getServiceRegistration(reference, &registration);
	properties_pt serviceProperties = NULL;
	serviceRegistration_getProperties(registration, &serviceProperties);
	char *exports = properties_get(serviceProperties, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES);
	char *provided = properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);

	if (exports == NULL || provided == NULL) {
		printf("RSA: No Services to export.\n");
	} else {
		printf("RSA: Export services (%s)\n", exports);
		array_list_pt interfaces = NULL;
		arrayList_create(&interfaces);
		if (strcmp(utils_stringTrim(exports), "*") == 0) {
			char *token;
			char *interface = apr_strtok(provided, ",", &token);
			while (interface != NULL) {
				arrayList_add(interfaces, utils_stringTrim(interface));
				interface = apr_strtok(NULL, ",", &token);
			}
		} else {
			char *exportToken;
			char *providedToken;

			char *pinterface = apr_strtok(provided, ",", &providedToken);
			while (pinterface != NULL) {
				char *einterface = apr_strtok(exports, ",", &exportToken);
				while (einterface != NULL) {
					if (strcmp(einterface, pinterface) == 0) {
						arrayList_add(interfaces, einterface);
					}
					einterface = apr_strtok(NULL, ",", &exportToken);
				}
				pinterface = apr_strtok(NULL, ",", &providedToken);
			}
		}

		if (arrayList_size(interfaces) != 0) {
			int iter = 0;
			for (iter = 0; iter < arrayList_size(interfaces); iter++) {
				char *interface = arrayList_get(interfaces, iter);
				export_registration_pt registration = NULL;

				exportRegistration_create(admin->pool, reference, NULL, admin, admin->context, &registration);
				arrayList_add(*registrations, registration);

				remoteServiceAdmin_installEndpoint(admin, registration, reference, interface);
				exportRegistration_open(registration);
				exportRegistration_startTracking(registration);
			}
			hashMap_put(admin->exportedServices, reference, *registrations);
		}
		arrayList_destroy(interfaces);
	}

	return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    remote_service_admin_pt admin = registration->rsa;
    array_list_pt registrations = NULL;

    registrations = hashMap_remove(admin->exportedServices, registration->reference);
    // Registrations are allocated on the RSA pool, so there is a "leak" here. Removed services still consume memory

    return CELIX_SUCCESS;
}

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_pt admin, export_registration_pt registration, service_reference_pt reference, char *interface) {
	celix_status_t status = CELIX_SUCCESS;
	properties_pt endpointProperties = properties_create();
	properties_pt serviceProperties = NULL;

	service_registration_pt sRegistration = NULL;
	serviceReference_getServiceRegistration(reference, &sRegistration);

	serviceRegistration_getProperties(sRegistration, &serviceProperties);

	hash_map_iterator_pt iter = hashMapIterator_create(serviceProperties);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		char *key = (char *) hashMapEntry_getKey(entry);
		char *value = (char *) hashMapEntry_getValue(entry);

		properties_set(endpointProperties, key, value);
	}
	char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
	char *uuid = NULL;
	bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	properties_set(endpointProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS, interface);
	properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);
	properties_set(endpointProperties, "service.imported", "true");
	properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);

//    properties_set(endpointProperties, ".ars.path", buf);
//    properties_set(endpointProperties, ".ars.port", admin->port);

	char buf[512];
	sprintf(buf, "/service/%s/%s", serviceId, interface);
    char *url = NULL;
    constructServiceUrl(admin,buf, &url);
    properties_set(endpointProperties, ".ars.alias", url);

    properties_set(endpointProperties, "service.imported.configs", ".ars");

	endpoint_description_pt endpointDescription = NULL;
	remoteServiceAdmin_createEndpointDescription(admin, serviceProperties, endpointProperties, interface, &endpointDescription);
	exportRegistration_setEndpointDescription(registration, endpointDescription);

	return status;
}

static celix_status_t constructServiceUrl(remote_service_admin_pt admin, char *service, char **serviceUrl) {
	celix_status_t status = CELIX_SUCCESS;
    
	if (*serviceUrl != NULL || admin == NULL || service == NULL ) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		char host[APRMAXHOSTLEN + 1];
		apr_sockaddr_t *sa;
		char *ip;
        
		apr_status_t stat = apr_gethostname(host, APRMAXHOSTLEN + 1, admin->pool); /*TODO mem leak*/
		if (stat != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			stat = apr_sockaddr_info_get(&sa, host, APR_INET, 0, 0, admin->pool); /*TODO mem leak*/
			if (stat != APR_SUCCESS) {
				status = CELIX_BUNDLE_EXCEPTION;
			} else {
				stat = apr_sockaddr_ip_get(&ip, sa);
				if (stat != APR_SUCCESS) {
					status = CELIX_BUNDLE_EXCEPTION;
				} else {
					*serviceUrl = apr_pstrcat(admin->pool, "http://", ip, ":", admin->port, service, NULL );
				}
			}
		}
	}
    
	return status;
}



celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, properties_pt serviceProperties,
		properties_pt endpointProperties, char *interface, endpoint_description_pt *description) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *childPool = NULL;
	apr_pool_create(&childPool, admin->pool); //TODO pool should be destroyed after when endpoint is removed

	*description = apr_palloc(childPool, sizeof(*description));
//	*description = malloc(sizeof(*description));
	if (!*description) {
		status = CELIX_ENOMEM;
	} else {
		char *uuid = NULL;
		status = bundleContext_getProperty(admin->context, (char *)OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
		if (status == CELIX_SUCCESS) {
			(*description)->properties = endpointProperties;
			(*description)->frameworkUUID = uuid;
			(*description)->serviceId = apr_atoi64(properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_SERVICE_ID));
			(*description)->id = apr_pstrdup(childPool, "TODO"); // does not work, txt record to big ?? --> apr_pstrcat(childPool, uuid, "-", (*description)->serviceId, NULL);
			(*description)->service = interface;
		}
	}

	return status;
}

celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_pt admin, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_pt admin, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_pt admin, endpoint_description_pt endpoint, import_registration_pt *registration) {
	celix_status_t status = CELIX_SUCCESS;

	printf("RSA: Import service %s\n", endpoint->service);
	importRegistration_create(admin->pool, endpoint, admin, admin->context, registration);

	array_list_pt importedRegs = hashMap_get(admin->importedServices, endpoint);
	if (importedRegs == NULL) {
		arrayList_create(&importedRegs);
		hashMap_put(admin->importedServices, endpoint, importedRegs);
	}
	arrayList_add(importedRegs, *registration);

	importRegistration_open(*registration);
	importRegistration_startTracking(*registration);

	return status;
}

celix_status_t remoteServiceAdmin_send(remote_service_admin_pt rsa, endpoint_description_pt endpointDescription, char *methodSignature, char *request, char **reply, int* replyStatus) {

    struct post post;
    post.readptr = request;
    post.size = strlen(request);

    struct get get;
    get.size = 0;
    get.writeptr = malloc(1);

    char *serviceUrl = properties_get(endpointDescription->properties, ".ars.alias");
    printf("CALCULATOR_PROXY: URL: %s\n", serviceUrl);
    char *url = apr_pstrcat(rsa->pool, serviceUrl, "/", methodSignature, NULL);

    celix_status_t status = CELIX_SUCCESS;
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(!curl) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, remoteServiceAdmin_readCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &post);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, remoteServiceAdmin_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&get);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)post.size);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        printf("CALCULATOR_PROXY: Data read: \"%s\" %d\n", get.writeptr, res);
        *reply = get.writeptr;

    }
    return status;
}

static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct post *post = userp;

    if (post->size) {
        *(char *) ptr = post->readptr[0];
        post->readptr++;
        post->size--;
        return 1;
    }

    return 0;
}

static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct get *mem = (struct get *)userp;

  mem->writeptr = realloc(mem->writeptr, mem->size + realsize + 1);
  if (mem->writeptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->writeptr[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->writeptr[mem->size] = 0;

  return realsize;
}


celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	*endpoint = reference->endpoint;

	return status;
}

celix_status_t exportReference_getExportedService(export_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedService(import_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
