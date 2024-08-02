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
#include "celix_earpm_broker_discovery.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <uuid/uuid.h>

#include "celix_stdlib_cleanup.h"
#include "celix_stdio_cleanup.h"
#include "celix_utils.h"
#include "celix_array_list.h"
#include "celix_long_hash_map.h"
#include "celix_threads.h"
#include "celix_log_helper.h"
#include "celix_constants.h"
#include "endpoint_listener.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_earpm_constants.h"

#define CELIX_EARPM_LOAD_PROFILE_INTERVAL 2 //s
#define CELIX_EARPM_LOAD_PROFILE_TRIES_MAX (600/CELIX_EARPM_LOAD_PROFILE_INTERVAL) //10 minutes


typedef struct celix_earpm_broker_listener {
    char* host;
    uint16_t port;
    char* bindInterface;
}celix_earpm_broker_listener_t;

typedef struct celix_earpm_endpoint_listener_entry {
    endpoint_listener_t* listener;
    const celix_properties_t*  properties;
    long serviceId;
}celix_earpm_endpoint_listener_entry_t;

struct celix_earpm_broker_discovery {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* fwUUID;
    const char* brokerProfilePath;
    long profileScheduledEventId;
    int loadProfileTries;
    celix_thread_mutex_t mutex;//protects below
    celix_long_hash_map_t* endpointListeners;//key:long, value:celix_earpm_endpoint_listener_entry_t*
    celix_array_list_t* brokerEndpoints;//element:endpoint_description_t*
};

static void celix_earpmDiscovery_onProfileScheduledEvent(void* data);

