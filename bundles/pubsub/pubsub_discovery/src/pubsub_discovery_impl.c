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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>
#include <jansson.h>
#include <assert.h>
#include <sys/time.h>

#include "celix_bundle_context.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_threads.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"

#include "pubsub_listeners.h"
#include "pubsub_endpoint.h"
#include "pubsub_discovery_impl.h"

#define L_DEBUG(...) \
    celix_logHelper_log(disc->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(disc->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(disc->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(disc->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

static celix_properties_t* pubsub_discovery_parseEndpoint(pubsub_discovery_t *disc, const char *key, const char *value);
static char* pubsub_discovery_createJsonEndpoint(const celix_properties_t *props);
static void pubsub_discovery_addDiscoveredEndpoint(pubsub_discovery_t *disc, celix_properties_t *endpoint);
static void pubsub_discovery_removeDiscoveredEndpoint(pubsub_discovery_t *disc, const char *uuid);

/* Discovery activator functions */
pubsub_discovery_t* pubsub_discovery_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper) {
    pubsub_discovery_t *disc = calloc(1, sizeof(*disc));
    disc->logHelper = logHelper;
    disc->context = context;
    disc->discoveredEndpoints = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    disc->announcedEndpoints = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    disc->discoveredEndpointsListeners = hashMap_create(NULL, NULL, NULL, NULL);
    celixThreadMutex_create(&disc->discoveredEndpointsListenersMutex, NULL);
    celixThreadMutex_create(&disc->announcedEndpointsMutex, NULL);
    celixThreadMutex_create(&disc->discoveredEndpointsMutex, NULL);

    celixThreadCondition_init(&disc->waitCond, NULL);
    celixThreadMutex_create(&disc->runningMutex, NULL);
    disc->running = true;


    disc->verbose = celix_bundleContext_getPropertyAsBool(context, PUBSUB_ETCD_DISCOVERY_VERBOSE_KEY, PUBSUB_ETCD_DISCOVERY_DEFAULT_VERBOSE);

    const char* etcdIp = celix_bundleContext_getProperty(context, PUBSUB_DISCOVERY_SERVER_IP_KEY, PUBSUB_DISCOVERY_SERVER_IP_DEFAULT);
    long etcdPort = celix_bundleContext_getPropertyAsLong(context, PUBSUB_DISCOVERY_SERVER_PORT_KEY, PUBSUB_DISCOVERY_SERVER_PORT_DEFAULT);
    long ttl = celix_bundleContext_getPropertyAsLong(context, PUBSUB_DISCOVERY_ETCD_TTL_KEY, PUBSUB_DISCOVERY_ETCD_TTL_DEFAULT);

    disc->etcdlib = etcdlib_create(etcdIp, etcdPort, ETCDLIB_NO_CURL_INITIALIZATION);
    disc->ttlForEntries = (int)ttl;
    disc->sleepInsecBetweenTTLRefresh = (int)(((float)ttl)/2.0);
    disc->pubsubPath = celix_bundleContext_getProperty(context, PUBSUB_DISCOVERY_SERVER_PATH_KEY, PUBSUB_DISCOVERY_SERVER_PATH_DEFAULT);
    disc->fwUUID = celix_bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);

    return disc;
}

celix_status_t pubsub_discovery_destroy(pubsub_discovery_t *ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;

    //note cleanup done in stop
    celixThreadMutex_lock(&ps_discovery->discoveredEndpointsMutex);
    hash_map_iterator_t iter = hashMapIterator_construct(ps_discovery->discoveredEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *props = hashMapIterator_nextValue(&iter);
        celix_properties_destroy(props);
    }
    hashMap_destroy(ps_discovery->discoveredEndpoints, false, false);
    celixThreadMutex_unlock(&ps_discovery->discoveredEndpointsMutex);
    celixThreadMutex_destroy(&ps_discovery->discoveredEndpointsMutex);

    celixThreadMutex_lock(&ps_discovery->discoveredEndpointsListenersMutex);
    hashMap_destroy(ps_discovery->discoveredEndpointsListeners, false, false);
    celixThreadMutex_unlock(&ps_discovery->discoveredEndpointsListenersMutex);
    celixThreadMutex_destroy(&ps_discovery->discoveredEndpointsListenersMutex);

    //note cleanup done in stop
    celixThreadMutex_lock(&ps_discovery->announcedEndpointsMutex);
    hashMap_destroy(ps_discovery->announcedEndpoints, false, false);
    celixThreadMutex_unlock(&ps_discovery->announcedEndpointsMutex);
    celixThreadMutex_destroy(&ps_discovery->announcedEndpointsMutex);
    celixThreadCondition_destroy(&ps_discovery->waitCond);

    celixThreadMutex_destroy(&ps_discovery->runningMutex);

    if (ps_discovery->etcdlib != NULL) {
        etcdlib_destroy(ps_discovery->etcdlib);
        ps_discovery->etcdlib = NULL;
    }

    free(ps_discovery);

    return status;
}


static void psd_etcdReadCallback(const char *key, const char *value, void* arg) {
    pubsub_discovery_t *disc = arg;
    celix_properties_t *props = pubsub_discovery_parseEndpoint(disc, key, value);
    if (props != NULL) {
        pubsub_discovery_addDiscoveredEndpoint(disc, props);
    }
}

static void psd_watchSetupConnection(pubsub_discovery_t *disc, bool *connectedPtr, long long *mIndex) {
    bool connected = *connectedPtr;
    if (!connected) {
        if (disc->verbose) {
            printf("[PSD] Reading etcd directory at %s\n", disc->pubsubPath);
        }
        int rc = etcdlib_get_directory(disc->etcdlib, disc->pubsubPath, psd_etcdReadCallback, disc, mIndex);
        if (rc == ETCDLIB_RC_OK) {
            *connectedPtr = true;
        } else {
            *connectedPtr = false;
        }
    }
}

static void psd_watchForChange(pubsub_discovery_t *disc, bool *connectedPtr, long long *mIndex) {
    bool connected = *connectedPtr;
    if (connected) {
        long long watchIndex = *mIndex + 1;

        char *action = NULL;
        char *value = NULL;
        char *readKey = NULL;
        //TODO add interruptable etcdlib_watch -> which returns a handle to interrupt and a can be used for a wait call
        int rc = etcdlib_watch(disc->etcdlib, disc->pubsubPath, watchIndex, &action, NULL, &value, &readKey, mIndex);
        if (rc == ETCDLIB_RC_ERROR) {
            L_ERROR("[PSD] Communicating with etcd. rc is %i, action value is %s\n", rc, action);
            *connectedPtr = false;
        } else if (rc == ETCDLIB_RC_TIMEOUT || action == NULL) {
            //nop
        } else {
            if (strncmp(ETCDLIB_ACTION_CREATE, action, strlen(ETCDLIB_ACTION_CREATE)) == 0 ||
                       strncmp(ETCDLIB_ACTION_SET, action, strlen(ETCDLIB_ACTION_SET)) == 0 ||
                       strncmp(ETCDLIB_ACTION_UPDATE, action, strlen(ETCDLIB_ACTION_UPDATE)) == 0) {
                celix_properties_t *props = pubsub_discovery_parseEndpoint(disc, readKey, value);
                if (props != NULL) {
                    pubsub_discovery_addDiscoveredEndpoint(disc, props);
                }
            } else if (strncmp(ETCDLIB_ACTION_DELETE, action, strlen(ETCDLIB_ACTION_DELETE)) == 0 ||
                       strncmp(ETCDLIB_ACTION_EXPIRE, action, strlen(ETCDLIB_ACTION_EXPIRE)) == 0) {
                char *uuid = strrchr(readKey, '/');
                if (uuid != NULL) {
                    uuid = uuid + 1;
                    pubsub_discovery_removeDiscoveredEndpoint(disc, uuid);
                }
            } else {
                //ETCDLIB_ACTION_GET -> nop
            }

            free(action);
            free(value);
            free(readKey);
        }
    } else {
        if (disc->verbose) {
            printf("[PSD] Skipping etcd watch -> not connected\n");
        }
    }
}

static void psd_cleanupIfDisconnected(pubsub_discovery_t *disc, bool *connectedPtr) {
    bool connected = *connectedPtr;
    if (!connected) {

        celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
        int size = hashMap_size(disc->discoveredEndpoints);
        if (disc->verbose) {
            printf("[PSD] Removing all discovered entries (%i) -> not connected\n", size);
        }

        hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpoints);
        while (hashMapIterator_hasNext(&iter)) {
            celix_properties_t *endpoint = hashMapIterator_nextValue(&iter);

            celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
            hash_map_iterator_t iter2 = hashMapIterator_construct(disc->discoveredEndpointsListeners);
            while (hashMapIterator_hasNext(&iter2)) {
                pubsub_discovered_endpoint_listener_t *listener = hashMapIterator_nextValue(&iter2);
                listener->removeDiscoveredEndpoint(listener->handle, endpoint);
            }
            celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);

            celix_properties_destroy(endpoint);
        }
        hashMap_clear(disc->discoveredEndpoints, false, false);
        celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);
    }
}

