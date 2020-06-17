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

#include <pubsub_serializer.h>
#include <stdlib.h>
#include <pubsub/subscriber.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <assert.h>
#include <pubsub_endpoint.h>
#include <arpa/inet.h>
#include <celix_log_helper.h>
#include <math.h>
#include "pubsub_websocket_topic_receiver.h"
#include "pubsub_psa_websocket_constants.h"
#include "pubsub_websocket_common.h"

#include <uuid/uuid.h>
#include <http_admin/api.h>
#include <jansson.h>
#include <pubsub_utils.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37
#endif


#define L_DEBUG(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

typedef struct pubsub_websocket_rcv_buffer {
    celix_thread_mutex_t mutex;
    celix_array_list_t *list;     //List of received websocket messages (type: pubsub_websocket_msg_entry_t *)
} pubsub_websocket_rcv_buffer_t;

typedef struct pubsub_websocket_msg_entry {
    size_t msgSize;
    const char *msgData;
} pubsub_websocket_msg_entry_t;

struct pubsub_websocket_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *uri;

    celix_websocket_service_t sockSvc;
    long svcId;

    pubsub_websocket_rcv_buffer_t recvBuffer;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = url (host:port), value = psa_websocket_requested_connection_entry_t*
        bool allConnected; //true if all requestedConnectection are connected
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_websocket_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_websocket_requested_connection_entry {
    char *key; //host:port
    char *socketAddress;
    long socketPort;
    char *uri;
    struct mg_connection *sockConnection;
    int connectRetryCount;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
    bool passive; //true if the connection is initiated by another resource (e.g. webpage)
} psa_websocket_requested_connection_entry_t;

typedef struct psa_websocket_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //key = msg type id, value = pubsub_msg_serializer_t
    pubsub_subscriber_t *svc;
    bool initialized; //true if the init function is called through the receive thread
} psa_websocket_subscriber_entry_t;


static void pubsub_websocketTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_websocketTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void* psa_websocket_recvThread(void * data);
static void psa_websocket_connectToAllRequestedConnections(pubsub_websocket_topic_receiver_t *receiver);
static void psa_websocket_initializeAllSubscribers(pubsub_websocket_topic_receiver_t *receiver);
static void *psa_websocket_getMsgTypeIdFromFqn(const char *fqn, hash_map_t *msg_type_id_map);

static void psa_websocketTopicReceiver_ready(struct mg_connection *connection, void *handle);
static int psa_websocketTopicReceiver_data(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle);
static void psa_websocketTopicReceiver_close(const struct mg_connection *connection, void *handle);


