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
#include "celix_earpm_client.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <mqtt_protocol.h>
#include <sys/queue.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "celix_compiler.h"
#include "celix_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include "celix_ref.h"
#include "celix_array_list.h"
#include "celix_long_hash_map.h"
#include "celix_string_hash_map.h"
#include "celix_utils.h"
#include "celix_constants.h"
#include "remote_constants.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_broker_discovery.h"

#define CELIX_EARPM_CLIENT_KEEP_ALIVE 60 //seconds
#define CELIX_EARPM_CLIENT_RECONNECT_DELAY_MAX 30 //seconds

typedef struct celix_earpm_client_broker_info {
    struct celix_ref ref;
    celix_array_list_t* addresses;
    int port;
    char* bindInterface;
    int family;
}celix_earpm_client_broker_info_t;

typedef struct celix_earpm_client_msg {
    struct celix_ref ref;
    TAILQ_ENTRY(celix_earpm_client_msg) entry;
    char* topic;
    char* payload;
    size_t payloadSize;
    celix_earpm_qos_e qos;
    celix_earpm_client_message_priority_e pri;
    bool sync;
    mosquitto_property* mqttProps;
    long seqNr;
    int mqttMid;
    celix_status_t error;
    struct celix_earpm_client_msg_pool* msgPool;
}celix_earpm_client_msg_t;

typedef TAILQ_HEAD(celix_earpm_client_message_list, celix_earpm_client_msg) celix_earpm_client_message_list_t;

typedef struct celix_earpm_client_msg_pool {
    size_t cap;
    size_t usedSize;
    celix_earpm_client_message_list_t freeMsgList;
    celix_earpm_client_msg_t* msgBlocks;
}celix_earpm_client_msg_pool_t;

struct celix_earpm_client {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    celix_earpm_client_receive_msg_fp receiveMsgCallback;
    celix_earpm_client_connected_fp connectedCallback;
    void* callbackHandle;
    size_t parallelMsgCap;
    struct mosquitto* mosq;
    mosquitto_property* connProps;
    mosquitto_property* disconnectProps;
    celix_thread_t mosqThread;
    celix_thread_mutex_t mutex;// protects belows
    celix_string_hash_map_t* brokerInfoMap;//key = endpoint uuid, value = celix_broker_info_t*
    celix_thread_cond_t brokerInfoChangedOrExiting;
    char* currentBrokerEndpointUUID;
    celix_thread_cond_t msgStatusChanged;
    celix_earpm_client_msg_pool_t freeMsgPool;
    celix_earpm_client_message_list_t waitingMessages;
    celix_long_hash_map_t* publishingMessages;//key = mid, value = celix_earpmc_message_t*
    celix_long_hash_map_t* publishedMessages;//key = seqNr, value = celix_earpmc_message_t*
    celix_string_hash_map_t* subscriptions;//key = topic, value = qos
    bool connected;
    bool running;
};

static celix_status_t celix_earpmClient_configMosq(mosquitto* mosq, celix_log_helper_t* logHelper, const char* sessionEndTopic, const mosquitto_property* sessionEndProps);
 static void celix_earpmClient_messageRelease(celix_earpm_client_msg_t* msg);
static void celix_earpmClient_brokerInfoRelease(celix_earpm_client_broker_info_t* info);
static void celix_earpmClient_connectCallback(struct mosquitto* mosq, void* handle, int rc, int flag, const mosquitto_property* props);
static void celix_earpmClient_disconnectCallback(struct mosquitto* mosq, void* handle, int rc, const mosquitto_property* props);
static void celix_earpmClient_messageCallback(struct mosquitto* mosq, void* handle, const struct mosquitto_message* message, const mosquitto_property* props);
static void celix_earpmClient_publishCallback(struct mosquitto* mosq, void* handle, int mid, int reasonCode, const mosquitto_property* props);
static void* celix_earpmClient_mosqThread(void* data);


static celix_status_t celix_earpmClient_msgPoolInit(celix_earpm_client_msg_pool_t* pool, size_t cap) {
    pool->cap = cap;
    pool->usedSize = 0;
    celix_earpm_client_msg_t* msgBlocks = pool->msgBlocks = (celix_earpm_client_msg_t*)calloc(cap, sizeof(celix_earpm_client_msg_t));
    if (msgBlocks == NULL) {
        return CELIX_ENOMEM;
    }
    TAILQ_INIT(&pool->freeMsgList);
    for (size_t i = 0; i < cap; ++i) {
        TAILQ_INSERT_TAIL(&pool->freeMsgList, &msgBlocks[i], entry);
    }
    return CELIX_SUCCESS;
}

static void celix_earpmClient_msgPoolDeInit(celix_earpm_client_msg_pool_t* pool) {
    assert(pool->usedSize == 0);
    free(pool->msgBlocks);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_msg_pool_t, celix_earpmClient_msgPoolDeInit)

static celix_earpm_client_msg_t* celix_earpmClient_msgPoolAlloc(celix_earpm_client_msg_pool_t* pool) {
    celix_earpm_client_msg_t* msg = TAILQ_FIRST(&pool->freeMsgList);
    if (msg != NULL) {
        TAILQ_REMOVE(&pool->freeMsgList, msg, entry);
        pool->usedSize ++;
    }
    return msg;
}

static void celix_earpmClient_msgPoolFree(celix_earpm_client_msg_pool_t* pool, celix_earpm_client_msg_t* msg) {
    TAILQ_INSERT_TAIL(&pool->freeMsgList, msg, entry);
    pool->usedSize --;
    return;
}

