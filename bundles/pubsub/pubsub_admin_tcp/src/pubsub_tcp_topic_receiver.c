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
#include <stdint.h>
#include <unistd.h>
#include <pubsub/subscriber.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <arpa/inet.h>
#include <celix_log_helper.h>
#include "pubsub_tcp_handler.h"
#include "pubsub_tcp_topic_receiver.h"
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_common.h"

#include <uuid/uuid.h>
#include <pubsub_admin_metrics.h>
#include <pubsub_utils.h>
#include "pubsub_interceptors_handler.h"
#include <celix_api.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN  37
#endif

#define L_DEBUG(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_tcp_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    char *scope;
    char *topic;
    size_t timeout;
    bool metricsEnabled;
    bool isPassive;
    pubsub_tcpHandler_t *socketHandler;
    pubsub_tcpHandler_t *sharedSocketHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } thread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = tcp url, value = psa_tcp_requested_connection_entry_t*
        bool allConnected; //true if all requestedConnection are connected
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_tcp_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_tcp_requested_connection_entry {
    pubsub_tcp_topic_receiver_t *parent;
    char *url;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_tcp_requested_connection_entry_t;

typedef struct psa_tcp_subscriber_metrics_entry_t {
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
    unsigned long nrOfMissingSeqNumbers;
} psa_tcp_subscriber_metrics_entry_t;

typedef struct psa_tcp_subscriber_entry {
    hash_map_t *msgTypes; //map from serializer svc
    hash_map_t *metrics; //key = msg type id, value = hash_map (key = origin uuid, value = psa_tcp_subscriber_metrics_entry_t*
    hash_map_t *subscriberServices; //key = servide id, value = pubsub_subscriber_t*
    bool initialized; //true if the init function is called through the receive thread
} psa_tcp_subscriber_entry_t;

static void pubsub_tcpTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_tcpTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void *psa_tcp_recvThread(void *data);
static void psa_tcp_connectToAllRequestedConnections(pubsub_tcp_topic_receiver_t *receiver);
static void psa_tcp_initializeAllSubscribers(pubsub_tcp_topic_receiver_t *receiver);
static void processMsg(void *handle, const pubsub_protocol_message_t *hdr, bool *release, struct timespec *receiveTime);
static void psa_tcp_connectHandler(void *handle, const char *url, bool lock);
static void psa_tcp_disConnectHandler(void *handle, const char *url, bool lock);
static bool psa_tcp_checkVersion(version_pt msgVersion, uint16_t major, uint16_t minor);

pubsub_tcp_topic_receiver_t *pubsub_tcpTopicReceiver_create(celix_bundle_context_t *ctx,
                                                            celix_log_helper_t *logHelper,
                                                            const char *scope,
                                                            const char *topic,
                                                            const celix_properties_t *topicProperties,
                                                            pubsub_tcp_endPointStore_t *handlerStore,
                                                            long serializerSvcId,
                                                            pubsub_serializer_service_t *serializer,
                                                            long protocolSvcId,
                                                            pubsub_protocol_service_t *protocol) {
    pubsub_tcp_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    receiver->protocolSvcId = protocolSvcId;
    receiver->protocol = protocol;
    receiver->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    pubsubInterceptorsHandler_create(ctx, scope, topic, &receiver->interceptorsHandler);
    const char *staticConnectUrls = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_STATIC_CONNECT_URLS_FOR, topic, scope);
    const char *isPassive = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_PASSIVE_ENABLED, topic, scope);
    const char *passiveKey = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_PASSIVE_SELECTION_KEY, topic, scope);

    if (isPassive) {
        receiver->isPassive = psa_tcp_isPassive(isPassive);
    }
    if (topicProperties != NULL) {
        if(staticConnectUrls == NULL) {
            staticConnectUrls = celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_CONNECT_URLS, NULL);
        }
        if (isPassive == NULL) {
            receiver->isPassive = celix_properties_getAsBool(topicProperties, PUBSUB_TCP_PASSIVE_CONFIGURED, false);
        }
        if (passiveKey == NULL) {
            passiveKey = celix_properties_get(topicProperties, PUBSUB_TCP_PASSIVE_KEY, NULL);
        }
    }

    // Set receiver connection thread timeout.
    // property is in ms, timeout value in us. (convert ms to us).
    receiver->timeout = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_SUBSCRIBER_CONNECTION_TIMEOUT,
                                                              PSA_TCP_SUBSCRIBER_CONNECTION_DEFAULT_TIMEOUT) * 1000;
    /* When it's an endpoint share the socket with the sender */
    if (passiveKey != NULL) {
        celixThreadMutex_lock(&handlerStore->mutex);
        pubsub_tcpHandler_t *entry = hashMap_get(handlerStore->map, passiveKey);
        if (entry == NULL) {
            if (receiver->socketHandler == NULL)
                receiver->socketHandler = pubsub_tcpHandler_create(receiver->protocol, receiver->logHelper);
            entry = receiver->socketHandler;
            receiver->sharedSocketHandler = receiver->socketHandler;
            hashMap_put(handlerStore->map, (void *) passiveKey, entry);
        } else {
            receiver->socketHandler = entry;
            receiver->sharedSocketHandler = entry;
        }
        celixThreadMutex_unlock(&handlerStore->mutex);
    } else {
        receiver->socketHandler = pubsub_tcpHandler_create(receiver->protocol, receiver->logHelper);
    }

    if (receiver->socketHandler != NULL) {
        long prio = celix_properties_getAsLong(topicProperties, PUBSUB_TCP_THREAD_REALTIME_PRIO, -1L);
        const char *sched = celix_properties_get(topicProperties, PUBSUB_TCP_THREAD_REALTIME_SCHED, NULL);
        long retryCnt = celix_properties_getAsLong(topicProperties, PUBSUB_TCP_SUBSCRIBER_RETRY_CNT_KEY,
                                                   PUBSUB_TCP_SUBSCRIBER_RETRY_CNT_DEFAULT);
        double rcvTimeout = celix_properties_getAsDouble(topicProperties, PUBSUB_TCP_SUBSCRIBER_RCVTIMEO_KEY, PUBSUB_TCP_SUBSCRIBER_RCVTIMEO_DEFAULT);
        long bufferSize = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_RECV_BUFFER_SIZE,
                                                                 PSA_TCP_DEFAULT_RECV_BUFFER_SIZE);
        long timeout = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_TIMEOUT, PSA_TCP_DEFAULT_TIMEOUT);

        pubsub_tcpHandler_setThreadName(receiver->socketHandler, topic, scope);
        pubsub_tcpHandler_setReceiveBufferSize(receiver->socketHandler, (unsigned int) bufferSize);
        pubsub_tcpHandler_setTimeout(receiver->socketHandler, (unsigned int) timeout);
        pubsub_tcpHandler_addMessageHandler(receiver->socketHandler, receiver, processMsg);
        pubsub_tcpHandler_addReceiverConnectionCallback(receiver->socketHandler, receiver, psa_tcp_connectHandler,
                                                        psa_tcp_disConnectHandler);
        pubsub_tcpHandler_setThreadPriority(receiver->socketHandler, prio, sched);
        pubsub_tcpHandler_setReceiveRetryCnt(receiver->socketHandler, (unsigned int) retryCnt);
        pubsub_tcpHandler_setReceiveTimeOut(receiver->socketHandler, rcvTimeout);
    }
    receiver->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_TCP_METRICS_ENABLED,
                                                                          PSA_TCP_DEFAULT_METRICS_ENABLED);
    celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
    celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
    celixThreadMutex_create(&receiver->thread.mutex, NULL);

    receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
    receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    if ((staticConnectUrls != NULL) && (receiver->socketHandler != NULL) && (!receiver->isPassive)) {
        char *urlsCopy = strndup(staticConnectUrls, 1024 * 1024);
        char *url;
        char *save = urlsCopy;
        while ((url = strtok_r(save, " ", &save))) {
            psa_tcp_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
            entry->statically = true;
            entry->connected = false;
            entry->url = strndup(url, 1024 * 1024);
            entry->parent = receiver;
            hashMap_put(receiver->requestedConnections.map, entry->url, entry);
            receiver->requestedConnections.allConnected = false;
        }
        free(urlsCopy);
    }

    if (receiver->socketHandler != NULL && (!receiver->isPassive)) {
        // Configure Receiver thread
        receiver->thread.running = true;
        celixThread_create(&receiver->thread.thread, NULL, psa_tcp_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "TCP TR %s/%s", scope == NULL ? "(null)" : scope, topic);
        celixThread_setName(&receiver->thread.thread, name);
    }

    //track subscribers
    if (receiver->socketHandler != NULL) {
        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size + 1];
        snprintf(buf, (size_t) size + 1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_tcpTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_tcpTopicReceiver_removeSubscriber;
        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    if (receiver->socketHandler == NULL) {
        if (receiver->scope != NULL) {
            free(receiver->scope);
        }
        free(receiver->topic);
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_TCP] Cannot create TopicReceiver for %s/%s", scope == NULL ? "(null)" : scope, topic);
    }
    return receiver;
}