pubsub_websocket_topic_receiver_t* pubsub_websocketTopicReceiver_create(celix_bundle_context_t *ctx,
                                                              celix_log_helper_t *logHelper,
                                                              const char *scope,
                                                              const char *topic,
                                                              const celix_properties_t *topicProperties,
                                                              long serializerSvcId,
                                                              pubsub_serializer_service_t *serializer) {
    pubsub_websocket_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    receiver->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    psa_websocket_setScopeAndTopicFilter(scope, topic, receiver->scopeAndTopicFilter);

    receiver->uri = psa_websocket_createURI(scope, topic);

    if (receiver->uri != NULL) {
        celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
        celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
        celixThreadMutex_create(&receiver->recvThread.mutex, NULL);
        celixThreadMutex_create(&receiver->recvBuffer.mutex, NULL);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        arrayList_create(&receiver->recvBuffer.list);
    }

    //track subscribers
    if (receiver->uri != NULL) {
        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size+1];
        snprintf(buf, (size_t)size+1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_websocketTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_websocketTopicReceiver_removeSubscriber;

        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //Register a websocket endpoint for this topic receiver
    if(receiver->uri != NULL){
        //Register a websocket svc first
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, WEBSOCKET_ADMIN_URI, receiver->uri);
        receiver->sockSvc.handle = receiver;
        //Set callbacks to monitor any incoming connections (passive), data events or close events
        receiver->sockSvc.ready = psa_websocketTopicReceiver_ready;
        receiver->sockSvc.data = psa_websocketTopicReceiver_data;
        receiver->sockSvc.close = psa_websocketTopicReceiver_close;
        receiver->svcId = celix_bundleContext_registerService(receiver->ctx, &receiver->sockSvc,
                                                           WEBSOCKET_ADMIN_SERVICE_NAME, props);
    }

    const char *staticConnects = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR, topic, scope);
    if(staticConnects == NULL) {
        staticConnects = celix_properties_get(topicProperties, PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES, NULL);
    }
    if (staticConnects != NULL) {
        char *copy = strndup(staticConnects, 1024*1024);
        char* addr;
        char* save = copy;

        while ((addr = strtok_r(save, " ", &save))) {
            char *colon = strchr(addr, ':');
            if (colon == NULL) {
                continue;
            }

            char *sockAddr = NULL;
            asprintf(&sockAddr, "%.*s", (int)(colon - addr), addr);

            long sockPort = atol((colon + 1));

            char *key = NULL;
            asprintf(&key, "%s:%li", sockAddr, sockPort);


            if (sockPort > 0) {
                psa_websocket_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
                entry->key = key;
                entry->uri = strndup(receiver->uri, 1024 * 1024);
                entry->socketAddress = sockAddr;
                entry->socketPort = sockPort;
                entry->connected = false;
                entry->statically = true;
                entry->passive = false;
                hashMap_put(receiver->requestedConnections.map, (void *) entry->key, entry);
            } else {
                L_WARN("[PSA_WEBSOCKET_TR] Invalid static socket address %s", addr);
                free(key);
                free(sockAddr);
            }
        }
        free(copy);
    }


    if (receiver->uri != NULL) {
        receiver->recvThread.running = true;
        celixThread_create(&receiver->recvThread.thread, NULL, psa_websocket_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "WEBSOCKET TR %s/%s", scope == NULL ? "(null)" : scope, topic);
        celixThread_setName(&receiver->recvThread.thread, name);
    }

    if (receiver->uri == NULL) {
        free(receiver->scope);
        free(receiver->topic);
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_WEBSOCKET] Cannot create TopicReceiver for %s/%s", scope == NULL ? "(null)" : scope, topic);
    }

    return receiver;
}

void pubsub_websocketTopicReceiver_destroy(pubsub_websocket_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        receiver->recvThread.running = false;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
        celixThread_join(receiver->recvThread.thread, NULL);

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celix_bundleContext_unregisterService(receiver->ctx, receiver->svcId);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL)  {
                receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                free(entry);
            }

        }
        hashMap_destroy(receiver->subscribers.map, false, false);


        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                if(entry->connected) {
                    mg_close_connection(entry->sockConnection);
                }
                free(entry->uri);
                free(entry->socketAddress);
                free(entry->key);
                free(entry);
            }
        }
        hashMap_destroy(receiver->requestedConnections.map, false, false);
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_destroy(&receiver->subscribers.mutex);
        celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        celixThreadMutex_destroy(&receiver->recvThread.mutex);

        celixThreadMutex_destroy(&receiver->recvBuffer.mutex);
        int msgBufSize = celix_arrayList_size(receiver->recvBuffer.list);
        while(msgBufSize > 0) {
            pubsub_websocket_msg_entry_t *msg = celix_arrayList_get(receiver->recvBuffer.list, msgBufSize - 1);
            free((void *) msg->msgData);
            free(msg);
            msgBufSize--;
        }
        celix_arrayList_destroy(receiver->recvBuffer.list);

        free(receiver->uri);
        free(receiver->scope);
        free(receiver->topic);
    }
    free(receiver);
}

const char* pubsub_websocketTopicReceiver_scope(pubsub_websocket_topic_receiver_t *receiver) {
    return receiver->scope;
}
const char* pubsub_websocketTopicReceiver_topic(pubsub_websocket_topic_receiver_t *receiver) {
    return receiver->topic;
}
const char* pubsub_websocketTopicReceiver_url(pubsub_websocket_topic_receiver_t *receiver) {
    return receiver->uri;
}