void* psd_watch(void *data) {
    pubsub_discovery_t *disc = data;

    long long mIndex = 0L;
    bool connected = false;

    celixThreadMutex_lock(&disc->runningMutex);
    bool running = disc->running;
    celixThreadMutex_unlock(&disc->runningMutex);

    while (running) {
        psd_watchSetupConnection(disc, &connected, &mIndex);
        psd_watchForChange(disc, &connected, &mIndex);
        psd_cleanupIfDisconnected(disc, &connected);

        celixThreadMutex_lock(&disc->runningMutex);
        if (!connected && disc->running) {
            celixThreadCondition_timedwaitRelative(&disc->waitCond, &disc->runningMutex, 5, 0); //if not connected wait a few seconds
        }
        running = disc->running;
        celixThreadMutex_unlock(&disc->runningMutex);
    }

    return NULL;
}

void* psd_refresh(void *data) {
    pubsub_discovery_t *disc = data;

    celixThreadMutex_lock(&disc->runningMutex);
    bool running = disc->running;
    celixThreadMutex_unlock(&disc->runningMutex);

    while (running) {
        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        celixThreadMutex_lock(&disc->announcedEndpointsMutex);
        hash_map_iterator_t iter = hashMapIterator_construct(disc->announcedEndpoints);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_announce_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry->isSet) {
                //only refresh ttl -> no index update -> no watch trigger
                int rc = etcdlib_refresh(disc->etcdlib, entry->key, disc->ttlForEntries);
                if (rc != ETCDLIB_RC_OK) {
                    L_WARN("[PSD] Warning: Cannot refresh etcd key %s\n", entry->key);
                    entry->isSet = false;
                    entry->errorCount += 1;
                } else {
                    entry->refreshCount += 1;
                }
            } else {
                char *str = pubsub_discovery_createJsonEndpoint(entry->properties);
                int rc = etcdlib_set(disc->etcdlib, entry->key, str, disc->ttlForEntries, false);
                if (rc == ETCDLIB_RC_OK) {
                    entry->isSet = true;
                    entry->setCount += 1;
                } else {
                    L_WARN("[PSD] Warning: Cannot set endpoint in etcd for key %s\n", entry->key);
                    entry->errorCount += 1;
                }
                free(str);
            }
        }
        celixThreadMutex_unlock(&disc->announcedEndpointsMutex);

        celixThreadMutex_lock(&disc->runningMutex);
        celixThreadCondition_timedwaitRelative(&disc->waitCond, &disc->runningMutex, disc->sleepInsecBetweenTTLRefresh, 0);
        running = disc->running;
        celixThreadMutex_unlock(&disc->runningMutex);
    }
    return NULL;
}

