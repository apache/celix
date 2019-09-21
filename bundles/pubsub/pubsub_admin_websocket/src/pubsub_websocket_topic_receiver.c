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
#include <sys/epoll.h>
#include <assert.h>
#include <pubsub_endpoint.h>
#include <arpa/inet.h>
#include <log_helper.h>
#include <math.h>
#include "pubsub_websocket_topic_receiver.h"
#include "pubsub_psa_websocket_constants.h"
#include "pubsub_websocket_common.h"

#include <uuid/uuid.h>
#include <pubsub_admin_metrics.h>
#include <http_admin/api.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37
#endif


#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

typedef struct pubsub_websocket_rcv_buffer {
    celix_thread_mutex_t mutex;
    celix_array_list_t *list;     //List of received websocket messages (type: pubsub_websocket_msg_t *)
    celix_array_list_t *rcvTimes; //Corresponding receive times of the received websocket messages (rcvTimes[i] -> list[i])
} pubsub_websocket_rcv_buffer_t;

struct pubsub_websocket_topic_receiver {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *uri;
    bool metricsEnabled;

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
    pubsub_websocket_rcv_buffer_t *recvBuffer;
    char *key; //host:port
    char *socketAddress;
    long socketPort;
    char *uri;
    struct mg_connection *sockConnection;
    int connectRetryCount;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_websocket_requested_connection_entry_t;

typedef struct psa_websocket_subscriber_metrics_entry_t {
    unsigned int msgTypeId;
    uuid_t origin;

    unsigned long nrOfMessagesReceived;
    unsigned long nrOfSerializationErrors;
    struct timespec lastMessageReceived;
    double averageTimeBetweenMessagesInSeconds;
    double averageSerializationTimeInSeconds;
    double averageDelayInSeconds;
    double maxDelayInSeconds;
    double minDelayInSeconds;
    unsigned int lastSeqNr;
    unsigned long nrOfMissingSeqNumbers;
} psa_websocket_subscriber_metrics_entry_t;

typedef struct psa_websocket_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //map from serializer svc
    hash_map_t *metrics; //key = msg type id, value = hash_map (key = origin uuid, value = psa_websocket_subscriber_metrics_entry_t*
    pubsub_subscriber_t *svc;
    bool initialized; //true if the init function is called through the receive thread
} psa_websocket_subscriber_entry_t;


static void pubsub_websocketTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_websocketTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void* psa_websocket_recvThread(void * data);
static void psa_websocket_connectToAllRequestedConnections(pubsub_websocket_topic_receiver_t *receiver);
static void psa_websocket_initializeAllSubscribers(pubsub_websocket_topic_receiver_t *receiver);

static int psa_websocketTopicReceiver_data(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle);
static void psa_websocketTopicReceiver_close(const struct mg_connection *connection, void *handle);


pubsub_websocket_topic_receiver_t* pubsub_websocketTopicReceiver_create(celix_bundle_context_t *ctx,
                                                              log_helper_t *logHelper,
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
    receiver->scope = strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    psa_websocket_setScopeAndTopicFilter(scope, topic, receiver->scopeAndTopicFilter);
    receiver->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_WEBSOCKET_METRICS_ENABLED, PSA_WEBSOCKET_DEFAULT_METRICS_ENABLED);

    receiver->uri = psa_websocket_createURI(scope, topic);

    if (receiver->uri != NULL) {
        celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
        celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
        celixThreadMutex_create(&receiver->recvThread.mutex, NULL);
        celixThreadMutex_create(&receiver->recvBuffer.mutex, NULL);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        arrayList_create(&receiver->recvBuffer.list);
        arrayList_create(&receiver->recvBuffer.rcvTimes);
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


    const char *staticConnects = celix_properties_get(topicProperties, PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES, NULL);
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
                entry->recvBuffer = &receiver->recvBuffer;
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
        snprintf(name, 64, "WEBSOCKET TR %s/%s", scope, topic);
        celixThread_setName(&receiver->recvThread.thread, name);
    }

    if (receiver->uri == NULL) {
        free(receiver->scope);
        free(receiver->topic);
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_WEBSOCKET] Cannot create TopicReceiver for %s/%s", scope, topic);
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

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL)  {
                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
                while (hashMapIterator_hasNext(&iter2)) {
                    hash_map_t *origins = hashMapIterator_nextValue(&iter2);
                    hashMap_destroy(origins, true, true);
                }
                hashMap_destroy(entry->metrics, false, false);

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
            pubsub_websocket_msg_t *msg = celix_arrayList_get(receiver->recvBuffer.list, msgBufSize - 1);
            free(msg);
            msgBufSize--;
        }
        celix_arrayList_destroy(receiver->recvBuffer.list);

        int rcvTimesSize = celix_arrayList_size(receiver->recvBuffer.rcvTimes);
        while(rcvTimesSize > 0) {
            struct timespec *time = celix_arrayList_get(receiver->recvBuffer.rcvTimes, rcvTimesSize - 1);
            free(time);
            rcvTimesSize--;
        }
        celix_arrayList_destroy(receiver->recvBuffer.rcvTimes);

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