long pubsub_websocketTopicReceiver_serializerSvcId(pubsub_websocket_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

void pubsub_websocketTopicReceiver_listConnections(pubsub_websocket_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        char *url = NULL;
        asprintf(&url, "%s:%li%s%s", entry->socketAddress, entry->socketPort, entry->statically ? " (static)" : "", entry->passive ? " (passive)" : "");
        if (entry->connected) {
            celix_arrayList_add(connectedUrls, url);
        } else {
            celix_arrayList_add(unconnectedUrls, url);
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}


void pubsub_websocketTopicReceiver_connectTo(pubsub_websocket_topic_receiver_t *receiver, const char *socketAddress, long socketPort) {
    L_DEBUG("[PSA_WEBSOCKET] TopicReceiver %s/%s ('%s') connecting to websocket address %s:li", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, receiver->uri, socketAddress, socketPort);

    char *key = NULL;
    asprintf(&key, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_websocket_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->key = key;
        entry->uri = strndup(receiver->uri, 1024 * 1024);
        entry->socketAddress = strndup(socketAddress, 1024 * 1024);
        entry->socketPort = socketPort;
        entry->connected = false;
        entry->statically = false;
        entry->passive = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->key, entry);
        receiver->requestedConnections.allConnected = false;
    } else {
        free(key);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    psa_websocket_connectToAllRequestedConnections(receiver);
}

void pubsub_websocketTopicReceiver_disconnectFrom(pubsub_websocket_topic_receiver_t *receiver, const char *socketAddress, long socketPort) {
    L_DEBUG("[PSA_WEBSOCKET] TopicReceiver %s/%s ('%s') disconnect from websocket address %s:%li", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, receiver->uri, socketAddress, socketPort);

    char *key = NULL;
    asprintf(&key, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);

    psa_websocket_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, key);
    if (entry != NULL && entry->connected) {
        mg_close_connection(entry->sockConnection);
    }
    if (entry != NULL) {
        free(entry->socketAddress);
        free(entry->uri);
        free(entry->key);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
    free(key);
}

static void pubsub_websocketTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_websocket_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, NULL);
    if (receiver->scope == NULL){
        if (subScope != NULL){
            return;
        }
    } else if (subScope != NULL) {
        if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
            //not the same scope. ignore
            return;
        }
    }

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_websocket_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount += 1;
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->usageCount = 1;
        entry->svc = svc;
        entry->initialized = false;

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);

        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_WEBSOCKET] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
            free(entry);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_websocketTopicReceiver_removeSubscriber(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props __attribute__((unused)), const celix_bundle_t *bnd) {
    pubsub_websocket_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_websocket_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_WEBSOCKET] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
        }
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void * psa_websocket_getMsgTypeIdFromFqn(const char *fqn, hash_map_t *msg_type_id_map) {
    void *msgTypeId = NULL;
    if(fqn != NULL && msg_type_id_map != NULL) {
        hash_map_iterator_t iter = hashMapIterator_construct(msg_type_id_map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *entry = hashMapIterator_nextEntry(&iter);
            pubsub_msg_serializer_t *serializer = hashMapEntry_getValue(entry);
            if(strcmp(serializer->msgName, fqn) == 0) {
                msgTypeId =  hashMapEntry_getKey(entry);
                return msgTypeId;
            }
        }
    }

    return msgTypeId;
}

static inline void processMsgForSubscriberEntry(pubsub_websocket_topic_receiver_t *receiver, psa_websocket_subscriber_entry_t* entry, pubsub_websocket_msg_header_t *hdr, const char* payload, size_t payloadSize) {
    //NOTE receiver->subscribers.mutex locked
    void *msgTypeId = psa_websocket_getMsgTypeIdFromFqn(hdr->id, entry->msgTypes);
    pubsub_msg_serializer_t* msgSer = hashMap_get(entry->msgTypes, msgTypeId);
    pubsub_subscriber_t *svc = entry->svc;

    if (msgSer!= NULL && msgTypeId != 0) {
        void *deSerializedMsg = NULL;
        bool validVersion = psa_websocket_checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            struct iovec deSerializeBuffer;
            deSerializeBuffer.iov_base = (void *)payload;
            deSerializeBuffer.iov_len  = payloadSize;
            celix_status_t status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 0, &deSerializedMsg);

            if (status == CELIX_SUCCESS) {
                bool release = true;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deSerializedMsg, NULL, &release);
                if (release) {
                    msgSer->freeDeserializeMsg(msgSer->handle, deSerializedMsg);
                }
            } else {
                L_WARN("[PSA_WEBSOCKET_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_WEBSOCKET_TR] Cannot find serializer for type id 0x%X, fqn %s", msgTypeId, hdr->id);
    }
}