celix_status_t pubsub_discovery_start(pubsub_discovery_t *ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;

    celixThread_create(&ps_discovery->watchThread, NULL, psd_watch, ps_discovery);
    celixThread_setName(&ps_discovery->watchThread, "PubSub ETCD Watch");
    celixThread_create(&ps_discovery->refreshTTLThread, NULL, psd_refresh, ps_discovery);
    celixThread_setName(&ps_discovery->refreshTTLThread, "PubSub ETCD Refresh TTL");

    return status;
}

celix_status_t pubsub_discovery_stop(pubsub_discovery_t *disc) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&disc->runningMutex);
    disc->running = false;
    celixThreadCondition_broadcast(&disc->waitCond);
    celixThreadMutex_unlock(&disc->runningMutex);

    celixThread_join(disc->watchThread, NULL);
    celixThread_join(disc->refreshTTLThread, NULL);

    celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
    hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *props = hashMapIterator_nextValue(&iter);

        celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
        hash_map_iterator_t iter2 = hashMapIterator_construct(disc->discoveredEndpointsListeners);
        while (hashMapIterator_hasNext(&iter2)) {
            pubsub_discovered_endpoint_listener_t *listener = hashMapIterator_nextValue(&iter2);
            listener->removeDiscoveredEndpoint(listener->handle, props);
        }
        celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);

        celix_properties_destroy(props);
    }
    hashMap_clear(disc->discoveredEndpoints, false, false);
    celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);

    celixThreadMutex_lock(&disc->announcedEndpointsMutex);
    iter = hashMapIterator_construct(disc->announcedEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_announce_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry->isSet) {
            etcdlib_del(disc->etcdlib, entry->key);
        }
        free(entry->key);
        celix_properties_destroy(entry->properties);
        free(entry);
    }
    hashMap_clear(disc->announcedEndpoints, false, false);
    celixThreadMutex_unlock(&disc->announcedEndpointsMutex);

    return status;
}