void pubsub_tcpTopicReceiver_destroy(pubsub_tcp_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        celixThreadMutex_lock(&receiver->thread.mutex);
        if (receiver->thread.running) {
            receiver->thread.running = false;
            celixThreadMutex_unlock(&receiver->thread.mutex);
            celixThread_join(receiver->thread.thread, NULL);
        }

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                hashMap_destroy(entry->subscriberServices, false, false);
                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
                while (hashMapIterator_hasNext(&iter2)) {
                    hash_map_t *origins = hashMapIterator_nextValue(&iter2);
                    hashMap_destroy(origins, true, true);
                }
                hashMap_destroy(entry->metrics, false, false);
                free(entry);
            }
        }
        hashMap_destroy(receiver->subscribers.map, false, false);

        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                free(entry->url);
                free(entry);
            }
        }
        hashMap_destroy(receiver->requestedConnections.map, false, false);
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_destroy(&receiver->subscribers.mutex);
        celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        celixThreadMutex_destroy(&receiver->thread.mutex);

        pubsub_tcpHandler_addMessageHandler(receiver->socketHandler, NULL, NULL);
        pubsub_tcpHandler_addReceiverConnectionCallback(receiver->socketHandler, NULL, NULL, NULL);
        if ((receiver->socketHandler) && (receiver->sharedSocketHandler == NULL)) {
            pubsub_tcpHandler_destroy(receiver->socketHandler);
            receiver->socketHandler = NULL;
        }
        pubsubInterceptorsHandler_destroy(receiver->interceptorsHandler);
        if (receiver->scope != NULL) {
            free(receiver->scope);
        }
        free(receiver->topic);
    }
    free(receiver);
}

