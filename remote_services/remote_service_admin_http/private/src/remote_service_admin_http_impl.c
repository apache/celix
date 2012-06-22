/*
 * remote_service_admin_http.c
 *
 *  Created on: May 24, 2012
 *      Author: alexander
 */
#include <stdlib.h>
#include <stdio.h>

#include <celix_errno.h>

typedef struct remote_service_admin_http *remote_service_admin_http_t;

struct remote_service_admin_http {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;

	struct mg_context *ctx;
};

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";

void *remoteServiceAdminHttp_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info);

celix_status_t remoteServiceAdminHttp_create(apr_pool_t *pool, BUNDLE_CONTEXT context, remote_service_admin_http_t *httpAdmin) {
	celix_status_t status = CELIX_SUCCESS;

	*httpAdmin = apr_palloc(pool, sizeof(**httpAdmin));
	if (!*httpAdmin) {
		status = CELIX_ENOMEM;
	} else {
		(*httpAdmin)->pool = pool;
		(*httpAdmin)->context = context;

		// Start webserver
		char *port = NULL;
		bundleContext_getProperty(context, "RSA_PORT", &port);
		if (port == NULL) {
			printf("No RemoteServiceAdmin port set, set it using RSA_PORT!\n");
		}
		const char *options[] = {"listening_ports", port, NULL};
		(*httpAdmin)->ctx = mg_start(remoteServiceAdminHttp_callback, (*httpAdmin), options);
	}

	return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
void *remoteServiceAdminHttp_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {
	if (request_info->uri != NULL) {
		printf("REMOTE_SERVICE_ADMIN: Handle request: %s\n", request_info->uri);
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

			char request[length - pos];
			strncpy(request, rest + pos + 1, length - pos);

			const char *lengthStr = mg_get_header(conn, (const char *) "Content-Length");
			int datalength = apr_atoi64(lengthStr);
			char data[datalength+1];
			mg_read(conn, data, datalength);
			data[datalength] = '\0';

			char *reply = NULL;
			remoteServiceAdmin_handleRequest(rsa, service, request, data, &reply);

			HASH_MAP_ITERATOR iter = hashMapIterator_create(rsa->exportedServices);
			while (hashMapIterator_hasNext(iter)) {
				HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
				ARRAY_LIST exports = hashMapEntry_getValue(entry);
				int expIt = 0;
				for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
					export_registration_t export = arrayList_get(exports, expIt);
					if (strcmp(service, export->endpointDescription->service) == 0) {
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