celix_earpm_client_t* celix_earpmClient_create(celix_earpm_client_create_options_t* options) {
    assert(options != NULL);
    assert(options->ctx != NULL);
    assert(options->logHelper != NULL);
    assert(options->sessionEndTopic != NULL);
    assert(options->sessionEndProps != NULL);
    assert(options->receiveMsgCallback != NULL);
    assert(options->connectedCallback != NULL);

    celix_log_helper_t* logHelper = options->logHelper;
    const char* fwUUID = celix_bundleContext_getProperty(options->ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (fwUUID == NULL) {
        celix_logHelper_error(logHelper, "Failed to get framework UUID.");
        return NULL;
    }
    long msgQueueCap = celix_bundleContext_getPropertyAsLong(options->ctx, CELIX_EARPM_MSG_QUEUE_CAPACITY, CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT);
    if (msgQueueCap <= 0 || msgQueueCap > CELIX_EARPM_MSG_QUEUE_MAX_SIZE) {
        celix_logHelper_error(logHelper, "Invalid message queue capacity %ld.", msgQueueCap);
        return NULL;
    }
    long parallelMsgCap = celix_bundleContext_getPropertyAsLong(options->ctx, CELIX_EARPM_PARALLEL_MSG_CAPACITY, CELIX_EARPM_PARALLEL_MSG_CAPACITY_DEFAULT);
    if (parallelMsgCap <= 0 || parallelMsgCap > msgQueueCap) {
        celix_logHelper_error(logHelper, "Invalid parallel message capacity %ld.", parallelMsgCap);
        return NULL;
    }

    celix_autofree celix_earpm_client_t *client = calloc(1, sizeof(*client));
    if (client == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for earpm client.");
        return NULL;
    }
    client->ctx = options->ctx;
    client->logHelper = logHelper;
    client->parallelMsgCap = (size_t)parallelMsgCap;
    client->currentBrokerEndpointUUID = NULL;
    client->connected = false;
    client->receiveMsgCallback = options->receiveMsgCallback;
    client->connectedCallback = options->connectedCallback;
    client->callbackHandle = options->callbackHandle;
    celix_status_t status = celixThreadMutex_create(&client->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create mqtt client mutex.");
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &client->mutex;

    status = celixThreadCondition_init(&client->msgStatusChanged, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create mqtt client condition. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) msgStatusChanged = &client->msgStatusChanged;

    status = celix_earpmClient_msgPoolInit(&client->freeMsgPool, msgQueueCap);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create message pool. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_earpm_client_msg_pool_t) freeMsgPool = &client->freeMsgPool;

    TAILQ_INIT(&client->waitingMessages);

    celix_autoptr(celix_long_hash_map_t) publishingMessages = NULL;
    {
        celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *) celix_earpmClient_messageRelease;
        publishingMessages = client->publishingMessages = celix_longHashMap_createWithOptions(&opts);
        if (publishingMessages == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(client->logHelper, "Failed to create publishing message map.");
            return NULL;
        }
    }

    celix_autoptr(celix_long_hash_map_t) publishedMessages = NULL;
    {
        celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *) celix_earpmClient_messageRelease;
        publishedMessages = client->publishedMessages = celix_longHashMap_createWithOptions(&opts);
        if (publishedMessages == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(client->logHelper, "Failed to create published message map.");
            return NULL;
        }
    }

    celix_autoptr(celix_string_hash_map_t) subscriptions = client->subscriptions = celix_stringHashMap_create();
    if (subscriptions == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(client->logHelper, "Failed to create subscriptions map.");
        return NULL;
    }

    celix_autoptr(celix_string_hash_map_t) brokerInfoMap = NULL;
    {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *) celix_earpmClient_brokerInfoRelease;
        brokerInfoMap = client->brokerInfoMap = celix_stringHashMap_createWithOptions(&opts);
        if (brokerInfoMap == NULL) {
            celix_logHelper_error(client->logHelper, "Failed to create broker info map.");
            return NULL;
        }
    }

    status = celixThreadCondition_init(&client->brokerInfoChangedOrExiting, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create broker info changed condition. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) brokerInfoChanged = &client->brokerInfoChangedOrExiting;

    celix_autoptr(mosquitto_property) connProps = NULL;
    int rc = mosquitto_property_add_int32(&connProps, MQTT_PROP_SESSION_EXPIRY_INTERVAL, CELIX_EARPM_SESSION_EXPIRY_INTERVAL);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to set mqtt session expiry interval. %d.", rc);
        return NULL;
    }
    client->connProps = connProps;

    celix_autoptr(mosquitto_property) disconnectProps = NULL;
    rc = mosquitto_property_add_int32(&disconnectProps, MQTT_PROP_SESSION_EXPIRY_INTERVAL, 0);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create disconnect properties. %d.", rc);
        return NULL;
    }
    client->disconnectProps = disconnectProps;

    celix_autoptr(mosquitto) mosq = client->mosq = mosquitto_new(fwUUID, false, client);
    if (client->mosq == NULL) {
        celix_logHelper_error(client->logHelper, "Failed to create mosquitto instance.");
        return NULL;
    }
    status = celix_earpmClient_configMosq(client->mosq, client->logHelper, options->sessionEndTopic, options->sessionEndProps);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to configure mosquitto instance.");
        return NULL;
    }
    client->running = true;
    status = celixThread_create(&client->mosqThread, NULL, celix_earpmClient_mosqThread, client);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to create mosq thread. %d.", status);
        return NULL;
    }
    celixThread_setName(&client->mosqThread, "earpm_mosq_th");

    celix_steal_ptr(mosq);
    celix_steal_ptr(disconnectProps);
    celix_steal_ptr(connProps);
    celix_steal_ptr(brokerInfoChanged);
    celix_steal_ptr(brokerInfoMap);
    celix_steal_ptr(subscriptions);
    celix_steal_ptr(publishedMessages);
    celix_steal_ptr(publishingMessages);
    celix_steal_ptr(freeMsgPool);
    celix_steal_ptr(msgStatusChanged);
    celix_steal_ptr(mutex);

    return celix_steal_ptr(client);
}

