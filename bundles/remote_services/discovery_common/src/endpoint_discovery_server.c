/*
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

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef ANDROID
#include <ifaddrs.h>
#endif
#include "civetweb.h"
#include "celix_errno.h"
#include "celix_utils.h"
#include "utils.h"
#include "celix_log_helper.h"
#include "discovery.h"
#include "endpoint_descriptor_writer.h"

// defines how often the webserver is restarted (with an increased port number)
#define MAX_NUMBER_OF_RESTARTS     15
#define DEFAULT_SERVER_THREADS     "5"

#define CIVETWEB_REQUEST_NOT_HANDLED 0
#define CIVETWEB_REQUEST_HANDLED 1

static const char *response_headers =
        "HTTP/1.1 200 OK\r\n"
        "Cache: no-cache\r\n"
        "Content-Type: application/xml;charset=utf-8\r\n"
        "\r\n";

struct endpoint_discovery_server {
    celix_log_helper_t **loghelper;
    hash_map_pt entries; // key = endpointId, value = endpoint_descriptor_pt

    celix_thread_mutex_t serverLock;

    const char *path;
    const char *port;
    const char *ip;
    struct mg_context *ctx;
};

// Forward declarations...
static int endpointDiscoveryServer_callback(struct mg_connection *conn);
static char* format_path(const char* path);

#ifndef ANDROID
static celix_status_t endpointDiscoveryServer_getIpAddress(char* interface, char** ip);
#endif

celix_status_t endpointDiscoveryServer_create(discovery_t *discovery,
                                              celix_bundle_context_t *context,
                                              const char* defaultServerPath,
                                              const char* defaultServerPort,
                                              const char* defaultServerIp,
                                              endpoint_discovery_server_t **server) {
    celix_status_t status;

    const char *port = NULL;
    const char *ip = NULL;
    char *detectedIp = NULL;
    const char *path = NULL;
    const char *retries = NULL;

    int max_ep_num = MAX_NUMBER_OF_RESTARTS;

    *server = malloc(sizeof(struct endpoint_discovery_server));
    if (!*server) {
        return CELIX_ENOMEM;
    }

    (*server)->loghelper = &discovery->loghelper;
    (*server)->entries = hashMap_create(&utils_stringHash, NULL, &utils_stringEquals, NULL);
    if (!(*server)->entries) {
        return CELIX_ENOMEM;
    }

    status = celixThreadMutex_create(&(*server)->serverLock, NULL);
    if (status != CELIX_SUCCESS) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    bundleContext_getProperty(context, DISCOVERY_SERVER_IP, &ip);
#ifndef ANDROID
    if (ip == NULL) {
        const char *interface = NULL;

        bundleContext_getProperty(context, DISCOVERY_SERVER_INTERFACE, &interface);
        if ((interface != NULL) && (endpointDiscoveryServer_getIpAddress((char*)interface, &detectedIp) != CELIX_SUCCESS)) {
            celix_logHelper_warning(*(*server)->loghelper, "Could not retrieve IP address for interface %s", interface);
        }

        if (detectedIp == NULL) {
            endpointDiscoveryServer_getIpAddress(NULL, &detectedIp);
        }

        ip = detectedIp;
    }
#endif

    if (ip != NULL) {
        celix_logHelper_info(*(*server)->loghelper, "Using %s for service annunciation", ip);
        (*server)->ip = strdup(ip);
    }
    else {
        celix_logHelper_warning(*(*server)->loghelper, "No IP address for service annunciation set. Using %s", defaultServerIp);
        (*server)->ip = strdup((char*) defaultServerIp);
    }

    if (detectedIp != NULL) {
        free(detectedIp);
    }

    bundleContext_getProperty(context, DISCOVERY_SERVER_PORT, &port);
    if (port == NULL) {
        port = defaultServerPort;
    }

    bundleContext_getProperty(context, DISCOVERY_SERVER_PATH, &path);
    if (path == NULL) {
        path = defaultServerPath;
    }

    bundleContext_getProperty(context, DISCOVERY_SERVER_MAX_EP, &retries);
    if (retries != NULL) {
        errno=0;
        max_ep_num = strtol(retries,NULL,10);
        if(errno!=0 || max_ep_num<=0){
            max_ep_num=MAX_NUMBER_OF_RESTARTS;
        }
    }

    (*server)->path = format_path(path);

    const struct mg_callbacks callbacks = {
            .begin_request = endpointDiscoveryServer_callback,
    };

    unsigned int port_counter = 0;
    char newPort[10];
    bool bindToAllInterfaces = celix_bundleContext_getPropertyAsBool(context, CELIX_DISCOVERY_BIND_ON_ALL_INTERFACES, CELIX_DISCOVERY_BIND_ON_ALL_INTERFACES_DEFAULT);

    do {
        char *listeningPorts = NULL;
        if (bindToAllInterfaces) {
            asprintf(&listeningPorts,"0.0.0.0:%s", port);
        } else {
            asprintf(&listeningPorts,"%s:%s", (*server)->ip, port);
        }

        const char *options[] = {
                "listening_ports", listeningPorts,
                "num_threads", DEFAULT_SERVER_THREADS,
                NULL
        };

        (*server)->ctx = mg_start(&callbacks, (*server), options);

        if ((*server)->ctx != NULL)
        {
            celix_logHelper_info(discovery->loghelper, "Starting discovery server on port %s...", listeningPorts);
        }
        else {
            errno = 0;
            char* endptr = (char*)port;
            long currentPort = strtol(port, &endptr, 10);

            if (*endptr || errno != 0) {
                currentPort = strtol(defaultServerPort, NULL, 10);
            }

            port_counter++;
            snprintf(&newPort[0], 10,  "%ld", (currentPort+1));

            celix_logHelper_warning(discovery->loghelper, "Error while starting discovery server on port %s - retrying on port %s...", port, newPort);
            port = newPort;

        }

        free(listeningPorts);

    } while(((*server)->ctx == NULL) && (port_counter < max_ep_num));

    (*server)->port = strdup(port);

    return status;
}

celix_status_t endpointDiscoveryServer_getUrl(endpoint_discovery_server_t *server, char* url, size_t maxLenUrl) {

    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    if (server->ip && server->port && server->path) {
        int written = snprintf(url, maxLenUrl, "http://%s:%s/%s", server->ip, server->port, server->path);
        status = written < maxLenUrl && written > 0 ? CELIX_SUCCESS : CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t endpointDiscoveryServer_destroy(endpoint_discovery_server_t *server) {
    celix_status_t status;

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
    free((void*) server->port);
    free((void*) server->ip);

    free(server);

    return status;
}

celix_status_t endpointDiscoveryServer_addEndpoint(endpoint_discovery_server_t *server, endpoint_description_t *endpoint) {
    celix_status_t status;

    status = celixThreadMutex_lock(&server->serverLock);
    if (status != CELIX_SUCCESS) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    // create a local copy of the endpointId which we can control...
    char* endpointId = strdup(endpoint->id);
    endpoint_description_t *cur_value = hashMap_get(server->entries, endpointId);
    if (!cur_value) {
        celix_logHelper_info(*server->loghelper, "exposing new endpoint \"%s\"...", endpointId);

        hashMap_put(server->entries, endpointId, endpoint);
    }

    status = celixThreadMutex_unlock(&server->serverLock);
    if (status != CELIX_SUCCESS) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t endpointDiscoveryServer_removeEndpoint(endpoint_discovery_server_t *server, endpoint_description_t *endpoint) {
    celix_status_t status;

    status = celixThreadMutex_lock(&server->serverLock);
    if (status != CELIX_SUCCESS) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    hash_map_entry_pt entry = hashMap_getEntry(server->entries, endpoint->id);
    if (entry) {
        char* key = hashMapEntry_getKey(entry);

        celix_logHelper_info(*server->loghelper, "removing endpoint \"%s\"...\n", key);

        hashMap_remove(server->entries, key);

        // we've made this key, see _addEndpoint above...
        free((void*) key);
    }

    status = celixThreadMutex_unlock(&server->serverLock);
    if (status != CELIX_SUCCESS) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

static char* format_path(const char* path) {
    char* result = celix_utils_trim(path);
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

static celix_status_t endpointDiscoveryServer_getEndpoints(endpoint_discovery_server_t *server, const char* the_endpoint_id, celix_array_list_t** endpoints) {
    *endpoints = celix_arrayList_create();
    if (!(*endpoints)) {
        return CELIX_ENOMEM;
    }


    hash_map_iterator_pt iter = hashMapIterator_create(server->entries);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

        char* endpoint_id = hashMapEntry_getKey(entry);
        if (the_endpoint_id == NULL || strcmp(the_endpoint_id, endpoint_id) == 0) {
            endpoint_description_t *endpoint = hashMapEntry_getValue(entry);

            celix_arrayList_add(*endpoints, endpoint);
        }
    }
    hashMapIterator_destroy(iter);

    return CELIX_SUCCESS;
}

static int endpointDiscoveryServer_writeEndpoints(struct mg_connection* conn, celix_array_list_t* endpoints) {
    celix_status_t status;
    int rv = CIVETWEB_REQUEST_NOT_HANDLED;

    endpoint_descriptor_writer_t *writer = NULL;
    status = endpointDescriptorWriter_create(&writer);
    if (status == CELIX_SUCCESS) {

        char *buffer = NULL;
        status = endpointDescriptorWriter_writeDocument(writer, endpoints, &buffer);
        if (buffer) {
            mg_write(conn, response_headers, strlen(response_headers));
            mg_write(conn, buffer, strlen(buffer));
        }

        rv = CIVETWEB_REQUEST_HANDLED;
    }

    if (writer != NULL) {
        endpointDescriptorWriter_destroy(writer);
    }

    return rv;
}

// returns all endpoints as XML...
static int endpointDiscoveryServer_returnAllEndpoints(endpoint_discovery_server_t *server, struct mg_connection* conn) {
    int status = CIVETWEB_REQUEST_NOT_HANDLED;

    celix_array_list_t* endpoints = NULL;

    if (celixThreadMutex_lock(&server->serverLock) == CELIX_SUCCESS) {
        endpointDiscoveryServer_getEndpoints(server, NULL, &endpoints);
        if (endpoints) {
            status = endpointDiscoveryServer_writeEndpoints(conn, endpoints);

            celix_arrayList_destroy(endpoints);
        }


        celixThreadMutex_unlock(&server->serverLock);
    }

    return status;
}

// returns a single endpoint as XML...
static int endpointDiscoveryServer_returnEndpoint(endpoint_discovery_server_t *server, struct mg_connection* conn, const char* endpoint_id) {
    int status = CIVETWEB_REQUEST_NOT_HANDLED;

    celix_array_list_t* endpoints = NULL;

    if (celixThreadMutex_lock(&server->serverLock) == CELIX_SUCCESS) {
        endpointDiscoveryServer_getEndpoints(server, endpoint_id, &endpoints);
        if (endpoints) {
            status = endpointDiscoveryServer_writeEndpoints(conn, endpoints);
            celix_arrayList_destroy(endpoints);
        }

        celixThreadMutex_unlock(&server->serverLock);
    }

    return status;
}

static int endpointDiscoveryServer_callback(struct mg_connection* conn) {
    int status = CIVETWEB_REQUEST_NOT_HANDLED;

    const struct mg_request_info *request_info = mg_get_request_info(conn);
    if (request_info->request_uri != NULL && strcmp("GET", request_info->request_method) == 0) {
        endpoint_discovery_server_t *server = request_info->user_data;

        const char *uri = request_info->request_uri;
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

#ifndef ANDROID
static celix_status_t endpointDiscoveryServer_getIpAddress(char* interface, char** ip) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != NULL && status != CELIX_SUCCESS; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
                if (interface == NULL) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
                else if (strcmp(ifa->ifa_name, interface) == 0) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
            }
        }

        freeifaddrs(ifaddr);
    }

    return status;
}
#endif