const char *pubsub_tcpTopicReceiver_scope(pubsub_tcp_topic_receiver_t *receiver) {
    return receiver->scope;
}

const char *pubsub_tcpTopicReceiver_topic(pubsub_tcp_topic_receiver_t *receiver) {
    return receiver->topic;
}

long pubsub_tcpTopicReceiver_serializerSvcId(pubsub_tcp_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

long pubsub_tcpTopicReceiver_protocolSvcId(pubsub_tcp_topic_receiver_t *receiver) {
    return receiver->protocolSvcId;
}

void pubsub_tcpTopicReceiver_listConnections(pubsub_tcp_topic_receiver_t *receiver, celix_array_list_t *connectedUrls,
                                             celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (receiver->isPassive) {
        char* interface_url = pubsub_tcpHandler_get_interface_url(receiver->socketHandler);
        char *url = NULL;
        asprintf(&url, "%s (passive)", interface_url ? interface_url : "");
        if (interface_url) {
            celix_arrayList_add(connectedUrls, url);
        } else {
            celix_arrayList_add(unconnectedUrls, url);
        }
        free(interface_url);
    } else {
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            char *url = NULL;
            asprintf(&url, "%s%s", entry->url, entry->statically ? " (static)" : "");
            if (entry->connected) {
                celix_arrayList_add(connectedUrls, url);
            } else {
                celix_arrayList_add(unconnectedUrls, url);
            }
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

bool pubsub_tcpTopicReceiver_isPassive(pubsub_tcp_topic_receiver_t *receiver) {
    return receiver->isPassive;
}

void pubsub_tcpTopicReceiver_connectTo(
    pubsub_tcp_topic_receiver_t *receiver,
    const char *url) {
    L_DEBUG("[PSA_TCP] TopicReceiver %s/%s connecting to tcp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_tcp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->url = strndup(url, 1024 * 1024);
        entry->connected = false;
        entry->statically = false;
        entry->parent = receiver;
        hashMap_put(receiver->requestedConnections.map, (void *) entry->url, entry);
        receiver->requestedConnections.allConnected = false;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    psa_tcp_connectToAllRequestedConnections(receiver);
}

void pubsub_tcpTopicReceiver_disconnectFrom(pubsub_tcp_topic_receiver_t *receiver, const char *url) {
    L_DEBUG("[PSA TCP] TopicReceiver %s/%s disconnect from tcp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_tcp_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, url);
    if (entry != NULL) {
        int rc = pubsub_tcpHandler_disconnect(receiver->socketHandler, entry->url);
        if (rc < 0)
            L_WARN("[PSA_TCP] Error disconnecting from tcp url %s. (%s)", url, strerror(errno));
    }
    if (entry != NULL) {
        free(entry->url);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_tcpTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props,
                                                  const celix_bundle_t *bnd) {
    pubsub_tcp_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, NULL);
    if (receiver->scope == NULL) {
        if (subScope != NULL) {
            return;
        }
    } else if (subScope != NULL) {
        if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
            //not the same scope. ignore
            return;
        }
    } else {
        //receiver scope is not NULL, but subScope is NULL -> ignore
        return;
    }

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_tcp_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void *) bndId);
    if (entry != NULL) {
        hashMap_put(entry->subscriberServices, (void*)svcId, svc);
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->subscriberServices = hashMap_create(NULL, NULL, NULL, NULL);
        entry->initialized = false;
        receiver->subscribers.allInitialized = false;

        hashMap_put(entry->subscriberServices, (void*)svcId, svc);

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t *) bnd, &entry->msgTypes);

        if (rc == 0) {
            entry->metrics = hashMap_create(NULL, NULL, NULL, NULL);
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                pubsub_msg_serializer_t *msgSer = hashMapIterator_nextValue(&iter);
                hash_map_t *origins = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
                hashMap_put(entry->metrics, (void *) (uintptr_t) msgSer->msgId, origins);
            }
        }

        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void *) bndId, entry);
        } else {
            L_ERROR("[PSA_TCP] Cannot create msg serializer map for TopicReceiver %s/%s",
                    receiver->scope == NULL ? "(null)" : receiver->scope,
                    receiver->topic);
            free(entry);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_tcpTopicReceiver_removeSubscriber(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props,
                                                     const celix_bundle_t *bnd) {
    pubsub_tcp_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_tcp_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void *) bndId);
    if (entry != NULL) {
        hashMap_remove(entry->subscriberServices, (void*)svcId);
    }
    if (entry != NULL && hashMap_size(entry->subscriberServices) == 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void *) bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_TCP] Cannot destroy msg serializers map for TopicReceiver %s/%s",
                    receiver->scope == NULL ? "(null)" : receiver->scope,
                    receiver->topic);
        }
        hash_map_iterator_t iter = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_t *origins = hashMapIterator_nextValue(&iter);
            hashMap_destroy(origins, true, true);
        }
        hashMap_destroy(entry->metrics, false, false);
        hashMap_destroy(entry->subscriberServices, false, false);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static inline void