long pubsub_websocketTopicReceiver_serializerSvcId(pubsub_websocket_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

void pubsub_websocketTopicReceiver_listConnections(pubsub_websocket_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        char *url = NULL;
        asprintf(&url, "%s%s", entry->uri, entry->statically ? " (static)" : "");
        if (entry->connected) {
            celix_arrayList_add(connectedUrls, url);
        } else {
            celix_arrayList_add(unconnectedUrls, url);
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}


void pubsub_websocketTopicReceiver_connectTo(pubsub_websocket_topic_receiver_t *receiver, const char *socketAddress, long socketPort, const char *uri) {
    L_DEBUG("[PSA_WEBSOCKET] TopicReceiver %s/%s connecting to websocket uri %s", receiver->scope, receiver->topic, uri);

    char *key = NULL;
    asprintf(&key, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_websocket_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->key = key;
        entry->uri = strndup(uri, 1024 * 1024);
        entry->socketAddress = strndup(socketAddress, 1024 * 1024);
        entry->socketPort = socketPort;
        entry->connected = false;
        entry->statically = false;
        entry->recvBuffer = &receiver->recvBuffer;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->uri, entry);
        receiver->requestedConnections.allConnected = false;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    psa_websocket_connectToAllRequestedConnections(receiver);
}

void pubsub_websocketTopicReceiver_disconnectFrom(pubsub_websocket_topic_receiver_t *receiver, const char *uri) {
    L_DEBUG("[PSA_WEBSOCKET] TopicReceiver %s/%s disconnect from websocket uri %s", receiver->scope, receiver->topic, uri);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_websocket_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, uri);
    if (entry != NULL && entry->connected) {
        mg_close_connection(entry->sockConnection);
        L_WARN("[PSA_WEBSOCKET] Error disconnecting from websocket uri %s.", uri);
    }
    if (entry != NULL) {
        free(entry->socketAddress);
        free(entry->uri);
        free(entry->key);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_websocketTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_websocket_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
        //not the same scope. ignore
        return;
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
            entry->metrics = hashMap_create(NULL, NULL, NULL, NULL);
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                pubsub_msg_serializer_t *msgSer = hashMapIterator_nextValue(&iter);
                hash_map_t *origins = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
                hashMap_put(entry->metrics, (void*)(uintptr_t)msgSer->msgId, origins);
            }
        }

        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_WEBSOCKET] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
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
            L_ERROR("[PSA_WEBSOCKET] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
        hash_map_iterator_t iter = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_t *origins = hashMapIterator_nextValue(&iter);
            hashMap_destroy(origins, true, true);
        }
        hashMap_destroy(entry->metrics, false, false);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static inline void processMsgForSubscriberEntry(pubsub_websocket_topic_receiver_t *receiver, psa_websocket_subscriber_entry_t* entry, const pubsub_websocket_msg_header_t *hdr, const void* payload, size_t payloadSize, struct timespec *receiveTime) {
    //NOTE receiver->subscribers.mutex locked
    pubsub_msg_serializer_t* msgSer = hashMap_get(entry->msgTypes, (void*)(uintptr_t)(hdr->type));
    pubsub_subscriber_t *svc = entry->svc;
    bool monitor = receiver->metricsEnabled;

    //monitoring
    struct timespec beginSer;
    struct timespec endSer;
    int updateReceiveCount = 0;
    int updateSerError = 0;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = psa_websocket_checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &beginSer);
            }
            celix_status_t status = msgSer->deserialize(msgSer->handle, payload, payloadSize, &deserializedMsg);
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &endSer);
            }
            if (status == CELIX_SUCCESS) {
                bool release = true;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, &release);
                if (release) {
                    msgSer->freeMsg(msgSer->handle, deserializedMsg);
                }
                updateReceiveCount += 1;
            } else {
                updateSerError += 1;
                L_WARN("[PSA_WEBSOCKET_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_WEBSOCKET_TR] Cannot find serializer for type id 0x%X", hdr->type);
    }

    if (msgSer != NULL && monitor) {
        hash_map_t *origins = hashMap_get(entry->metrics, (void*)(uintptr_t )hdr->type);
        char uuidStr[UUID_STR_LEN+1];
        uuid_unparse(hdr->originUUID, uuidStr);
        psa_websocket_subscriber_metrics_entry_t *metrics = hashMap_get(origins, uuidStr);

        if (metrics == NULL) {
            metrics = calloc(1, sizeof(*metrics));
            hashMap_put(origins, strndup(uuidStr, UUID_STR_LEN+1), metrics);
            uuid_copy(metrics->origin, hdr->originUUID);
            metrics->msgTypeId = hdr->type;
            metrics->maxDelayInSeconds = -INFINITY;
            metrics->minDelayInSeconds = INFINITY;
            metrics->lastSeqNr = 0;
        }

        double diff = celix_difftime(&beginSer, &endSer);
        long n = metrics->nrOfMessagesReceived;
        metrics->averageSerializationTimeInSeconds = (metrics->averageSerializationTimeInSeconds * n + diff) / (n+1);

        diff = celix_difftime(&metrics->lastMessageReceived, receiveTime);
        n = metrics->nrOfMessagesReceived;
        if (metrics->nrOfMessagesReceived >= 1) {
            metrics->averageTimeBetweenMessagesInSeconds = (metrics->averageTimeBetweenMessagesInSeconds * n + diff) / (n + 1);
        }
        metrics->lastMessageReceived = *receiveTime;


        int incr = hdr->seqNr - metrics->lastSeqNr;
        if (metrics->lastSeqNr >0 && incr > 1) {
            metrics->nrOfMissingSeqNumbers += (incr - 1);
            L_WARN("Missing message seq nr went from %i to %i", metrics->lastSeqNr, hdr->seqNr);
        }
        metrics->lastSeqNr = hdr->seqNr;

        struct timespec sendTime;
        sendTime.tv_sec = (time_t)hdr->sendtimeSeconds;
        sendTime.tv_nsec = (long)hdr->sendTimeNanoseconds; //TODO FIXME the tv_nsec is not correct
        diff = celix_difftime(&sendTime, receiveTime);
        metrics->averageDelayInSeconds = (metrics->averageDelayInSeconds * n + diff) / (n+1);
        if (diff < metrics->minDelayInSeconds) {
            metrics->minDelayInSeconds = diff;
        }
        if (diff > metrics->maxDelayInSeconds) {
            metrics->maxDelayInSeconds = diff;
        }

        metrics->nrOfMessagesReceived += updateReceiveCount;
        metrics->nrOfSerializationErrors += updateSerError;
    }
}