static inline void processMsg(pubsub_websocket_topic_receiver_t *receiver, const char *msg, size_t msgSize) {
    json_error_t error;
    json_t *jsMsg = json_loadb(msg, msgSize, 0, &error);
    if(jsMsg != NULL) {
        json_t *jsId = json_object_get(jsMsg, "id");
        json_t *jsMajor = json_object_get(jsMsg, "major");
        json_t *jsMinor = json_object_get(jsMsg, "minor");
        json_t *jsSeqNr = json_object_get(jsMsg, "seqNr");
        json_t *jsData = json_object_get(jsMsg, "data");

        if (jsId && jsMajor && jsMinor && jsSeqNr && jsData) {
            pubsub_websocket_msg_header_t hdr;
            hdr.id = json_string_value(jsId);
            hdr.major = (uint8_t) json_integer_value(jsMajor);
            hdr.minor = (uint8_t) json_integer_value(jsMinor);
            hdr.seqNr = (uint32_t) json_integer_value(jsSeqNr);
            const char *payload = json_dumps(jsData, 0);
            size_t payloadSize = strlen(payload);
            printf("Received msg: id %s\tmajor %u\tminor %u\tseqNr %u\tdata %s\n", hdr.id, hdr.major, hdr.minor, hdr.seqNr, payload);

            celixThreadMutex_lock(&receiver->subscribers.mutex);
            hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
            while (hashMapIterator_hasNext(&iter)) {
                psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
                if (entry != NULL) {
                    processMsgForSubscriberEntry(receiver, entry, &hdr, payload, payloadSize);
                }
            }
            celixThreadMutex_unlock(&receiver->subscribers.mutex);
            free((void *) payload);
        } else {
            L_WARN("[PSA_WEBSOCKET_TR] Received unsupported message: "
                   "ID = %s, major = %d, minor = %d, seqNr = %d, data valid? %s",
                   (jsId ? json_string_value(jsId) : "ERROR"),
                   json_integer_value(jsMajor), json_integer_value(jsMinor),
                   json_integer_value(jsSeqNr), (jsData ? "TRUE" : "FALSE"));
        }
        json_decref(jsMsg);
    } else {
        L_WARN("[PSA_WEBSOCKET_TR] Failed to load websocket JSON message, error line: %d, error message: %s", error.line, error.text);
        return;
    }

}

static void* psa_websocket_recvThread(void * data) {
    pubsub_websocket_topic_receiver_t *receiver = data;

    celixThreadMutex_lock(&receiver->recvThread.mutex);
    bool running = receiver->recvThread.running;
    celixThreadMutex_unlock(&receiver->recvThread.mutex);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    bool allConnected = receiver->requestedConnections.allConnected;
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    bool allInitialized = receiver->subscribers.allInitialized;
    celixThreadMutex_unlock(&receiver->subscribers.mutex);


    while (running) {
        if (!allConnected) {
            psa_websocket_connectToAllRequestedConnections(receiver);
        }
        if (!allInitialized) {
            psa_websocket_initializeAllSubscribers(receiver);
        }

        while(celix_arrayList_size(receiver->recvBuffer.list) > 0) {
            celixThreadMutex_lock(&receiver->recvBuffer.mutex);
            pubsub_websocket_msg_entry_t *msg = (pubsub_websocket_msg_entry_t *) celix_arrayList_get(receiver->recvBuffer.list, 0);
            celix_arrayList_removeAt(receiver->recvBuffer.list, 0);
            celixThreadMutex_unlock(&receiver->recvBuffer.mutex);

            processMsg(receiver, msg->msgData, msg->msgSize);
            free((void *)msg->msgData);
            free(msg);
        }

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        running = receiver->recvThread.running;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        allConnected = receiver->requestedConnections.allConnected;
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        allInitialized = receiver->subscribers.allInitialized;
        celixThreadMutex_unlock(&receiver->subscribers.mutex);
    } // while

    return NULL;
}

static void psa_websocketTopicReceiver_ready(struct mg_connection *connection, void *handle) {
    if (handle != NULL) {
        pubsub_websocket_topic_receiver_t *receiver = (pubsub_websocket_topic_receiver_t *) handle;

        //Get request info with host, port and uri information
        const struct mg_request_info *ri = mg_get_request_info(connection);
        if (ri != NULL && strcmp(receiver->uri, ri->request_uri) == 0) {
            char *key = NULL;
            asprintf(&key, "%s:%i", ri->remote_addr, ri->remote_port);

            celixThreadMutex_lock(&receiver->requestedConnections.mutex);
            psa_websocket_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
            if (entry == NULL) {
                entry = calloc(1, sizeof(*entry));
                entry->key = key;
                entry->uri = strndup(ri->request_uri, 1024 * 1024);
                entry->socketAddress = strndup(ri->remote_addr, 1024 * 1024);
                entry->socketPort = ri->remote_port;
                entry->connected = true;
                entry->statically = false;
                entry->passive = true;
                hashMap_put(receiver->requestedConnections.map, (void *) entry->key, entry);
                receiver->requestedConnections.allConnected = false;
            } else {
                free(key);
            }

            celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
        }
    }
}