void pubsub_discovery_discoveredEndpointsListenerAdded(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_discovery_t *disc = handle;
    pubsub_discovered_endpoint_listener_t *listener = svc;

    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
    hashMap_put(disc->discoveredEndpointsListeners, (void*)svcId, listener);
    celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);

    celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
    hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *props = hashMapIterator_nextValue(&iter);
        listener->addDiscoveredEndpoint(listener->handle, props);
    }
    celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);
}

void pubsub_discovery_discoveredEndpointsListenerRemoved(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_discovery_t *disc = handle;

    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
    hashMap_remove(disc->discoveredEndpointsListeners, (void*)svcId);
    celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);
}

celix_status_t pubsub_discovery_announceEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_discovery_t *disc = handle;
    celix_status_t status = CELIX_SUCCESS;

    bool valid = pubsubEndpoint_isValid(endpoint, true, true);
    const char *config = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
    const char *scope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *topic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);

    const char *visibility = celix_properties_get(endpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_VISIBILITY_DEFAULT);

    if (valid && strncmp(visibility, PUBSUB_ENDPOINT_SYSTEM_VISIBILITY, strlen(PUBSUB_ENDPOINT_SYSTEM_VISIBILITY)) == 0) {
        pubsub_announce_entry_t *entry = calloc(1, sizeof(*entry));
        clock_gettime(CLOCK_MONOTONIC, &entry->createTime);
        entry->isSet = false;
        entry->properties = celix_properties_copy(endpoint);
        asprintf(&entry->key, "/pubsub/%s/%s/%s/%s", config, scope == NULL ? PUBSUB_DEFAULT_ENDPOINT_SCOPE : scope, topic, uuid);

        const char *hashKey = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_UUID, NULL);
        celixThreadMutex_lock(&disc->announcedEndpointsMutex);
        hashMap_put(disc->announcedEndpoints, (void*)hashKey, entry);
        celixThreadMutex_unlock(&disc->announcedEndpointsMutex);

        celixThreadMutex_lock(&disc->runningMutex);
        celixThreadCondition_broadcast(&disc->waitCond);
        celixThreadMutex_unlock(&disc->runningMutex);
    } else if (valid) {
        L_DEBUG("[PSD] Ignoring endpoint %s/%s because the visibility is not %s. Configured visibility is %s\n", scope == NULL ? "(null)" : scope, topic, PUBSUB_ENDPOINT_SYSTEM_VISIBILITY, visibility);
    }

    if (!valid) {
        L_ERROR("[PSD] Error cannot announce endpoint. missing some mandatory properties\n");
    }

    return status;
}
celix_status_t pubsub_discovery_revokeEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_discovery_t *disc = handle;
    celix_status_t status = CELIX_SUCCESS;

    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
    pubsub_announce_entry_t *entry = NULL;

    if (uuid != NULL) {
        celixThreadMutex_lock(&disc->announcedEndpointsMutex);
        entry = hashMap_remove(disc->announcedEndpoints, uuid);
        celixThreadMutex_unlock(&disc->announcedEndpointsMutex);
    } else {
        L_WARN("[PSD] Cannot remove announced endpoint. missing endpoint uuid property\n");
    }

    if (entry != NULL) {
        if (entry->isSet) {
            etcdlib_del(disc->etcdlib, entry->key);
        }
        free(entry->key);
        celix_properties_destroy(entry->properties);
        free(entry);
    }

    return status;
}