processMsgForSubscriberEntry(pubsub_tcp_topic_receiver_t *receiver, psa_tcp_subscriber_entry_t *entry,
                             const pubsub_protocol_message_t *message, bool *releaseMsg, struct timespec *receiveTime __attribute__((unused))) {
    //NOTE receiver->subscribers.mutex locked
    pubsub_msg_serializer_t *msgSer = hashMap_get(entry->msgTypes, (void *) (uintptr_t) (message->header.msgId));
    bool monitor = receiver->metricsEnabled;

    //monitoring
    struct timespec beginSer;
    struct timespec endSer;
    int updateReceiveCount = 0;
    int updateSerError = 0;

    if (msgSer != NULL) {
        void *deSerializedMsg = NULL;
        bool validVersion = psa_tcp_checkVersion(msgSer->msgVersion, message->header.msgMajorVersion,
                                                 message->header.msgMinorVersion);
        if (validVersion) {
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &beginSer);
            }
            struct iovec deSerializeBuffer;
            deSerializeBuffer.iov_base = message->payload.payload;
            deSerializeBuffer.iov_len = message->payload.length;
            celix_status_t status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 1, &deSerializedMsg);
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &endSer);
            }
            // When received payload pointer is the same as deserializedMsg, set ownership of pointer to topic receiver
            if (message->payload.payload == deSerializedMsg) {
                *releaseMsg = true;
            }

            if (status == CELIX_SUCCESS) {
                const char *msgType = msgSer->msgName;
                uint32_t msgId = message->header.msgId;
                celix_properties_t *metadata = message->metadata.metadata;
                bool cont = pubsubInterceptorHandler_invokePreReceive(receiver->interceptorsHandler, msgType, msgId, deSerializedMsg, &metadata);
                bool release = true;
                if (cont) {
                    hash_map_iterator_t iter = hashMapIterator_construct(entry->subscriberServices);
                    while (hashMapIterator_hasNext(&iter)) {
                        pubsub_subscriber_t *svc = hashMapIterator_nextValue(&iter);
                        svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deSerializedMsg, message->metadata.metadata, &release);
                        pubsubInterceptorHandler_invokePostReceive(receiver->interceptorsHandler, msgType, msgId, deSerializedMsg, metadata);
                        if (!release && hashMapIterator_hasNext(&iter)) {
                            //receive function has taken ownership and still more receive function to come ..
                            //deserialize again for new message
                            status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 1, &deSerializedMsg);
                            if (status != CELIX_SUCCESS) {
                                L_WARN("[PSA_TCP_TR] Cannot deserialize msg type %s for scope/topic %s/%s",
                                       msgSer->msgName,
                                       receiver->scope == NULL ? "(null)" : receiver->scope,
                                       receiver->topic);
                                break;
                            }
                            release = true;
                        }
                    }
                    if (release) {
                        msgSer->freeDeserializeMsg(msgSer->handle, deSerializedMsg);
                    }
                    if (message->metadata.metadata) {
                        celix_properties_destroy(message->metadata.metadata);
                    }
                    updateReceiveCount += 1;
                }
            } else {
                updateSerError += 1;
                L_WARN("[PSA_TCP_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName,
                       receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_TCP_TR] Cannot find serializer for type id 0x%X. Received payload size is %u.", message->header.msgId, message->payload.length);
    }
}