celix_earpm_broker_discovery_t* celix_earpmDiscovery_create(celix_bundle_context_t* ctx) {
    assert(ctx != NULL);
    celix_autoptr(celix_log_helper_t) logHelper = celix_logHelper_create(ctx, "CELIX_EARPM_DISCOVERY");
    if (logHelper == NULL) {
        return NULL;
    }
    celix_autofree celix_earpm_broker_discovery_t* discovery = calloc(1, sizeof(*discovery));
    if (discovery == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for celix earpm broker discovery");
        return NULL;
    }
    discovery->fwUUID = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (discovery->fwUUID == NULL) {
        celix_logHelper_error(logHelper, "Failed to get framework uuid from context");
        return NULL;
    }
    discovery->ctx = ctx;
    discovery->logHelper = logHelper;
    discovery->profileScheduledEventId = -1;
    discovery->loadProfileTries = 0;
    discovery->brokerProfilePath = celix_bundleContext_getProperty(ctx, CELIX_EARPM_BROKER_PROFILE, CELIX_EARPM_BROKER_PROFILE_DEFAULT);
    discovery->brokerEndpoints = NULL;
    celix_status_t status = celixThreadMutex_create(&discovery->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create mutex for celix earpm broker discovery");
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &discovery->mutex;
    celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
    opts.simpleRemovedCallback = free;
    discovery->endpointListeners = celix_longHashMap_createWithOptions(&opts);
    if (discovery->endpointListeners == NULL) {
        celix_logHelper_error(logHelper, "Failed to create endpoint listeners map for celix earpm broker discovery");
        return NULL;
    }

    celix_steal_ptr(mutex);
    celix_steal_ptr(logHelper);

    return celix_steal_ptr(discovery);
}

void celix_earpmDiscovery_destroy(celix_earpm_broker_discovery_t* discovery) {
    assert(discovery != NULL);
    celix_longHashMap_destroy(discovery->endpointListeners);
    celix_arrayList_destroy(discovery->brokerEndpoints);
    celixThreadMutex_destroy(&discovery->mutex);
    celix_logHelper_destroy(discovery->logHelper);
    free(discovery);
}

celix_status_t celix_earpmDiscovery_start(celix_earpm_broker_discovery_t* discovery) {
    assert(discovery != NULL);
    celix_auto(celix_mutex_lock_guard_t) mutexLockGuard = celixMutexLockGuard_init(&discovery->mutex);

    if (discovery->brokerEndpoints == NULL && discovery->profileScheduledEventId < 0) {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "MqttProfileCheckEvent";
        opts.intervalInSeconds = CELIX_EARPM_LOAD_PROFILE_INTERVAL;
        opts.callbackData = discovery;
        opts.callback =celix_earpmDiscovery_onProfileScheduledEvent;
        discovery->profileScheduledEventId = celix_bundleContext_scheduleEvent(discovery->ctx, &opts);
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_earpmDiscovery_stop(celix_earpm_broker_discovery_t* discovery) {
    assert(discovery != NULL);
    (void)celix_bundleContext_removeScheduledEvent(discovery->ctx, __atomic_load_n(&discovery->profileScheduledEventId, __ATOMIC_RELAXED));
    return CELIX_SUCCESS;
}

static void celix_earpmDiscovery_notifyEndpointsToListener(celix_earpm_broker_discovery_t* discovery, celix_earpm_endpoint_listener_entry_t* listenerEntry, bool added) {
    const char* listenerScope = celix_properties_get(listenerEntry->properties, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, NULL);
    celix_filter_t* filter = listenerScope == NULL ? NULL : celix_filter_create(listenerScope);
    celix_status_t (*process)(void *handle, endpoint_description_t *endpoint, char *matchedFilter) = listenerEntry->listener->endpointAdded;
    if (!added) {
        process = listenerEntry->listener->endpointRemoved;
    }
    int size = celix_arrayList_size(discovery->brokerEndpoints);
    for (int i = 0; i < size; ++i) {
        endpoint_description_t *endpoint = celix_arrayList_get(discovery->brokerEndpoints, i);
        if (celix_filter_match(filter, endpoint->properties)) {
            //TODO 判断是否支持动态IP
            celix_status_t status = process(listenerEntry->listener->handle, endpoint, (char*) listenerScope);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(discovery->logHelper, "Failed to %s endpoint to listener(%ld).", added ? "add" : "remove", listenerEntry->serviceId);
            }
        }
    }
    return;
}

celix_status_t celix_earpmDiscovery_addEndpointListener(void* handle, void* service, const celix_properties_t* properties) {
    celix_earpm_broker_discovery_t* discovery = handle;
    assert(discovery != NULL);
    long serviceId = celix_properties_getAsLong(properties, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId < 0) {
        celix_logHelper_error(discovery->logHelper, "Failed to get service id from properties");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree celix_earpm_endpoint_listener_entry_t* svcEntry = calloc(1, sizeof(*svcEntry));
    if (svcEntry == NULL) {
        celix_logHelper_error(discovery->logHelper, "Failed to allocate memory for endpoint listener entry");
        return CELIX_ENOMEM;
    }
    svcEntry->serviceId = serviceId;
    svcEntry->listener = service;
    svcEntry->properties = properties;

    celix_auto(celix_mutex_lock_guard_t) mutexLockGuard = celixMutexLockGuard_init(&discovery->mutex);

    celix_status_t status = celix_longHashMap_put(discovery->endpointListeners, serviceId, svcEntry);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(discovery->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(discovery->logHelper, "Failed to add endpoint listener entry to map");
        return status;
    }

    celix_earpmDiscovery_notifyEndpointsToListener(discovery, svcEntry, true);

    return CELIX_SUCCESS;
}

celix_status_t celix_earpmDiscovery_removeEndpointListener(void* handle, void* service, const celix_properties_t* properties) {
    celix_earpm_broker_discovery_t* discovery = handle;
    assert(discovery != NULL);
    long serviceId = celix_properties_getAsLong(properties, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId < 0) {
        celix_logHelper_error(discovery->logHelper, "Failed to get service id from properties");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_auto(celix_mutex_lock_guard_t) mutexLockGuard = celixMutexLockGuard_init(&discovery->mutex);

    celix_earpm_endpoint_listener_entry_t* svcEntry = celix_longHashMap_get(discovery->endpointListeners, serviceId);
    if (svcEntry == NULL) {
        return CELIX_SUCCESS;
    }

    celix_earpmDiscovery_notifyEndpointsToListener(discovery, svcEntry, false);

    return CELIX_SUCCESS;
}

static celix_earpm_broker_listener_t* celix_earpmDiscovery_brokerListenerCreate(const char* host, uint16_t port) {
    celix_autofree celix_earpm_broker_listener_t* listener = calloc(1, sizeof(*listener));
    if (listener == NULL) {
        return NULL;
    }
    listener->port = port;
    listener->bindInterface = NULL;
    listener->host = NULL;
    if (host) {
        listener->host = celix_utils_strdup(host);
        if (listener->host == NULL) {
            return NULL;
        }
    }
    return celix_steal_ptr(listener);
}

static void celix_earpmDiscovery_brokerListenerDestroy(celix_earpm_broker_listener_t* listener) {
    free(listener->host);
    free(listener->bindInterface);
    free(listener);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_broker_listener_t, celix_earpmDiscovery_brokerListenerDestroy)

static void celix_earpmDiscovery_stripLine(char* line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len = strlen(line);
    }
    return ;
}

static celix_array_list_t* celix_earpmDiscovery_parseBrokerProfile(celix_earpm_broker_discovery_t* discovery, FILE* file) {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER;
    opts.simpleRemovedCallback = (void *)celix_earpmDiscovery_brokerListenerDestroy;
    celix_autoptr(celix_array_list_t) brokerListeners = celix_arrayList_createWithOptions(&opts);
    if (brokerListeners == NULL) {
        celix_logHelper_error(discovery->logHelper, "Failed to create broker listeners list.");
        return NULL;
    }
    celix_earpm_broker_listener_t* curListener = NULL;
    char line[512] = {0};
    char* token = NULL;
    char* save = NULL;
    while (fgets(line, sizeof(line), file) != NULL) {
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r'){
            continue;
        }
        celix_earpmDiscovery_stripLine(line);

        token = strtok_r(line, " ", &save);
        if (token == NULL) {
            continue;
        }
        if (celix_utils_stringEquals(token, "listener")) {
            curListener = NULL;
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                celix_logHelper_error(discovery->logHelper, "Invalid listener line in broker profile file %s.", discovery->brokerProfilePath);
                continue;
            }
            char* portEnd = NULL;
            long port = strtol(token, &portEnd, 10);
            if (portEnd == NULL || *portEnd != '\0' || portEnd == token || port < 0 || port > UINT16_MAX) {
                celix_logHelper_error(discovery->logHelper, "Invalid port in listener line(%s) in broker profile file %s.", token, discovery->brokerProfilePath);
                continue;
            }
            char* host = strtok_r(NULL, " ", &save);
            if (port == 0 && host == NULL) {
                celix_logHelper_error(discovery->logHelper, "Invalid unix socket listener line in broker profile file %s.", discovery->brokerProfilePath);
                continue;
            }
            celix_autoptr(celix_earpm_broker_listener_t) listener = celix_earpmDiscovery_brokerListenerCreate(host,(uint16_t) port);
            if (listener == NULL) {
                celix_logHelper_error(discovery->logHelper, "Failed to create broker listener for port %ld.", port);
                continue;
            }
            celix_status_t status = celix_arrayList_add(brokerListeners, listener);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(discovery->logHelper, "Failed to add broker listener for port %ld.", port);
                continue;
            }
            curListener = celix_steal_ptr(listener);
        } else if (celix_utils_stringEquals(token, "bind_interface") && curListener != NULL) {
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                continue;
            }
            curListener->bindInterface = celix_utils_strdup(token);
            if (curListener->bindInterface == NULL) {
                celix_logHelper_error(discovery->logHelper, "Failed to dup bind interface %s.", token);
                continue;
            }
        }
    }
    return celix_steal_ptr(brokerListeners);
}