static void pubsub_discovery_addDiscoveredEndpoint(pubsub_discovery_t *disc, celix_properties_t *endpoint) {
    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
    assert(uuid != NULL);

    celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
    bool exists = hashMap_containsKey(disc->discoveredEndpoints, (void*)uuid);
    if (exists) {
        //if exists -> keep old and free properties
        celix_properties_destroy(endpoint);
    } else {
        hashMap_put(disc->discoveredEndpoints, (void*)uuid, endpoint);
    }
    celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);

    if (!exists) {
        if (disc->verbose) {
            const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, "!Error!");
            const char *admin = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
            const char *ser = celix_properties_get(endpoint, PUBSUB_SERIALIZER_TYPE_KEY, "!Error!");
            const char *prot = celix_properties_get(endpoint, PUBSUB_PROTOCOL_TYPE_KEY, "!Error!");
            L_INFO("[PSD] Adding discovered endpoint %s. type is %s, admin is %s, serializer is %s, protocol is %s.\n",
                   uuid, type, admin, ser, prot);
        }

        celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
        hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpointsListeners);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_discovered_endpoint_listener_t *listener = hashMapIterator_nextValue(&iter);
            listener->addDiscoveredEndpoint(listener->handle, endpoint);
        }
        celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);
    } else {
        //assuming this is the same endpoint -> ignore
    }
}

static void pubsub_discovery_removeDiscoveredEndpoint(pubsub_discovery_t *disc, const char *uuid) {
    celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
    celix_properties_t *endpoint = hashMap_remove(disc->discoveredEndpoints, (void*)uuid);
    celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);

    if (endpoint == NULL) {
        L_WARN("Cannot find endpoint with uuid %s\n", uuid);
        return;
    }

    if (disc->verbose) {
        const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, "!Error!");
        const char *admin = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
        const char *ser = celix_properties_get(endpoint, PUBSUB_SERIALIZER_TYPE_KEY, "!Error!");
        const char *prot = celix_properties_get(endpoint, PUBSUB_PROTOCOL_TYPE_KEY, "!Error!");
        L_INFO("[PSD] Removing discovered endpoint %s. type is %s, admin is %s, serializer is %s, protocol = %s.\n",
               uuid, type, admin, ser, prot);
    }

    if (endpoint != NULL) {
        celixThreadMutex_lock(&disc->discoveredEndpointsListenersMutex);
        hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpointsListeners);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_discovered_endpoint_listener_t *listener = hashMapIterator_nextValue(&iter);
            listener->removeDiscoveredEndpoint(listener->handle, endpoint);
        }
        celixThreadMutex_unlock(&disc->discoveredEndpointsListenersMutex);

        celix_properties_destroy(endpoint);
    } else {
        L_WARN("[PSD] Warning unexpected remove from non existing endpoint (uuid is %s)\n", uuid);
    }
}

celix_properties_t* pubsub_discovery_parseEndpoint(pubsub_discovery_t *disc, const char *key, const char* etcdValue) {
    celix_properties_t *props = celix_properties_create();

    // etcdValue contains the json formatted string
    json_error_t error;
    json_t* jsonRoot = json_loads(etcdValue, JSON_DECODE_ANY, &error);

    if (json_is_object(jsonRoot)) {

        void *iter = json_object_iter(jsonRoot);

        const char *key;
        json_t *value;

        while (iter) {
            key = json_object_iter_key(iter);
            value = json_object_iter_value(iter);
            celix_properties_set(props, key, json_string_value(value));
            iter = json_object_iter_next(jsonRoot, iter);
        }
    }

    if (jsonRoot != NULL) {
        json_decref(jsonRoot);
    }

    bool valid = pubsubEndpoint_isValid(props, true, true);
    if (!valid) {
        L_WARN("[PSD] Retrieved endpoint '%s' is not valid\n", key);
        L_DEBUG("[PSD] Key %s: %s\n", key, etcdValue);
        celix_properties_destroy(props);
        props = NULL;
    }

    return props;
}

static char* pubsub_discovery_createJsonEndpoint(const celix_properties_t *props) {
    //note props is already check for validity (pubsubEndpoint_isValid)

    json_t *jsEndpoint = json_object();
    const char* propKey = NULL;
    PROPERTIES_FOR_EACH((celix_properties_t*)props, propKey) {
        const char* val = celix_properties_get(props, propKey, NULL);
        json_object_set_new(jsEndpoint, propKey, json_string(val));
    }
    char* str = json_dumps(jsEndpoint, JSON_COMPACT);
    json_decref(jsEndpoint);
    return str;
}