void celix_earpmClient_destroy(celix_earpm_client_t* client) {
    assert(client != NULL);
    celixThreadMutex_lock(&client->mutex);
    client->running = false;
    celixThreadMutex_unlock(&client->mutex);
    int rc = mosquitto_disconnect_v5(client->mosq, MQTT_RC_DISCONNECT_WITH_WILL_MSG, client->disconnectProps);
    if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
        celix_logHelper_error(client->logHelper, "Failed to disconnect mosquitto, will try to force destroy. %d.", rc);
    }
    celixThreadCondition_signal(&client->brokerInfoChangedOrExiting);
    celixThread_join(client->mosqThread, NULL);
    mosquitto_destroy(client->mosq);
    mosquitto_property_free_all(&client->disconnectProps);
    mosquitto_property_free_all(&client->connProps);
    celixThreadCondition_destroy(&client->brokerInfoChangedOrExiting);
    celix_stringHashMap_destroy(client->brokerInfoMap);
    celix_stringHashMap_destroy(client->subscriptions);
    celix_longHashMap_destroy(client->publishedMessages);
    celix_longHashMap_destroy(client->publishingMessages);
    celix_earpm_client_msg_t* msg = TAILQ_FIRST(&client->waitingMessages);
    while (msg != NULL) {
        TAILQ_REMOVE(&client->waitingMessages, msg, entry);
        celix_earpmClient_messageRelease(msg);
        msg = TAILQ_FIRST(&client->waitingMessages);
    }
    celix_earpmClient_msgPoolDeInit(&client->freeMsgPool);
    celixThreadCondition_destroy(&client->msgStatusChanged);
    celixThreadMutex_destroy(&client->mutex);
    free(client->currentBrokerEndpointUUID);
    free(client);
    return;
}

static celix_status_t celix_earpmClient_configMosq(mosquitto *mosq, celix_log_helper_t* logHelper, const char* sessionEndTopic, const mosquitto_property* sessionEndProps) {
    assert(mosq != NULL);
    int rc = mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to set mqtt protocol version.");
        return CELIX_ILLEGAL_STATE;
    }
    rc = mosquitto_int_option(mosq, MOSQ_OPT_TCP_NODELAY, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to set mqtt tcp no delay.");
        return CELIX_ILLEGAL_STATE;
    }
    celix_autoptr(mosquitto_property) sessionEndPropsCopy = NULL;
    rc = mosquitto_property_copy_all(&sessionEndPropsCopy, sessionEndProps);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to copy session end properties. %d.", rc);
        return rc == MOSQ_ERR_NOMEM ? ENOMEM : CELIX_ILLEGAL_ARGUMENT;
    }
    //It will ensure that Will Message is sent when the session ends by setting the Will Delay Interval to be longer than
    // the Session Expiry Interval. Because the Server delays publishing the Client’s Will Message until the Will Delay Interval
    // has passed or the Session ends, whichever happens first.
    if (mosquitto_property_add_int32(&sessionEndPropsCopy, MQTT_PROP_WILL_DELAY_INTERVAL, CELIX_EARPM_SESSION_EXPIRY_INTERVAL * 2) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to add will delay interval property for will message.");
        return CELIX_ILLEGAL_STATE;
    }
    rc = mosquitto_will_set_v5(mosq, sessionEndTopic, 0, NULL, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, sessionEndPropsCopy);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to set mqtt will. %d.", rc);
        return CELIX_ILLEGAL_STATE;
    }
    celix_steal_ptr(sessionEndPropsCopy);

    mosquitto_connect_v5_callback_set(mosq, celix_earpmClient_connectCallback);
    mosquitto_disconnect_v5_callback_set(mosq, celix_earpmClient_disconnectCallback);
    mosquitto_message_v5_callback_set(mosq, celix_earpmClient_messageCallback);
    mosquitto_publish_v5_callback_set(mosq, celix_earpmClient_publishCallback);
    mosquitto_threaded_set(mosq, true);
    return CELIX_SUCCESS;
}

static bool celix_earpmClient_matchIpFamily(const char* address, int family) {
    if (family == AF_UNSPEC) {
        return true;
    }
    char buf[sizeof(struct in6_addr)];
    bool isIPv4 = (inet_pton(AF_INET, address, buf) != 0);
    bool isIPv6 = isIPv4 ? false : (inet_pton(AF_INET6, address, buf) != 0);
    if (!isIPv4 && !isIPv6) {
        return true;//The address is host name, let the mqtt library to resolve it.
    }
    if (family == AF_INET && isIPv4) {
        return true;
    }
    if (family == AF_INET6 && isIPv6) {
        return true;
    }
    return false;
}

