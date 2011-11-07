/*
 * remote_service_admin_impl.c
 *
 *  Created on: Sep 30, 2011
 *      Author: alexander
 */
#include <stdio.h>
#include <stdlib.h>

#include <apr_uuid.h>
#include <apr_strings.h>

#include "headers.h"
#include "remote_service_admin_impl.h"
#include "export_registration_impl.h"
#include "import_registration_impl.h"
#include "remote_constants.h"
#include "constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "bundle.h"

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";

void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info);
celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_t admin, export_registration_t registration, SERVICE_REFERENCE reference, char *interface);
celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t admin, PROPERTIES serviceProperties, PROPERTIES endpointProperties, char *interface, endpoint_description_t *description);
celix_status_t remoteServiceAdmin_getUUID(remote_service_admin_t rsa, char **uuidStr);

celix_status_t remoteServiceAdmin_create(apr_pool_t *pool, BUNDLE_CONTEXT context, remote_service_admin_t *admin) {
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
		const char *port = getenv("RSA_PORT");
		const char *options[] = {"listening_ports", port, NULL};
		(*admin)->ctx = mg_start(remoteServiceAdmin_callback, (*admin), options);
	}

	return status;
}

celix_status_t remoteServiceAdmin_stop(remote_service_admin_t admin) {
	celix_status_t status = CELIX_SUCCESS;

	HASH_MAP_ITERATOR iter = hashMapIterator_create(admin->exportedServices);
	while (hashMapIterator_hasNext(iter)) {
		ARRAY_LIST exports = hashMapIterator_nextValue(iter);
		int i;
		for (i = 0; i < arrayList_size(exports); i++) {
			export_registration_t export = arrayList_get(exports, i);
			exportRegistration_stopTracking(export);
		}
	}
	iter = hashMapIterator_create(admin->importedServices);
	while (hashMapIterator_hasNext(iter)) {
		ARRAY_LIST exports = hashMapIterator_nextValue(iter);
		int i;
		for (i = 0; i < arrayList_size(exports); i++) {
			import_registration_t export = arrayList_get(exports, i);
			importRegistration_stopTracking(export);
		}
	}

	return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {
	if (request_info->uri != NULL) {
		printf("RSA Handle request: %s\n", request_info->uri);
		remote_service_admin_t rsa = request_info->user_data;

		if (strncmp(request_info->uri, "/services/", 10) == 0) {
			// uri = /services/myservice/call
			char *uri = request_info->uri;
			// rest = myservice/call
			char *rest = uri+10;
			int length = strlen(rest);
			char *callStart = strchr(rest, '/');
			int pos = callStart - rest;
			char service[pos+1];
			strncpy(service, rest, pos);
			service[pos] = '\0';
			printf("RSA Find Handler for Service: %s\n", service);

			char request[length - pos];
			strncpy(request, rest + pos + 1, length - pos);
			printf("RSA Send Request: %s\n", request);

			const char *lengthStr = mg_get_header(conn, (const char *) "Content-Length");
			int datalength = apr_atoi64(lengthStr);
			char data[datalength+1];
			mg_read(conn, data, datalength);
			data[datalength] = '\0';

			HASH_MAP_ITERATOR iter = hashMapIterator_create(rsa->exportedServices);
			while (hashMapIterator_hasNext(iter)) {
				HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
				ARRAY_LIST exports = hashMapEntry_getValue(entry);
				int expIt = 0;
				for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
					export_registration_t export = arrayList_get(exports, expIt);
					if (strcmp(service, export->endpointDescription->service) == 0) {
						printf("RSA Sending Request: %s\n", request);
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

	return "";
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t admin, SERVICE_REFERENCE reference, PROPERTIES properties, ARRAY_LIST *registrations) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(admin->pool, registrations);


	char *exports = properties_get(reference->registration->properties, (char *) SERVICE_EXPORTED_INTERFACES);
	char *provided = properties_get(reference->registration->properties, (char *) OBJECTCLASS);

	if (exports == NULL || provided == NULL) {
		printf("RSA: No Services to export.\n");
	} else {
		printf("RSA: Export services (%s)\n", exports);
		ARRAY_LIST interfaces = NULL;
		arrayList_create(admin->pool, &interfaces);
		if (strcmp(string_trim(exports), "*") == 0) {
			char *token;
			char *interface = apr_strtok(provided, ",", &token);
			while (interface != NULL) {
				arrayList_add(interfaces, string_trim(interface));
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
				export_registration_t registration = NULL;

				exportRegistration_create(admin->pool, reference, NULL, admin, admin->context, &registration);
				arrayList_add(*registrations, registration);

				remoteServiceAdmin_installEndpoint(admin, registration, reference, interface);
				exportRegistration_open(registration);
				exportRegistration_startTracking(registration);
			}
			hashMap_put(admin->exportedServices, reference, *registrations);
		}
	}

	return status;
}

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_t admin, export_registration_t registration, SERVICE_REFERENCE reference, char *interface) {
	celix_status_t status = CELIX_SUCCESS;
	PROPERTIES endpointProperties = properties_create();

	HASH_MAP_ITERATOR iter = hashMapIterator_create(reference->registration->properties);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		char *key = (char *) hashMapEntry_getKey(entry);
		char *value = (char *) hashMapEntry_getValue(entry);

		properties_set(endpointProperties, key, value);
	}
	char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) SERVICE_ID);
	properties_set(endpointProperties, (char *) OBJECTCLASS, interface);
	properties_set(endpointProperties, (char *) ENDPOINT_SERVICE_ID, serviceId);
	char *uuid = NULL;
	remoteServiceAdmin_getUUID(admin, &uuid);
	properties_set(endpointProperties, (char *) ENDPOINT_FRAMEWORK_UUID, uuid);
	char *service = "/services/example";
	properties_set(endpointProperties, (char *) SERVICE_LOCATION, apr_pstrdup(admin->pool, service));

	endpoint_description_t endpointDescription = NULL;
	remoteServiceAdmin_createEndpointDescription(admin, reference->registration->properties, endpointProperties, interface, &endpointDescription);
	exportRegistration_setEndpointDescription(registration, endpointDescription);

	return status;
}

celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t admin, PROPERTIES serviceProperties,
		PROPERTIES endpointProperties, char *interface, endpoint_description_t *description) {
	celix_status_t status = CELIX_SUCCESS;

	*description = apr_palloc(admin->pool, sizeof(*description));
//	*description = malloc(sizeof(*description));
	if (!*description) {
		status = CELIX_ENOMEM;
	} else {
		char *uuid = NULL;
		status = bundleContext_getProperty(admin->context, ENDPOINT_FRAMEWORK_UUID, &uuid);
		if (status == CELIX_SUCCESS) {
			(*description)->properties = endpointProperties;
			(*description)->frameworkUUID = uuid;
			(*description)->serviceId = apr_atoi64(properties_get(serviceProperties, (char *) SERVICE_ID));
			(*description)->id = properties_get(endpointProperties, (char *) SERVICE_LOCATION);
			(*description)->service = interface;
		}
	}

	return status;
}

celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t admin, ARRAY_LIST *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t admin, ARRAY_LIST *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_t admin, endpoint_description_t endpoint, import_registration_t *registration) {
	celix_status_t status = CELIX_SUCCESS;

	printf("RSA: Import service %s\n", endpoint->service);
	importRegistration_create(admin->pool, endpoint, admin, admin->context, registration);

	ARRAY_LIST importedRegs = hashMap_get(admin->importedServices, endpoint);
	if (importedRegs == NULL) {
		arrayList_create(admin->pool, &importedRegs);
		hashMap_put(admin->importedServices, endpoint, importedRegs);
	}
	arrayList_add(importedRegs, *registration);

	importRegistration_open(*registration);
	importRegistration_startTracking(*registration);

	return status;
}


celix_status_t exportReference_getExportedEndpoint(export_reference_t reference, endpoint_description_t *endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	*endpoint = reference->endpoint;

	return status;
}

celix_status_t exportReference_getExportedService(export_reference_t reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_t reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedService(import_reference_t reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_getUUID(remote_service_admin_t rsa, char **uuidStr) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleContext_getProperty(rsa->context, ENDPOINT_FRAMEWORK_UUID, uuidStr);
	if (status == CELIX_SUCCESS) {
		if (*uuidStr == NULL) {
			apr_uuid_t uuid;
			apr_uuid_get(&uuid);
			*uuidStr = apr_palloc(rsa->pool, APR_UUID_FORMATTED_LENGTH + 1);
			apr_uuid_format(*uuidStr, &uuid);
			setenv(ENDPOINT_FRAMEWORK_UUID, *uuidStr, 1);
		}
	}

	return status;
}