bool pubsub_discovery_executeCommand(void *handle, const char * commandLine __attribute__((unused)), FILE *os, FILE *errorStream __attribute__((unused))) {
    pubsub_discovery_t *disc = handle;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    //TODO add support for query (scope / topic)

    fprintf(os, "\n");
    fprintf(os, "Discovery configuration:\n");
    fprintf(os, "   |- etcd host                = %s\n", etcdlib_host(disc->etcdlib));
    fprintf(os, "   |- etcd port                = %i\n", etcdlib_port(disc->etcdlib));
    fprintf(os, "   |- entries ttl              = %i seconds\n", disc->ttlForEntries);
    fprintf(os, "   |- entries refresh time     = %i seconds\n", disc->sleepInsecBetweenTTLRefresh);
    fprintf(os, "   |- pubsub discovery path    = %s\n", disc->pubsubPath);

    fprintf(os, "\n");
    fprintf(os, "Discovered Endpoints:\n");
    celixThreadMutex_lock(&disc->discoveredEndpointsMutex);
    hash_map_iterator_t iter = hashMapIterator_construct(disc->discoveredEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *ep = hashMapIterator_nextValue(&iter);
        const char *uuid = celix_properties_get(ep, PUBSUB_ENDPOINT_UUID, "!Error!");
        const char *scope = celix_properties_get(ep, PUBSUB_ENDPOINT_TOPIC_SCOPE, "!Error!");
        const char *topic = celix_properties_get(ep, PUBSUB_ENDPOINT_TOPIC_NAME, "!Error!");
        const char *adminType = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
        const char *serType = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
        const char *protType = celix_properties_get(ep, PUBSUB_ENDPOINT_PROTOCOL, "!Error!");
        const char *type = celix_properties_get(ep, PUBSUB_ENDPOINT_TYPE, "!Error!");
        fprintf(os, "Endpoint %s:\n", uuid);
        fprintf(os, "   |- type          = %s\n", type);
        fprintf(os, "   |- scope         = %s\n", scope);
        fprintf(os, "   |- topic         = %s\n", topic);
        fprintf(os, "   |- admin type    = %s\n", adminType);
        fprintf(os, "   |- serializer    = %s\n", serType);
        fprintf(os, "   |- protocol      = %s\n", protType);
    }
    celixThreadMutex_unlock(&disc->discoveredEndpointsMutex);

    fprintf(os, "\n");
    fprintf(os, "Announced Endpoints:\n");
    celixThreadMutex_lock(&disc->announcedEndpointsMutex);
    iter = hashMapIterator_construct(disc->announcedEndpoints);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_announce_entry_t *entry = hashMapIterator_nextValue(&iter);
        const char *uuid = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_UUID, "!Error!");
        const char *scope = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE, "!Error!");
        const char *topic = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_TOPIC_NAME, "!Error!");
        const char *adminType = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
        const char *serType = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
        const char *protType = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_PROTOCOL, "!Error!");
        const char *type = celix_properties_get(entry->properties, PUBSUB_ENDPOINT_TYPE, "!Error!");
        int age = (int)(now.tv_sec - entry->createTime.tv_sec);
        fprintf(os, "Endpoint %s:\n", uuid);
        fprintf(os, "   |- type          = %s\n", type);
        fprintf(os, "   |- scope         = %s\n", scope);
        fprintf(os, "   |- topic         = %s\n", topic);
        fprintf(os, "   |- admin type    = %s\n", adminType);
        fprintf(os, "   |- serializer    = %s\n", serType);
        fprintf(os, "   |- protocol      = %s\n", protType);
        fprintf(os, "   |- age           = %ds\n", age);
        fprintf(os, "   |- is set        = %s\n", entry->isSet ? "true" : "false");
        if (disc->verbose) {
            fprintf(os, "   |- set count     = %d\n", entry->setCount);
            fprintf(os, "   |- refresh count = %d\n", entry->refreshCount);
            fprintf(os, "   |- error count   = %d\n", entry->errorCount);
        }
    }
    celixThreadMutex_unlock(&disc->announcedEndpointsMutex);

    return true;
}