static celix_earpm_client_broker_info_t* celix_earpmClient_brokerInfoCreate(const char* addresses, int port, const char* bindInterface, int family) {
    celix_autofree celix_earpm_client_broker_info_t* info = calloc(1, sizeof(*info));
    if (info == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    celix_ref_init(&info->ref);
    info->port = port;
    info->family = family;
    if (bindInterface != NULL) {
        info->bindInterface = celix_utils_strdup(bindInterface);
        if (info->bindInterface == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    } else {
        celix_autofree char* addressesCopy = celix_utils_strdup(addresses);
        if (addressesCopy == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        celix_autoptr(celix_array_list_t) addressList = info->addresses = celix_arrayList_createStringArray();
        if (info->addresses == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        char* save = NULL;
        char* token = strtok_r(addressesCopy, ",", &save);
        while (token != NULL) {
            if (celix_earpmClient_matchIpFamily(token, family)) {
                if (celix_arrayList_addString(info->addresses, token) != CELIX_SUCCESS) {
                    errno = ENOMEM;
                    return NULL;
                }
            }
            token = strtok_r(NULL, ",", &save);
        }
        if (celix_arrayList_size(addressList) == 0) {
            errno = EINVAL;
            return NULL;
        }
        celix_steal_ptr(addressList);
    }

    return celix_steal_ptr(info);
}

static bool celix_earpmClient_brokerInfoDestroy(struct celix_ref* ref) {
    celix_earpm_client_broker_info_t* info = (celix_earpm_client_broker_info_t*)ref;
    celix_arrayList_destroy(info->addresses);
    free(info->bindInterface);
    free(info);
    return true;
}

static void celix_earpmClient_brokerInfoRetain(celix_earpm_client_broker_info_t* info) {
    celix_ref_get(&info->ref);
    return;
}

static void celix_earpmClient_brokerInfoRelease(celix_earpm_client_broker_info_t* info) {
    celix_ref_put(&info->ref, celix_earpmClient_brokerInfoDestroy);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_broker_info_t, celix_earpmClient_brokerInfoRelease)

celix_status_t celix_earpmClient_mqttBrokerEndpointAdded(void* handle, const endpoint_description_t* endpoint, char* matchedFilter CELIX_UNUSED) {
    assert(handle != NULL);
    assert(endpoint != NULL);
    celix_earpm_client_t* client = handle;
    const char* bindInterface = NULL;
    int port = (int)celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 0);
    port = port != 0 ? port : (int)celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0);
    if (port < 0) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char* addresses = celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, NULL);
    if (addresses == NULL) {
        addresses = celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, NULL);
        if (addresses == NULL) {
            return CELIX_ILLEGAL_ARGUMENT;
        }
        bindInterface = celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, NULL);
        if (addresses[0] == '\0' && bindInterface == NULL) {
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }
    int family = (int)celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, AF_UNSPEC);
    {
        celix_autoptr(celix_earpm_client_broker_info_t) info = celix_earpmClient_brokerInfoCreate(addresses, port,
                                                                                                  bindInterface, family);
        if (info == NULL) {
            celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(client->logHelper, "Failed to create broker information.");
            return errno;
        }
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
        celix_status_t status = celix_stringHashMap_put(client->brokerInfoMap, endpoint->id, info);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(client->logHelper, "Failed to add broker information to map. %d.", status);
            return status;
        }
        celix_steal_ptr(info);
    }

    celix_status_t status = celixThreadCondition_signal(&client->brokerInfoChangedOrExiting);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_warning(client->logHelper, "Failed to signal adding broker information. %d.", status);
    }
    return  CELIX_SUCCESS;
}

celix_status_t celix_earpmClient_mqttBrokerEndpointRemoved(void* handle, const endpoint_description_t* endpoint, char* matchedFilter CELIX_UNUSED) {
    assert(handle != NULL);
    assert(endpoint != NULL);
    celix_earpm_client_t* client = handle;
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
    celix_stringHashMap_remove(client->brokerInfoMap, endpoint->id);
    return  CELIX_SUCCESS;
}

static bool celix_earpmClient_validateTopic(const char* topic) {
    if (strlen(topic) == 0 || strlen(topic) > 1024) {
        return false;
    }
    //The characters +, #, and $ are part of the MQTT topic pattern syntax,
    // so they are not allowed in the topic name of celix event admin.
    if (strpbrk(topic, "#+$")) {
        return false;
    }
    return true;
}