static celix_array_list_t* celix_earpmDiscovery_createBrokerEndpoints(celix_earpm_broker_discovery_t* discovery, celix_array_list_t* brokerListeners) {
    celix_array_list_create_options_t options = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    options.elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER;
    options.simpleRemovedCallback = (void *)endpointDescription_destroy;
    celix_autoptr(celix_array_list_t) brokerEndpoints = celix_arrayList_createWithOptions(&options);
    if (brokerEndpoints == NULL) {
        celix_logHelper_error(discovery->logHelper, "Failed to create broker endpoints list.");
        return NULL;
    }
    int size = celix_arrayList_size(brokerListeners);
    for (int i = 0; i < size; ++i) {
        celix_earpm_broker_listener_t* listener = celix_arrayList_get(brokerListeners, i);
        const char* host = listener->host != NULL ? listener->host : "";
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        if (props == NULL) {
            celix_logHelper_logTssErrors(discovery->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(discovery->logHelper, "Failed to create properties for mqtt broker listener %s:%d", host, listener->port);
            return NULL;
        }
        uuid_t uid;
        uuid_generate(uid);
        char endpointUUID[37];
        uuid_unparse_lower(uid, endpointUUID);
        celix_status_t status = celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, discovery->fwUUID);
        CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "celix_mqtt_broker_info"));
        CELIX_DO_IF(status, status = celix_properties_setLong(props, CELIX_RSA_ENDPOINT_SERVICE_ID, INT32_MAX));
        CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, endpointUUID));
        CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED, "true"));
        CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "celix.server.mqtt"));
        if (listener->port != 0) {
            CELIX_DO_IF(status, status = celix_properties_setLong(props, CELIX_RSA_PORT, listener->port));
            CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_RSA_IP_ADDRESSES, host));
            const char* bindInterface = listener->bindInterface != NULL ? listener->bindInterface : "all";
            CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, bindInterface));
        } else {
            CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_EARPM_MQTT_BROKER_UNIX_SOCKET_PATH_KEY, host));
        }
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(discovery->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(discovery->logHelper, "Failed to set properties for mqtt broker listener %s:%d", host, listener->port);
            return NULL;
        }

        celix_autoptr(endpoint_description_t) endpoint = NULL;
        status = endpointDescription_create(props, &endpoint);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(discovery->logHelper, "Failed to create endpoint for mqtt broker listener %s:%d", host, listener->port);
            return NULL;
        }
        celix_steal_ptr(props);
        status = celix_arrayList_add(brokerEndpoints, endpoint);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(discovery->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(discovery->logHelper, "Failed to add endpoint for mqtt broker listener %s:%d", host, listener->port);
            return NULL;
        }
        celix_steal_ptr(endpoint);
    }
    return celix_steal_ptr(brokerEndpoints);
}