static void
processMsg(void *handle, const pubsub_protocol_message_t *message, bool *release, struct timespec *receiveTime) {
    pubsub_tcp_topic_receiver_t *receiver = handle;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            processMsgForSubscriberEntry(receiver, entry, message, release, receiveTime);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void *psa_tcp_recvThread(void *data) {
    pubsub_tcp_topic_receiver_t *receiver = data;

    celixThreadMutex_lock(&receiver->thread.mutex);
    bool running = receiver->thread.running;
    celixThreadMutex_unlock(&receiver->thread.mutex);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    bool allConnected = receiver->requestedConnections.allConnected;
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    bool allInitialized = receiver->subscribers.allInitialized;
    celixThreadMutex_unlock(&receiver->subscribers.mutex);

    while (running) {
        if (!allConnected) {
            psa_tcp_connectToAllRequestedConnections(receiver);
        }
        if (!allInitialized) {
            psa_tcp_initializeAllSubscribers(receiver);
        }
        usleep(receiver->timeout);

        celixThreadMutex_lock(&receiver->thread.mutex);
        running = receiver->thread.running;
        celixThreadMutex_unlock(&receiver->thread.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        allConnected = receiver->requestedConnections.allConnected;
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        allInitialized = receiver->subscribers.allInitialized;
        celixThreadMutex_unlock(&receiver->subscribers.mutex);
    } // while
    return NULL;
}

pubsub_admin_receiver_metrics_t *pubsub_tcpTopicReceiver_metrics(pubsub_tcp_topic_receiver_t *receiver) {
    pubsub_admin_receiver_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->scope == NULL ? PUBSUB_DEFAULT_ENDPOINT_SCOPE : receiver->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->topic);

    int msgTypesCount = 0;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter2)) {
            hashMapIterator_nextValue(&iter2);
            msgTypesCount += 1;
        }
    }

    result->nrOfMsgTypes = (unsigned long) msgTypesCount;
    result->msgTypes = calloc(msgTypesCount, sizeof(*result->msgTypes));
    int i = 0;
    iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter2)) {
            hash_map_t *origins = hashMapIterator_nextValue(&iter2);
            result->msgTypes[i].origins = calloc((size_t) hashMap_size(origins), sizeof(*(result->msgTypes[i].origins)));
            result->msgTypes[i].nrOfOrigins = hashMap_size(origins);
            int k = 0;
            hash_map_iterator_t iter3 = hashMapIterator_construct(origins);
            while (hashMapIterator_hasNext(&iter3)) {
                psa_tcp_subscriber_metrics_entry_t *metrics = hashMapIterator_nextValue(&iter3);
                result->msgTypes[i].typeId = metrics->msgTypeId;
                pubsub_msg_serializer_t *msgSer = hashMap_get(entry->msgTypes, (void *) (uintptr_t) metrics->msgTypeId);
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
                    L_WARN("[PSA_TCP]: Error cannot find key 0x%X in msg map during metrics collection!\n",
                           metrics->msgTypeId);
                }
            }
            i += 1;
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
    return result;
}