celix_status_t celix_earpmClient_subscribe(celix_earpm_client_t* client, const char* topic, celix_earpm_qos_e qos) {
    assert(client != NULL);
    assert(topic != NULL);
    if (!celix_earpmClient_validateTopic(topic)) {
        celix_logHelper_error(client->logHelper, "Invalid topic pattern %s.", topic);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    char mqttTopic[strlen(topic) + 1];
    if (topic[strlen(topic) - 1] == '*') {
        strcpy(mqttTopic, topic);
        mqttTopic[strlen(topic) - 1] = '#';
        topic = mqttTopic;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
    celix_earpm_qos_e oldQos = (celix_earpm_qos_e)celix_stringHashMap_getLong(client->subscriptions, topic, CELIX_EARPM_QOS_UNKNOWN);
    celix_status_t status = celix_stringHashMap_putLong(client->subscriptions, topic, qos);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(client->logHelper, "Failed to add subscription to map.");
        return status;
    }
    if (client->connected) {
        int rc = mosquitto_subscribe_v5(client->mosq, NULL, topic, qos, MQTT_SUB_OPT_NO_LOCAL, NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            if (oldQos != CELIX_EARPM_QOS_UNKNOWN) {// rollback qos state
                (void)celix_stringHashMap_putLong(client->subscriptions, topic, oldQos);
            } else {
                (void)celix_stringHashMap_remove(client->subscriptions, topic);
            }
            celix_logHelper_error(client->logHelper, "Failed to subscribe topic %s with qos %d. %d", topic, (int)qos, rc);
            return CELIX_BUNDLE_EXCEPTION;
        }
    }

    return CELIX_SUCCESS;
}

celix_status_t celix_earpmClient_unsubscribe(celix_earpm_client_t* client, const char* topic) {
    assert(client != NULL);
    assert(topic != NULL);
    if (!celix_earpmClient_validateTopic(topic)) {
        celix_logHelper_error(client->logHelper, "Invalid topic pattern %s.", topic);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    char mqttTopic[strlen(topic) + 1];
    if (topic[strlen(topic) - 1] == '*') {
        strcpy(mqttTopic, topic);
        mqttTopic[strlen(topic) - 1] = '#';
        topic = mqttTopic;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
    if (client->connected) {
        (void)celix_stringHashMap_remove(client->subscriptions, topic);
        int rc = mosquitto_unsubscribe(client->mosq, NULL, topic);
        if (rc != MOSQ_ERR_SUCCESS) {
            celix_logHelper_warning(client->logHelper, "Failed to unsubscribe topic %s. %d", topic, rc);
            return CELIX_BUNDLE_EXCEPTION;
        }
    } else {
        celix_status_t status = celix_stringHashMap_putLong(client->subscriptions, topic, CELIX_EARPM_QOS_UNKNOWN);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(client->logHelper, "Failed to mark subscription expiry, %d.", status);
            return status;
        }
    }
    return CELIX_SUCCESS;
}

static inline bool celix_earpmClient_hasFreeMsgFor(celix_earpm_client_t* client, celix_earpm_client_message_priority_e msgPriority) {
    switch (msgPriority) {
        case CELIX_EARPM_MSG_PRI_LOW:
            return client->freeMsgPool.usedSize < client->freeMsgPool.cap * 70 / 100;
        case CELIX_EARPM_MSG_PRI_MIDDLE:
            return client->freeMsgPool.usedSize < client->freeMsgPool.cap * 85 / 100;
        case CELIX_EARPM_MSG_PRI_HIGH:
            return client->freeMsgPool.usedSize < client->freeMsgPool.cap;
    }
    assert(0);//LCOV_EXCL_LINE, should never be reached
}

static inline bool celix_earpmClient_isPublishingQueueFull(celix_earpm_client_t* client) {
    return celix_longHashMap_size(client->publishingMessages) >= client->parallelMsgCap;
}

static celix_earpm_client_msg_t* celix_earpmClient_messageCreate(celix_earpm_client_msg_pool_t* msgPool, const char* topic, celix_earpm_qos_e qos,
                                                                 celix_earpm_client_message_priority_e pri, bool sync) {
    static long nextSeqNr = 0;
    celix_earpm_client_msg_t* msg = celix_earpmClient_msgPoolAlloc(msgPool);
    if (msg == NULL) {
        return NULL;
    }
    celix_ref_init(&msg->ref);
    msg->msgPool = msgPool;
    msg->payloadSize = 0;
    msg->qos = qos;
    msg->pri = pri;
    msg->sync = sync;
    msg->seqNr = __atomic_fetch_add(&nextSeqNr, 1, __ATOMIC_RELAXED);
    msg->error = CELIX_SUCCESS;
    msg->mqttMid = -1;
    msg->payload = NULL;
    msg->payloadSize = 0;
    msg->mqttProps = NULL;
    msg->topic = celix_utils_strdup(topic);
    if (msg->topic == NULL) {
        celix_earpmClient_msgPoolFree(msgPool, msg);
        return NULL;
    }
    return celix_steal_ptr(msg);
}

static celix_status_t celix_earpmClient_fillMessagePayload(celix_earpm_client_msg_t* msg, const char* payload, size_t payloadSize, const mosquitto_property* mqttProps) {
    celix_autofree char* _payload = NULL;
    if (payloadSize > 0 && payload != NULL) {
        _payload = malloc(payloadSize);
        if (_payload == NULL) {
            return ENOMEM;
        }
        memcpy(_payload, payload, payloadSize);
    }
    if (mqttProps != NULL) {
        int rc = mosquitto_property_copy_all(&msg->mqttProps, mqttProps);
        if (rc != MOSQ_ERR_SUCCESS) {
            return ENOMEM;
        }
    }
    if (_payload != NULL) {
        msg->payload = celix_steal_ptr(_payload);
        msg->payloadSize = payloadSize;
    }
    return CELIX_SUCCESS;
}

static void celix_earpmClient_messageDestroy(celix_earpm_client_msg_t* msg) {
    mosquitto_property_free_all(&msg->mqttProps);
    free(msg->payload);
    free(msg->topic);
    celix_earpmClient_msgPoolFree(msg->msgPool, msg);
    return;
}

static void celix_earpmClient_messageRetain(celix_earpm_client_msg_t* msg) {
    celix_ref_get(&msg->ref);
    return;
}

static void celix_earpmClient_messageRelease(celix_earpm_client_msg_t* msg) {
    celix_ref_put(&msg->ref, (void *) celix_earpmClient_messageDestroy);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_msg_t, celix_earpmClient_messageRelease)

static celix_status_t celix_earpmClient_publishMessage(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg,
                                                       const char* payload, size_t payloadSize, const mosquitto_property* props) {
    int mid = 0;
    int rc = mosquitto_publish_v5(client->mosq, &mid, msg->topic, (int)payloadSize, payload, msg->qos, false, props);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to publish topic %s with qos %d, %d.", msg->topic, (int)msg->qos, rc);
        return CELIX_BUNDLE_EXCEPTION;
    }
    msg->mqttMid = mid;
    celix_status_t status = celix_longHashMap_put(client->publishingMessages, mid, msg);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(client->logHelper, "Failed to add message to publishing queue. %s.", msg->topic);
        return status;
    }
    celix_earpmClient_messageRetain(msg);
    return CELIX_SUCCESS;
}

static void celix_earpmClient_enqueueMsgToWaitingQueue(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg) {
    celix_earpm_client_msg_t* higherPriMsg = NULL;
    TAILQ_FOREACH_REVERSE(higherPriMsg, &client->waitingMessages, celix_earpm_client_message_list, entry) {
        if (higherPriMsg->pri >=  msg->pri) {
            break;
        }
    }
    if (higherPriMsg == NULL) {
        TAILQ_INSERT_HEAD(&client->waitingMessages, msg, entry);
    } else {
        TAILQ_INSERT_AFTER(&client->waitingMessages, higherPriMsg, msg, entry);
    }
    celix_earpmClient_messageRetain(msg);
    return;
}

