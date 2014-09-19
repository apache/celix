/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * endpoint_discovery_server.c
 *
 * \date		Aug 12, 2014
 * \author		<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdint.h>

#include "civetweb.h"
#include "celix_errno.h"
#include "utils.h"
#include "celix_log.h"
#include "discovery.h"
#include "discovery_impl.h"

#include "endpoint_descriptor_writer.h"
#include "endpoint_discovery_server.h"


#define DEFAULT_SERVER_THREADS "1"

#define CIVETWEB_REQUEST_NOT_HANDLED 0
#define CIVETWEB_REQUEST_HANDLED 1

static const char *response_headers =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/xml;charset=utf-8\r\n"
  "\r\n";

struct endpoint_discovery_server {
    hash_map_pt entries; // key = endpointId, value = endpoint_descriptor_pt

    celix_thread_mutex_t serverLock;

    const char* path;
    struct mg_context* ctx;
};

// Forward declarations...
static int endpointDiscoveryServer_callback(struct mg_connection *conn);
static char* format_path(char* path);

celix_status_t endpointDiscoveryServer_create(discovery_pt discovery, bundle_context_pt context, endpoint_discovery_server_pt *server) {
	celix_status_t status = CELIX_SUCCESS;

	*server = malloc(sizeof(struct endpoint_discovery_server));
	if (!*server) {
		return CELIX_ENOMEM;
	}

	(*server)->entries = hashMap_create(&utils_stringHash, NULL, &utils_stringEquals, NULL);
	if (!(*server)->entries) {
		return CELIX_ENOMEM;
	}

	status = celixThreadMutex_create(&(*server)->serverLock, NULL);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	char *port = NULL;
	bundleContext_getProperty(context, DISCOVERY_SERVER_PORT, &port);
	if (port == NULL) {
		port = DEFAULT_SERVER_PORT;
	}

	char *path = NULL;
	bundleContext_getProperty(context, DISCOVERY_SERVER_PATH, &path);
	if (path == NULL) {
		path = DEFAULT_SERVER_PATH;
	}

	(*server)->path = format_path(path);

	const char *options[] = {
		"listening_ports", port,
		"num_threads", DEFAULT_SERVER_THREADS,
		NULL
	};

	const struct mg_callbacks callbacks = {
		.begin_request = endpointDiscoveryServer_callback,
	};

	(*server)->ctx = mg_start(&callbacks, (*server), options);

	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Starting discovery server on port %s...", port);

	return status;
}

celix_status_t endpointDiscoveryServer_destroy(endpoint_discovery_server_pt server) {
	celix_status_t status = CELIX_SUCCESS;

	// stop & block until the actual server is shut down...
	if (server->ctx != NULL) {
		mg_stop(server->ctx);
		server->ctx = NULL;
	}

	status = celixThreadMutex_lock(&server->serverLock);

	hashMap_destroy(server->entries, true /* freeKeys */, false /* freeValues */);

	status = celixThreadMutex_unlock(&server->serverLock);
	status = celixThreadMutex_destroy(&server->serverLock);

	free((void*) server->path);
	free(server);

	return status;
}

celix_status_t endpointDiscoveryServer_addEndpoint(endpoint_discovery_server_pt server, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	status = celixThreadMutex_lock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	// create a local copy of the endpointId which we can control...
	char* endpointId = strdup(endpoint->id);
	endpoint_description_pt cur_value = hashMap_get(server->entries, endpointId);
	if (!cur_value) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "exposing new endpoint \"%s\"...", endpointId);

		hashMap_put(server->entries, endpointId, endpoint);
	}

	status = celixThreadMutex_unlock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

celix_status_t endpointDiscoveryServer_removeEndpoint(endpoint_discovery_server_pt server, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	status = celixThreadMutex_lock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	hash_map_entry_pt entry = hashMap_getEntry(server->entries, endpoint->id);
	if (entry) {
		char* key = hashMapEntry_getKey(entry);

		fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "removing endpoint \"%s\"...\n", key);

		hashMap_remove(server->entries, key);

		// we've made this key, see _addEnpoint above...
		free((void*) key);
	}

	status = celixThreadMutex_unlock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