static bool celix_earpmDiscovery_loadBrokerProfile(celix_earpm_broker_discovery_t* discovery) {
    celix_autoptr(FILE) file = fopen(discovery->brokerProfilePath, "r");
    if (file == NULL) {
        celix_log_level_e logLevel = errno != ENOENT ? CELIX_LOG_LEVEL_ERROR : CELIX_LOG_LEVEL_DEBUG;
        celix_logHelper_log(discovery->logHelper, logLevel, "Failed to open broker profile file %s. %d", discovery->brokerProfilePath, errno);
        return errno != ENOENT;
    }

    celix_autoptr(celix_array_list_t) brokerListeners = celix_earpmDiscovery_parseBrokerProfile(discovery, file);
    if (brokerListeners == NULL) {
        celix_logHelper_error(discovery->logHelper, "Failed to parse broker profile.");
        return false;
    }

    celix_array_list_t* brokerEndpoints = celix_earpmDiscovery_createBrokerEndpoints(discovery, brokerListeners);
    if (brokerEndpoints == NULL) {
        celix_logHelper_error(discovery->logHelper, "Failed to create broker endpoints.");
        return false;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexLockGuard = celixMutexLockGuard_init(&discovery->mutex);
    discovery->brokerEndpoints = brokerEndpoints;

    return true;
}

static void celix_earpmDiscovery_onProfileScheduledEvent(void* data) {
    celix_earpm_broker_discovery_t* discovery = data;
    assert(discovery != NULL);
    bool loaded = celix_earpmDiscovery_loadBrokerProfile(discovery);
    if (loaded) {
        celix_logHelper_info(discovery->logHelper, "Loaded broker profile from %s", discovery->brokerProfilePath);
        celix_auto(celix_mutex_lock_guard_t) mutexLockGuard = celixMutexLockGuard_init(&discovery->mutex);
        CELIX_LONG_HASH_MAP_ITERATE(discovery->endpointListeners, iter) {
            celix_earpm_endpoint_listener_entry_t* listenerEntry = iter.value.ptrValue;
            celix_earpmDiscovery_notifyEndpointsToListener(discovery, listenerEntry, true);
        }
    }
    if (loaded || ++discovery->loadProfileTries >= CELIX_EARPM_LOAD_PROFILE_TRIES_MAX) {
        celix_bundleContext_removeScheduledEventAsync(discovery->ctx, discovery->profileScheduledEventId);
        __atomic_store_n(&discovery->profileScheduledEventId, -1, __ATOMIC_RELEASE);
    }
    return;
}