static celix_status_t celix_earpmClient_publishDoNext(celix_earpm_client_t* client, celix_earpm_client_msg_t *msg, const char* payload, size_t payloadSize, const mosquitto_property* mqttProps) {
    celix_status_t status = CELIX_SUCCESS;
    if (client->connected && !celix_earpmClient_isPublishingQueueFull(client)) {
        //Publish directly, do not fill payload to message, decrease memory usage.
        status = celix_earpmClient_publishMessage(client, msg, payload, payloadSize, mqttProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(client->logHelper, "Failed to publish message %s, %d.", msg->topic, status);
            return status;
        }
    } else {
        status = celix_earpmClient_fillMessagePayload(msg, payload, payloadSize, mqttProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(client->logHelper, "Failed to fill payload for message %s. %d.", msg->topic, status);
            return status;
        }
        celix_earpmClient_enqueueMsgToWaitingQueue(client, msg);
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_earpmClient_publishAsync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, celix_earpm_qos_e qos, const mosquitto_property* mqttProps, celix_earpm_client_message_priority_e pri) {
    assert(client != NULL);
    assert(topic != NULL);
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
    if (qos <= CELIX_EARPM_QOS_AT_MOST_ONCE && !client->connected) {
        return ENOTCONN;
    }
    if (!celix_earpmClient_hasFreeMsgFor(client, pri)) {
        celix_logHelper_error(client->logHelper, "Too many messages wait for publish, dropping message with qos %d priority %d. %s.",
                              (int)qos, (int)pri, topic);
        return ENOMEM;
    }

    celix_autoptr(celix_earpm_client_msg_t) msg = celix_earpmClient_messageCreate(&client->freeMsgPool, topic, qos, pri, false);
    if (msg == NULL) {
        celix_logHelper_error(client->logHelper, "Failed to create message for %s.", topic);
        return ENOMEM;
    }

    return celix_earpmClient_publishDoNext(client, msg, payload, payloadSize, mqttProps);
}

static inline bool celix_earpmClient_isMsgPublished(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg) {
    return celix_longHashMap_hasKey(client->publishedMessages, msg->seqNr);
}

static void celix_earpmClient_deleteMsgFromQueue(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg) {
    bool removed = celix_longHashMap_remove(client->publishedMessages, msg->mqttMid);
    if (!removed) {
        removed = celix_longHashMap_remove(client->publishingMessages, msg->mqttMid);
    }
    if (!removed) {
        TAILQ_REMOVE(&client->waitingMessages, msg, entry);
        celix_earpmClient_messageRelease(msg);
    }
    celixThreadCondition_broadcast(&client->msgStatusChanged);
}

static celix_status_t celix_earpmClient_waitForMsgPublished(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg, const struct timespec* absTime) {
    while (!celix_earpmClient_isMsgPublished(client, msg)) {
        celix_status_t status = celixThreadCondition_waitUntil(&client->msgStatusChanged, &client->mutex, absTime);
        if (status != CELIX_SUCCESS) {
            celix_earpmClient_deleteMsgFromQueue(client, msg);
            celix_logHelper_error(client->logHelper, "Failed to waiting for message(%s) published. %d.", msg->topic, status);
            return status;
        }
    }
    (void)celix_longHashMap_remove(client->publishedMessages, msg->seqNr);
    if (msg->error != CELIX_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to publish message %s, mqtt reason code %d.", msg->topic, msg->error);
        return msg->error;
    }
    return CELIX_SUCCESS;
}

celix_status_t celix_earpmClient_publishSync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, celix_earpm_qos_e qos, const mosquitto_property* mqttProps, const struct timespec* absTime) {
    assert(client != NULL);
    assert(topic != NULL);
    assert(absTime != NULL);
    celix_status_t status = CELIX_SUCCESS;
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
    if (qos <= CELIX_EARPM_QOS_AT_MOST_ONCE && !client->connected) {
        celix_logHelper_warning(client->logHelper, "Mqtt client not connected, dropping sync message with qos %d. %s.", (int)qos, topic);
        return ENOTCONN;
    }
    while (!celix_earpmClient_hasFreeMsgFor(client, CELIX_EARPM_MSG_PRI_LOW)) {
        if (qos <= CELIX_EARPM_QOS_AT_MOST_ONCE) {
            celix_logHelper_warning(client->logHelper, "Too many messages wait for publish, dropping sync message with qos %d. %s.", (int)qos, topic);
            return ENOMEM;
        }
        celix_logHelper_warning(client->logHelper, "Too many messages wait for publish, waiting for message queue idle. %s.", topic);
        status = celixThreadCondition_waitUntil(&client->msgStatusChanged, &client->mutex, absTime);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_warning(client->logHelper, "Failed to waiting for message queue idle. %d.", status);
            return status;
        }
    }
    celix_autoptr(celix_earpm_client_msg_t) msg = celix_earpmClient_messageCreate(&client->freeMsgPool, topic, qos,
                                                                                  CELIX_EARPM_MSG_PRI_LOW, true);
    if (msg == NULL) {
        celix_logHelper_error(client->logHelper, "Failed to create message for %s.", topic);
        return ENOMEM;
    }
    status = celix_earpmClient_publishDoNext(client, msg, payload, payloadSize, mqttProps);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    return celix_earpmClient_waitForMsgPublished(client, msg, absTime);
}

static void celix_earpmClient_enqueueMsgToPublishedQueue(celix_earpm_client_t* client, celix_earpm_client_msg_t* msg, celix_status_t error) {
    if (msg->sync == false) {//Async message no need to release to published queue.
        return;
    }
    msg->error = error;
    celix_status_t status = celix_longHashMap_put(client->publishedMessages, msg->seqNr, msg);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(client->logHelper, "Failed to add message(%ld) to published queue.", msg->seqNr);
    } else {
        celix_earpmClient_messageRetain(msg);
    }
    return;
}

static void celix_earpmClient_releaseWaitingMsgToPublishing(celix_earpm_client_t* client) {
    for (celix_earpm_client_msg_t *curMsg = TAILQ_FIRST(&client->waitingMessages), *nextMsg = NULL;
    (curMsg != NULL) && !celix_earpmClient_isPublishingQueueFull(client); curMsg = nextMsg) {
        nextMsg = TAILQ_NEXT(curMsg, entry);
        celix_status_t status = celix_earpmClient_publishMessage(client, curMsg, curMsg->payload, curMsg->payloadSize,
                                                                 curMsg->mqttProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(client->logHelper, "Failed to publish waiting message %s, %d.", curMsg->topic, status);
            celix_earpmClient_enqueueMsgToPublishedQueue(client, curMsg, status);
        }
        TAILQ_REMOVE(&client->waitingMessages, curMsg, entry);
        celix_earpmClient_messageRelease(curMsg);
    }
    return;
}