static char* format_path(char* path) {
	char* result = strdup(utils_stringTrim(path));
	// check whether the path starts with a leading slash...
	if (result[0] != '/') {
		size_t len = strlen(result);
		result = realloc(result, len + 2);
		memmove(result + 1, result, len);
		result[0] = '/';
		result[len + 1] = 0;
	}
	return result;
}

static celix_status_t endpointDiscoveryServer_getEndpoints(endpoint_discovery_server_pt server, const char* the_endpoint_id, array_list_pt *endpoints) {
	celix_status_t status = CELIX_SUCCESS;

	status = arrayList_create(endpoints);
	if (status != CELIX_SUCCESS) {
		return CELIX_ENOMEM;
	}

	status = celixThreadMutex_lock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	hash_map_iterator_pt iter = hashMapIterator_create(server->entries);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

		char* endpoint_id = hashMapEntry_getKey(entry);
		if (the_endpoint_id == NULL || strcmp(the_endpoint_id, endpoint_id) == 0) {
			endpoint_description_pt endpoint = hashMapEntry_getValue(entry);

			arrayList_add(*endpoints, endpoint);
		}
	}
	hashMapIterator_destroy(iter);

	status = celixThreadMutex_unlock(&server->serverLock);
	if (status != CELIX_SUCCESS) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

static int endpointDiscoveryServer_writeEndpoints(struct mg_connection* conn, array_list_pt endpoints) {
	celix_status_t status = CELIX_SUCCESS;

    endpoint_descriptor_writer_pt writer = NULL;
    status = endpointDescriptorWriter_create(&writer);
    if (status != CELIX_SUCCESS) {
    	return CIVETWEB_REQUEST_NOT_HANDLED;
    }

    char *buffer = NULL;
    status = endpointDescriptorWriter_writeDocument(writer, endpoints, &buffer);
    if (buffer) {
    	mg_write(conn, response_headers, strlen(response_headers));
    	mg_write(conn, buffer, strlen(buffer));
    }

    status = endpointDescriptorWriter_destroy(writer);

	return CIVETWEB_REQUEST_HANDLED;
}

// returns all endpoints as XML...
static int endpointDiscoveryServer_returnAllEndpoints(endpoint_discovery_server_pt server, struct mg_connection* conn) {
	int status = CIVETWEB_REQUEST_NOT_HANDLED;

	array_list_pt endpoints = NULL;
	endpointDiscoveryServer_getEndpoints(server, NULL, &endpoints);
	if (endpoints) {
		status = endpointDiscoveryServer_writeEndpoints(conn, endpoints);

		arrayList_destroy(endpoints);
	}

	return status;
}

// returns a single endpoint as XML...
static int endpointDiscoveryServer_returnEndpoint(endpoint_discovery_server_pt server, struct mg_connection* conn, const char* endpoint_id) {
	int status = CIVETWEB_REQUEST_NOT_HANDLED;

	array_list_pt endpoints = NULL;
	endpointDiscoveryServer_getEndpoints(server, endpoint_id, &endpoints);
	if (endpoints) {
		status = endpointDiscoveryServer_writeEndpoints(conn, endpoints);

		arrayList_destroy(endpoints);
	}

	return status;
}

static int endpointDiscoveryServer_callback(struct mg_connection* conn) {
	int status = CIVETWEB_REQUEST_NOT_HANDLED;

	const struct mg_request_info *request_info = mg_get_request_info(conn);
	if (request_info->uri != NULL && strcmp("GET", request_info->request_method) == 0) {
		endpoint_discovery_server_pt server = request_info->user_data;

		const char *uri = request_info->uri;
		const size_t path_len = strlen(server->path);
		const size_t uri_len = strlen(uri);

		if (strncmp(server->path, uri, strlen(server->path)) == 0) {
			// Be lenient when it comes to the trailing slash...
			if (path_len == uri_len || (uri_len == (path_len + 1) && uri[path_len] == '/')) {
				status = endpointDiscoveryServer_returnAllEndpoints(server, conn);
			} else {
				const char* endpoint_id = uri + path_len + 1; // right after the slash...

				status = endpointDiscoveryServer_returnEndpoint(server, conn, endpoint_id);
			}
		}
	}

	return status;
}