static void psa_tcp_connectToAllRequestedConnections(pubsub_tcp_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if ((entry) && (!entry->connected) && (!receiver->isPassive)) {
                int rc = pubsub_tcpHandler_connect(entry->parent->socketHandler, entry->url);
                if (rc < 0) {
                    allConnected = false;
                }
            }
        }
        receiver->requestedConnections.allConnected = allConnected;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_tcp_connectHandler(void *handle, const char *url, bool lock) {
    pubsub_tcp_topic_receiver_t *receiver = handle;
    L_DEBUG("[PSA_TCP] TopicReceiver %s/%s connecting to tcp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);
    if (lock)
        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_tcp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->parent = receiver;
        entry->url = strndup(url, 1024 * 1024);
        entry->statically = true;
        hashMap_put(receiver->requestedConnections.map, (void *) entry->url, entry);
        receiver->requestedConnections.allConnected = false;
    }
    entry->connected = true;
    if (lock)
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_tcp_disConnectHandler(void *handle, const char *url, bool lock) {
    pubsub_tcp_topic_receiver_t *receiver = handle;
    L_DEBUG("[PSA TCP] TopicReceiver %s/%s disconnect from tcp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);
    if (lock)
        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_tcp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry != NULL) {
        entry->connected = false;
        receiver->requestedConnections.allConnected = false;
    }
    if (lock)
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_tcp_initializeAllSubscribers(pubsub_tcp_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    if (!receiver->subscribers.allInitialized) {
        bool allInitialized = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->initialized) {
                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->subscriberServices);
                while (hashMapIterator_hasNext(&iter2)) {
                    pubsub_subscriber_t *svc = hashMapIterator_nextValue(&iter2);
                    int rc = 0;
                    if (svc != NULL && svc->init != NULL) {
                        rc = svc->init(svc->handle);
                    }
                    if (rc == 0) {
                        //note now only initialized on first subscriber entries added.
                        entry->initialized = true;
                    } else {
                        L_WARN("Cannot initialize subscriber svc. Got rc %i", rc);
                        allInitialized = false;
                    }
                }
            }
        }
        receiver->subscribers.allInitialized = allInitialized;
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static bool psa_tcp_checkVersion(version_pt msgVersion, uint16_t major, uint16_t minor) {
    bool check = false;

    if (major == 0 && minor == 0) {
        //no check
        return true;
    }

    int versionMajor;
    int versionMinor;
    if (msgVersion!=NULL) {
        version_getMajor(msgVersion, &versionMajor);
        version_getMinor(msgVersion, &versionMinor);
        if (major==((unsigned char)versionMajor)) { /* Different major means incompatible */
            check = (minor>=((unsigned char)versionMinor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
        }
    }

    return check;
}