static void celix_earpmClient_refreshSubscriptions(celix_earpm_client_t* client) {
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(client->subscriptions);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        const char* topic = iter.key;
        celix_earpm_qos_e qos = (celix_earpm_qos_e)iter.value.longValue;
        if (qos > CELIX_EARPM_QOS_UNKNOWN) {
            int rc = mosquitto_subscribe_v5(client->mosq, NULL, topic, qos, MQTT_SUB_OPT_NO_LOCAL, NULL);
            if (rc != MOSQ_ERR_SUCCESS) {
                celix_logHelper_error(client->logHelper, "Error subscribing to topic %s with qos %d. %d.", topic, (int)qos, rc);
            }
            celix_stringHashMapIterator_next(&iter);
        } else {
            int rc = mosquitto_unsubscribe(client->mosq, NULL, topic);
            if (rc != MOSQ_ERR_SUCCESS) {
                celix_logHelper_warning(client->logHelper, "Error unsubscribing from topic %s. %d.", topic, rc);
            }
            celix_stringHashMapIterator_remove(&iter);
        }
    }
    return;
}

static void celix_earpmClient_connectCallback(struct mosquitto* mosq CELIX_UNUSED, void* handle, int rc, int flag CELIX_UNUSED, const mosquitto_property* props CELIX_UNUSED) {
    assert(handle != NULL);
    celix_earpm_client_t* client = handle;
    if (rc != MQTT_RC_SUCCESS) {
        celix_logHelper_error(client->logHelper, "Failed to connect to mqtt broker. %d.", rc);
        return;
    }
    celix_logHelper_trace(client->logHelper, "Connected to broker.");
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
        client->connected = true;
        celix_earpmClient_refreshSubscriptions(client);
        celix_earpmClient_releaseWaitingMsgToPublishing(client);
    }
    celixThreadCondition_broadcast(&client->msgStatusChanged);

    client->connectedCallback(client->callbackHandle);
    return;
}

static void celix_earpmClient_dropQos0Messages(celix_earpm_client_t* client) {
    celix_long_hash_map_iterator_t iter = celix_longHashMap_begin(client->publishingMessages);
    while (!celix_longHashMapIterator_isEnd(&iter)) {
        celix_earpm_client_msg_t* msg = (celix_earpm_client_msg_t*)iter.value.ptrValue;
        if (msg->qos <= CELIX_EARPM_QOS_AT_MOST_ONCE) {
            celix_logHelper_warning(client->logHelper, "Mqtt disconnected, drop publishing message with qos %d. %s.", (int)msg->qos, msg->topic);
            celix_earpmClient_enqueueMsgToPublishedQueue(client, msg, CELIX_ILLEGAL_STATE);
            celix_longHashMapIterator_remove(&iter);
        } else {
            celix_longHashMapIterator_next(&iter);
        }
    }

    for (celix_earpm_client_msg_t *curMsg = TAILQ_FIRST(&client->waitingMessages), *nextMsg = NULL; curMsg != NULL; curMsg = nextMsg) {
        nextMsg = TAILQ_NEXT(curMsg, entry);
        if (curMsg->qos <= CELIX_EARPM_QOS_AT_MOST_ONCE) {
            celix_logHelper_warning(client->logHelper, "Mqtt disconnected, drop waiting message with qos %d. %s.", (int)curMsg->qos, curMsg->topic);
            celix_earpmClient_enqueueMsgToPublishedQueue(client, curMsg, CELIX_ILLEGAL_STATE);
            TAILQ_REMOVE(&client->waitingMessages, curMsg, entry);
            celix_earpmClient_messageRelease(curMsg);
        }
    }
    return;
}

static void celix_earpmClient_disconnectCallback(struct mosquitto* mosq CELIX_UNUSED, void* handle, int rc, const mosquitto_property* props CELIX_UNUSED) {
    assert(handle != NULL);
    celix_earpm_client_t* client = handle;
    celix_logHelper_trace(client->logHelper, "Disconnected from broker. %d", rc);

    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
        client->connected = false;
        celix_earpmClient_dropQos0Messages(client);
        //QOS1 and QOS2 messages will be resent when reconnecting.
    }
    celixThreadCondition_broadcast(&client->msgStatusChanged);
    return;
}

static void celix_earpmClient_messageCallback(struct mosquitto* mosq CELIX_UNUSED, void* handle, const struct mosquitto_message* message, const mosquitto_property* props) {
    assert(handle != NULL);
    assert(message != NULL);
    assert(message->topic != NULL);
    celix_earpm_client_t* client = handle;

    celix_logHelper_trace(client->logHelper, "Received message on topic %s.", message->topic);
    client->receiveMsgCallback(client->callbackHandle, message->topic, message->payload, message->payloadlen, props);
    return;
}

static void celix_earpmClient_publishCallback(struct mosquitto* mosq CELIX_UNUSED, void* handle, int mid, int reasonCode, const mosquitto_property *props CELIX_UNUSED) {
    assert(handle != NULL);
    celix_earpm_client_t* client = handle;
    celix_log_level_e logLevel = (reasonCode == MQTT_RC_SUCCESS || reasonCode == MQTT_RC_NO_MATCHING_SUBSCRIBERS) ? CELIX_LOG_LEVEL_TRACE : CELIX_LOG_LEVEL_ERROR;
    celix_logHelper_log(client->logHelper, logLevel, "Published message(mid:%d). reason code %d", mid, reasonCode);

    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
        celix_earpm_client_msg_t* msg = celix_longHashMap_get(client->publishingMessages, mid);
        if (msg != NULL) {
            celix_status_t status = (reasonCode == MQTT_RC_SUCCESS || reasonCode == MQTT_RC_NO_MATCHING_SUBSCRIBERS) ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
            celix_earpmClient_enqueueMsgToPublishedQueue(client, msg, status);
            celix_longHashMap_remove(client->publishingMessages, mid);
        }
        celix_earpmClient_releaseWaitingMsgToPublishing(client);
    }
    celixThreadCondition_broadcast(&client->msgStatusChanged);
    return;
}

static int celix_earpmClient_connectBrokerWithHost(celix_earpm_client_t* client, const char* host, int port) {
    int rc = mosquitto_connect_bind_v5(client->mosq, host, port, CELIX_EARPM_CLIENT_KEEP_ALIVE, NULL, client->connProps);
    celix_logHelper_info(client->logHelper, "Connected to broker %s:%i. %d.", host, port, rc);
    return rc == MOSQ_ERR_SUCCESS ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
}