static inline void processMsg(pubsub_websocket_topic_receiver_t *receiver, const pubsub_websocket_msg_header_t *hdr, const char *payload, size_t payloadSize, struct timespec *receiveTime) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            processMsgForSubscriberEntry(receiver, entry, hdr, payload, payloadSize, receiveTime);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
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
            pubsub_websocket_msg_t *msg = (pubsub_websocket_msg_t *) celix_arrayList_get(receiver->recvBuffer.list, 0);
            struct timespec *rcvTime = (struct timespec *) celix_arrayList_get(receiver->recvBuffer.rcvTimes, 0);
            celixThreadMutex_unlock(&receiver->recvBuffer.mutex);

            processMsg(receiver, &msg->header, msg->payload, msg->payloadSize, rcvTime);
            free(msg);
            free(rcvTime);

            celixThreadMutex_lock(&receiver->recvBuffer.mutex);
            celix_arrayList_removeAt(receiver->recvBuffer.list, 0);
            celix_arrayList_removeAt(receiver->recvBuffer.rcvTimes, 0);
            celixThreadMutex_unlock(&receiver->recvBuffer.mutex);
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

static int psa_websocketTopicReceiver_data(struct mg_connection *connection __attribute__((unused)),
                                            int op_code __attribute__((unused)),
                                            char *data,
                                            size_t length,
                                            void *handle) {
    //Received a websocket message, append this message to the buffer of the receiver.
    if (handle != NULL) {
        psa_websocket_requested_connection_entry_t *entry = (psa_websocket_requested_connection_entry_t *) handle;

        pubsub_websocket_msg_t *rcvdMsg = malloc(length);
        memcpy(rcvdMsg, data, length);

        //Check if payload is completely received
        unsigned long rcvdPayloadSize = length - sizeof(rcvdMsg->header) - sizeof(rcvdMsg->payloadSize);
        if(rcvdMsg->payloadSize == rcvdPayloadSize) {
            celixThreadMutex_lock(&entry->recvBuffer->mutex);
            celix_arrayList_add(entry->recvBuffer->list, rcvdMsg);
            struct timespec *receiveTime = malloc(sizeof(*receiveTime));
            clock_gettime(CLOCK_REALTIME, receiveTime);
            celix_arrayList_add(entry->recvBuffer->rcvTimes, receiveTime);
            celixThreadMutex_unlock(&entry->recvBuffer->mutex);
        } else {
            free(rcvdMsg);
        }
    }

    return 1; //keep open (non-zero), 0 to close the socket
}