static int psa_websocketTopicReceiver_data(struct mg_connection *connection __attribute__((unused)),
                                            int op_code __attribute__((unused)),
                                            char *data,
                                            size_t length,
                                            void *handle) {
    //Received a websocket message, append this message to the buffer of the receiver.
    if (handle != NULL) {
        pubsub_websocket_topic_receiver_t *receiver = (pubsub_websocket_topic_receiver_t *) handle;

        celixThreadMutex_lock(&receiver->recvBuffer.mutex);
        pubsub_websocket_msg_entry_t *msg = malloc(sizeof(*msg));
        const char *rcvdMsgData = malloc(length);
        memcpy((void *) rcvdMsgData, data, length);
        msg->msgData = rcvdMsgData;
        msg->msgSize = length;
        celix_arrayList_add(receiver->recvBuffer.list, msg);
        celixThreadMutex_unlock(&receiver->recvBuffer.mutex);
    }

    return 1; //keep open (non-zero), 0 to close the socket
}

static void psa_websocketTopicReceiver_close(const struct mg_connection *connection, void *handle) {
    //Reset connection for this receiver entry
    if (handle != NULL) {
        pubsub_websocket_topic_receiver_t *receiver = (pubsub_websocket_topic_receiver_t *) handle;

        //Get request info with host, port and uri information
        const struct mg_request_info *ri = mg_get_request_info(connection);
        if (ri != NULL && strcmp(receiver->uri, ri->request_uri) == 0) {
            char *key = NULL;
            asprintf(&key, "%s:%i", ri->remote_addr, ri->remote_port);

            celixThreadMutex_lock(&receiver->requestedConnections.mutex);
            psa_websocket_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
            if (entry != NULL) {
                entry->connected = false;
                entry->sockConnection = NULL;
                if(entry->passive) {
                    hashMap_remove(receiver->requestedConnections.map, key);
                    free(entry->key);
                    free(entry->uri);
                    free(entry->socketAddress);
                    free(entry);
                }
            }
            celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
            free(key);
        }
    }
}


static void psa_websocket_connectToAllRequestedConnections(pubsub_websocket_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->connected && !entry->passive) {
                char errBuf[100] = {0};
                entry->sockConnection = mg_connect_websocket_client(entry->socketAddress,
                                                                    (int) entry->socketPort,
                                                                    0, // No ssl
                                                                    errBuf,
                                                                    (size_t) sizeof(errBuf),
                                                                    entry->uri,
                                                                    NULL,
                                                                    psa_websocketTopicReceiver_data,
                                                                    psa_websocketTopicReceiver_close,
                                                                    receiver);
                if(entry->sockConnection != NULL) {
                    entry->connected = true;
                    entry->connectRetryCount = 0;
                } else {
                    entry->connectRetryCount += 1;
                    allConnected = false;
                    if((entry->connectRetryCount % 10) == 0) {
                        L_WARN("[PSA_WEBSOCKET] Error connecting to websocket %s:%li/%s. Error: %s",
                               entry->socketAddress,
                               entry->socketPort,
                               entry->uri, errBuf);
                    }
                }
            }
        }
        receiver->requestedConnections.allConnected = allConnected;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_websocket_initializeAllSubscribers(pubsub_websocket_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    if (!receiver->subscribers.allInitialized) {
        bool allInitialized = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->initialized) {
                int rc = 0;
                if (entry->svc != NULL && entry->svc->init != NULL) {
                    rc = entry->svc->init(entry->svc->handle);
                }
                if (rc == 0) {
                    entry->initialized = true;
                } else {
                    L_WARN("Cannot initialize subscriber svc. Got rc %i", rc);
                    allInitialized = false;
                }
            }
        }
        receiver->subscribers.allInitialized = allInitialized;
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}