static int celix_earpmClient_connectBrokerWithInterface(celix_earpm_client_t* client, const char* interfaceName, int port, int family) {
    struct ifaddrs* ifaddr = NULL;
    struct ifaddrs* ifa = NULL;
    if (getifaddrs(&ifaddr) < 0) {
        celix_logHelper_error(client->logHelper, "Failed to get interface address for %s. %d.", interfaceName, errno);
        return errno;
    }
    celix_status_t status = CELIX_ILLEGAL_STATE;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if(celix_utils_stringEquals(ifa->ifa_name, interfaceName) && (ifa->ifa_addr->sa_family == family || family == AF_UNSPEC)) {
            char host[INET6_ADDRSTRLEN] = {0};
            inet_ntop(ifa->ifa_addr->sa_family, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, host, INET6_ADDRSTRLEN);
            if (ifa->ifa_addr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, host, INET6_ADDRSTRLEN);
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr, host, INET6_ADDRSTRLEN);
            }
            status = celix_earpmClient_connectBrokerWithHost(client, host, port);
            if (status == CELIX_SUCCESS) {
                break;
            }
        }
    }
    if (ifaddr != NULL) {
        freeifaddrs(ifaddr);
    }
    return status;
}

static int celix_earpmClient_connectBroker(celix_earpm_client_t* client) {
    celixThreadMutex_lock(&client->mutex);
    bool curBrokerEndpointExisted = false;
    if (client->currentBrokerEndpointUUID != NULL) {
        curBrokerEndpointExisted = celix_stringHashMap_hasKey(client->brokerInfoMap, client->currentBrokerEndpointUUID);
    }
    celixThreadMutex_unlock(&client->mutex);
    if (curBrokerEndpointExisted && mosquitto_reconnect(client->mosq) == MOSQ_ERR_SUCCESS) {
        return CELIX_SUCCESS;
    }
    celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    opts.simpleRemovedCallback = (void *) celix_earpmClient_brokerInfoRelease;
    celix_autoptr(celix_string_hash_map_t) brokerInfoMap = celix_stringHashMap_createWithOptions(&opts);
    if (brokerInfoMap == NULL) {
        celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(client->logHelper, "Failed to create broker info map.");
        return ENOMEM;
    }
    celixThreadMutex_lock(&client->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(client->brokerInfoMap, iter) {
        if (celix_stringHashMap_put(brokerInfoMap, iter.key, iter.value.ptrValue) == CELIX_SUCCESS) {
            celix_earpmClient_brokerInfoRetain((celix_earpm_client_broker_info_t *) iter.value.ptrValue);
        } else {
            celix_logHelper_logTssErrors(client->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(client->logHelper, "Failed to copy broker info.");
        }
    }
    celixThreadMutex_unlock(&client->mutex);

    celix_status_t status = CELIX_ILLEGAL_STATE;
    CELIX_STRING_HASH_MAP_ITERATE(brokerInfoMap, iter) {
        celix_earpm_client_broker_info_t* info = iter.value.ptrValue;
        const char* address = "";
        if (info->bindInterface != NULL) {
            status = celix_earpmClient_connectBrokerWithInterface(client, info->bindInterface, info->port, info->family);
        } else {
            int size = celix_arrayList_size(info->addresses);
            for (int i = 0; i < size; ++i) {
                address = celix_arrayList_getString(info->addresses, i);
                status = celix_earpmClient_connectBrokerWithHost(client, address, info->port);
            }
        }
        if (status == CELIX_SUCCESS) {
            celix_logHelper_info(client->logHelper, "Connected to broker %s:%i", address, info->port);
            free(client->currentBrokerEndpointUUID);
            client->currentBrokerEndpointUUID = celix_utils_strdup(iter.key);
            return CELIX_SUCCESS;
        }
    }
    return status;
}

static void* celix_earpmClient_mosqThread(void* data) {
    assert(data != NULL);
    celix_earpm_client_t* client = (celix_earpm_client_t*)data;

    unsigned int reconnectDelay = 0;
    unsigned int reconnectCount = 0;
    bool running = client->running;
    while (running) {
        celixThreadMutex_lock(&client->mutex);
        while (client->running && (celix_stringHashMap_size(client->brokerInfoMap) == 0 || reconnectDelay > 0)) {
            if (reconnectDelay == 0) {
                celixThreadCondition_wait(&client->brokerInfoChangedOrExiting, &client->mutex);
            } else {
                celixThreadCondition_timedwaitRelative(&client->brokerInfoChangedOrExiting, &client->mutex, reconnectDelay, 0);
                reconnectDelay = 0;
            }
        }
        running = client->running;
        celixThreadMutex_unlock(&client->mutex);

        if (running) {
            celix_status_t status = celix_earpmClient_connectBroker(client);
            if (status == CELIX_SUCCESS) {
                reconnectCount = 0;
            } else {
                reconnectCount++;
                reconnectDelay = reconnectCount * 1;
                reconnectDelay = (reconnectDelay < CELIX_EARPM_CLIENT_RECONNECT_DELAY_MAX) ? reconnectDelay : CELIX_EARPM_CLIENT_RECONNECT_DELAY_MAX;
                celix_logHelper_info(client->logHelper, "Failed to connect to broker, retry after %u second.", reconnectDelay);
            }
            {
                //If mosquitto_disconnect_v5 is called before celix_earpmc_connectBroker when celix_earpmc_destroy is occurred,
                // it will induce current thread can't exit in time. So, check the running flag again here.
                celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&client->mutex);
                if (client->running == false) {
                    return NULL;
                }
            }
            while (status == CELIX_SUCCESS) {
                int rc = mosquitto_loop(client->mosq, CELIX_EARPM_CLIENT_KEEP_ALIVE * 1000, 1);
                status = (rc == MOSQ_ERR_SUCCESS) ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
            }
        }
    }
    return NULL;
}