static void psa_websocketTopicReceiver_close(const struct mg_connection *connection __attribute__((unused)), void *handle) {
    //Reset connection for this receiver entry
    if (handle != NULL) {
        psa_websocket_requested_connection_entry_t *entry = (psa_websocket_requested_connection_entry_t *) handle;
        entry->connected = false;
        entry->sockConnection = NULL;
    }
}

pubsub_admin_receiver_metrics_t* pubsub_websocketTopicReceiver_metrics(pubsub_websocket_topic_receiver_t *receiver) {
    pubsub_admin_receiver_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->topic);

    size_t msgTypesCount = 0;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter2)) {
            hashMapIterator_nextValue(&iter2);
            msgTypesCount += 1;
        }
    }

    result->nrOfMsgTypes = (unsigned long)msgTypesCount;
    result->msgTypes = calloc(msgTypesCount, sizeof(*result->msgTypes));
    int i = 0;
    iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter2)) {
            hash_map_t *origins = hashMapIterator_nextValue(&iter2);
            result->msgTypes[i].origins = calloc((size_t)hashMap_size(origins), sizeof(*(result->msgTypes[i].origins)));
            result->msgTypes[i].nrOfOrigins = hashMap_size(origins);
            int k = 0;
            hash_map_iterator_t iter3 = hashMapIterator_construct(origins);
            while (hashMapIterator_hasNext(&iter3)) {
                psa_websocket_subscriber_metrics_entry_t *metrics = hashMapIterator_nextValue(&iter3);
                result->msgTypes[i].typeId = metrics->msgTypeId;
                pubsub_msg_serializer_t *msgSer = hashMap_get(entry->msgTypes, (void*)(uintptr_t)metrics->msgTypeId);
                if (msgSer) {
                    snprintf(result->msgTypes[i].typeFqn, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", msgSer->msgName);
                    uuid_copy(result->msgTypes[i].origins[k].originUUID, metrics->origin);
                    result->msgTypes[i].origins[k].nrOfMessagesReceived = metrics->nrOfMessagesReceived;
                    result->msgTypes[i].origins[k].nrOfSerializationErrors = metrics->nrOfSerializationErrors;
                    result->msgTypes[i].origins[k].averageDelayInSeconds = metrics->averageDelayInSeconds;
                    result->msgTypes[i].origins[k].maxDelayInSeconds = metrics->maxDelayInSeconds;
                    result->msgTypes[i].origins[k].minDelayInSeconds = metrics->minDelayInSeconds;
                    result->msgTypes[i].origins[k].averageTimeBetweenMessagesInSeconds = metrics->averageTimeBetweenMessagesInSeconds;
                    result->msgTypes[i].origins[k].averageSerializationTimeInSeconds = metrics->averageSerializationTimeInSeconds;
                    result->msgTypes[i].origins[k].lastMessageReceived = metrics->lastMessageReceived;
                    result->msgTypes[i].origins[k].nrOfMissingSeqNumbers = metrics->nrOfMissingSeqNumbers;

                    k += 1;
                } else {
                    L_WARN("[PSA_WEBSOCKET]: Error cannot find key 0x%X in msg map during metrics collection!\n", metrics->msgTypeId);
                }
            }
            i +=1 ;
        }
    }

    celixThreadMutex_unlock(&receiver->subscribers.mutex);

    return result;
}


static void psa_websocket_connectToAllRequestedConnections(pubsub_websocket_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->connected) {
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
                                                                    entry);
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
