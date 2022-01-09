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
#include <arpa/inet.h>
#include <celix_log_helper.h>
#include "pubsub_skt_handler.h"
#include "pubsub_udp_topic_receiver.h"
#include "pubsub_psa_udp_constants.h"

#include <uuid/uuid.h>
#include <pubsub_utils.h>
#include "pubsub_interceptors_handler.h"
#include <celix_api.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN  37
#endif

#define L_TRACE(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_TRACE, __VA_ARGS__)
#define L_DEBUG(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_udp_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    char *scope;
    char *topic;
    char *url;
    pubsub_serializer_handler_t* serializerHandler;
    void *admin;
    size_t timeout;
    bool isPassive;
    bool isStatic;
    pubsub_sktHandler_t *socketHandler;
    pubsub_sktHandler_t *sharedSocketHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } thread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = udp url, value = psa_udp_requested_connection_entry_t*
        bool allConnected; //true if all requestedConnection are connected
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = long svc id, value = psa_udp_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_udp_requested_connection_entry {
    pubsub_udp_topic_receiver_t *parent;
    char *url;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_udp_requested_connection_entry_t;

typedef struct psa_udp_subscriber_entry {
    pubsub_subscriber_t* subscriberSvc;
    bool initialized; //true if the init function is called through the receive thread
} psa_udp_subscriber_entry_t;

static void pubsub_udpTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props);
static void pubsub_udpTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props);
static void *psa_udp_recvThread(void *data);
static void psa_udp_connectToAllRequestedConnections(pubsub_udp_topic_receiver_t *receiver);
static void psa_udp_initializeAllSubscribers(pubsub_udp_topic_receiver_t *receiver);
static void processMsg(void *handle, const pubsub_protocol_message_t *message, bool *release, struct timespec *receiveTime);
static void psa_udp_connectHandler(void *handle, const char *url, bool lock);
static void psa_udp_disConnectHandler(void *handle, const char *url, bool lock);

pubsub_udp_topic_receiver_t *pubsub_udpTopicReceiver_create(celix_bundle_context_t *ctx,
                                                            celix_log_helper_t *logHelper,
                                                            const char *scope,
                                                            const char *topic,
                                                            pubsub_serializer_handler_t* serializerHandler,
                                                            void *admin,
                                                            const celix_properties_t *topicProperties,
                                                            pubsub_sktHandler_endPointStore_t *handlerStore,
                                                            long protocolSvcId,
                                                            pubsub_protocol_service_t *protocol) {
    pubsub_udp_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerHandler = serializerHandler;
    receiver->admin = admin;
    receiver->protocolSvcId = protocolSvcId;
    receiver->protocol = protocol;
    receiver->scope = celix_utils_strdup(scope);
    receiver->topic = celix_utils_strdup(topic);
    receiver->interceptorsHandler = pubsubInterceptorsHandler_create(ctx, scope, topic, PUBSUB_UDP_ADMIN_TYPE,
                                                                     pubsub_serializerHandler_getSerializationType(serializerHandler));
    const char *ip = celix_bundleContext_getProperty(ctx, PUBSUB_UDP_PSA_IP_KEY, NULL);
    const char *discUrl = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDP_STATIC_BIND_URL_FOR, topic, scope);
    const char *staticConnectUrls = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDP_STATIC_CONNECT_URLS_FOR, topic, scope);
    const char *isPassive = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDP_PASSIVE_ENABLED, topic, scope);
    const char *passiveKey = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDP_PASSIVE_SELECTION_KEY, topic, scope);

    if (isPassive) {
        receiver->isPassive = pubsub_sktHandler_isPassive(isPassive);
    }
    if (topicProperties != NULL) {
        if (discUrl == NULL) {
            discUrl = celix_properties_get(topicProperties, PUBSUB_UDP_STATIC_DISCOVER_URL, NULL);
        }
        if(staticConnectUrls == NULL) {
            staticConnectUrls = celix_properties_get(topicProperties, PUBSUB_UDP_STATIC_CONNECT_URLS, NULL);
        }
        if (isPassive == NULL) {
            receiver->isPassive = celix_properties_getAsBool(topicProperties, PUBSUB_UDP_PASSIVE_CONFIGURED, false);
        }
        if (passiveKey == NULL) {
            passiveKey = celix_properties_get(topicProperties, PUBSUB_UDP_PASSIVE_KEY, NULL);
        }
    }

    // Set receiver connection thread timeout.
    // property is in ms, timeout value in us. (convert ms to us).
    receiver->timeout = celix_bundleContext_getPropertyAsLong(ctx, PSA_UDP_SUBSCRIBER_CONNECTION_TIMEOUT,
                                                              PSA_UDP_SUBSCRIBER_CONNECTION_DEFAULT_TIMEOUT) * 1000;
    /* When it's an endpoint share the socket with the sender */
    if (passiveKey != NULL) {
        celixThreadMutex_lock(&handlerStore->mutex);
        pubsub_sktHandler_t *entry = hashMap_get(handlerStore->map, passiveKey);
        if (entry == NULL) {
            if (receiver->socketHandler == NULL)
                receiver->socketHandler = pubsub_sktHandler_create(receiver->protocol, receiver->logHelper);
            entry = receiver->socketHandler;
            receiver->sharedSocketHandler = receiver->socketHandler;
            hashMap_put(handlerStore->map, (void *) passiveKey, entry);
        } else {
            receiver->socketHandler = entry;
            receiver->sharedSocketHandler = entry;
        }
        celixThreadMutex_unlock(&handlerStore->mutex);
    } else {
        receiver->socketHandler = pubsub_sktHandler_create(receiver->protocol, receiver->logHelper);
    }

    if (receiver->socketHandler != NULL) {
        long prio = celix_properties_getAsLong(topicProperties, PUBSUB_UDP_THREAD_REALTIME_PRIO, -1L);
        const char *sched = celix_properties_get(topicProperties, PUBSUB_UDP_THREAD_REALTIME_SCHED, NULL);
        long retryCnt = celix_properties_getAsLong(topicProperties, PUBSUB_UDP_SUBSCRIBER_RETRY_CNT_KEY,
                                                   PUBSUB_UDP_SUBSCRIBER_RETRY_CNT_DEFAULT);
        double rcvTimeout = celix_properties_getAsDouble(topicProperties, PUBSUB_UDP_SUBSCRIBER_RCVTIMEO_KEY, PUBSUB_UDP_SUBSCRIBER_RCVTIMEO_DEFAULT);
        long bufferSize = celix_bundleContext_getPropertyAsLong(ctx, PSA_UDP_RECV_BUFFER_SIZE,
                                                                 PSA_UDP_DEFAULT_RECV_BUFFER_SIZE);
        long timeout = celix_bundleContext_getPropertyAsLong(ctx, PSA_UDP_TIMEOUT, PSA_UDP_DEFAULT_TIMEOUT);

        pubsub_sktHandler_setThreadName(receiver->socketHandler, topic, scope);
        pubsub_sktHandler_setReceiveBufferSize(receiver->socketHandler, (unsigned int) bufferSize);
        pubsub_sktHandler_setTimeout(receiver->socketHandler, (unsigned int) timeout);
        pubsub_sktHandler_addMessageHandler(receiver->socketHandler, receiver, processMsg);
        pubsub_sktHandler_addReceiverConnectionCallback(receiver->socketHandler, receiver, psa_udp_connectHandler,
                                                        psa_udp_disConnectHandler);
        pubsub_sktHandler_setThreadPriority(receiver->socketHandler, prio, sched);
        pubsub_sktHandler_setReceiveRetryCnt(receiver->socketHandler, (unsigned int) retryCnt);
        pubsub_sktHandler_setReceiveTimeOut(receiver->socketHandler, rcvTimeout);
    }
    celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
    celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
    celixThreadMutex_create(&receiver->thread.mutex, NULL);

    receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
    receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    if ((staticConnectUrls != NULL) && (receiver->socketHandler != NULL) && (!receiver->isPassive)) {
        char *urlsCopy = celix_utils_strdup(staticConnectUrls);
        char *url;
        char *save = urlsCopy;
        while ((url = strtok_r(save, " ", &save))) {
            pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
            bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
            bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
            if (is_multicast || is_broadcast) {
                psa_udp_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
                entry->statically = true;
                entry->connected = false;
                entry->url = celix_utils_strdup(url);
                entry->parent = receiver;
                hashMap_put(receiver->requestedConnections.map, entry->url, entry);
                receiver->requestedConnections.allConnected = false;
            }
            pubsub_utils_url_free(url_info);
        }
        free(urlsCopy);
    }

    if (!receiver->isPassive) {
        //setting up tcp socket for UDP TopicReceiver
        char *urls = NULL;
        if (discUrl != NULL) {
            urls = celix_utils_strdup(discUrl);
        } else if (ip != NULL) {
            urls = celix_utils_strdup(ip);
        } else {
            struct sockaddr_in *sin = pubsub_utils_url_getInAddr(NULL, 0);
            urls = pubsub_utils_url_get_url(sin, NULL);
            free(sin);
        }
        if (urls) {
            char *urlsCopy = celix_utils_strdup(urls);
            char *url;
            char *save = urlsCopy;
            while ((url = strtok_r(save, " ", &save))) {
                pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
                bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
                bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
                if (!is_multicast && !is_broadcast) {
                    int rc = pubsub_sktHandler_udp_bind(receiver->socketHandler, url);
                    if (rc < 0) {
                        L_WARN("Error for udp listen using dynamic bind url '%s'. %s", url, strerror(errno));
                    } else {
                        url = NULL;}
                }
                pubsub_utils_url_free(url_info);
            }
            receiver->url = pubsub_sktHandler_get_interface_url(receiver->socketHandler);
            free(urlsCopy);
        }
        free(urls);
    }
    if (receiver->socketHandler != NULL && (!receiver->isPassive)) {
        // Configure Receiver thread
        receiver->thread.running = true;
        celixThread_create(&receiver->thread.thread, NULL, psa_udp_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "UDP TR %s/%s", scope == NULL ? "(null)" : scope, topic);
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
        opts.addWithProperties = pubsub_udpTopicReceiver_addSubscriber;
        opts.removeWithProperties = pubsub_udpTopicReceiver_removeSubscriber;
        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    if (receiver->socketHandler == NULL) {
        if (receiver->scope != NULL) {
            free(receiver->scope);
        }
        free(receiver->topic);
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_udp] Cannot create TopicReceiver for %s/%s", scope == NULL ? "(null)" : scope, topic);
    }
    return receiver;
}

void pubsub_udpTopicReceiver_destroy(pubsub_udp_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        celixThreadMutex_lock(&receiver->thread.mutex);
        if (receiver->thread.running) {
            receiver->thread.running = false;
            celixThreadMutex_unlock(&receiver->thread.mutex);
            celixThread_join(receiver->thread.thread, NULL);
        }

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hashMap_destroy(receiver->subscribers.map, false, true);
        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
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

        pubsub_sktHandler_addMessageHandler(receiver->socketHandler, NULL, NULL);
        pubsub_sktHandler_addReceiverConnectionCallback(receiver->socketHandler, NULL, NULL, NULL);
        if ((receiver->socketHandler) && (receiver->sharedSocketHandler == NULL)) {
            pubsub_sktHandler_destroy(receiver->socketHandler);
            receiver->socketHandler = NULL;
        }
        pubsubInterceptorsHandler_destroy(receiver->interceptorsHandler);
        if (receiver->scope != NULL) {
            free(receiver->scope);
        }
        free(receiver->url);
        free(receiver->topic);
    }
    free(receiver);
}

const char *pubsub_udpTopicReceiver_scope(pubsub_udp_topic_receiver_t *receiver) {
    return receiver->scope;
}

const char *pubsub_udpTopicReceiver_topic(pubsub_udp_topic_receiver_t *receiver) {
    return receiver->topic;
}

const char *pubsub_udpTopicReceiver_serializerType(pubsub_udp_topic_receiver_t *receiver) {
    return pubsub_serializerHandler_getSerializationType(receiver->serializerHandler);
}

long pubsub_udpTopicReceiver_protocolSvcId(pubsub_udp_topic_receiver_t *receiver) {
    return receiver->protocolSvcId;
}

bool pubsub_udpTopicReceiver_isStatic(pubsub_udp_topic_receiver_t *receiver) {
    return receiver->isStatic;
}

bool pubsub_udpTopicReceiver_isPassive(pubsub_udp_topic_receiver_t *receiver) {
    return receiver->isPassive;
}

const char *pubsub_udpTopicReceiver_url(pubsub_udp_topic_receiver_t *receiver) {
    if (receiver->isPassive) {
        return pubsub_sktHandler_get_connection_url(receiver->socketHandler);
    } else {
        return receiver->url;
    }
}


void pubsub_udpTopicReceiver_listConnections(pubsub_udp_topic_receiver_t *receiver, celix_array_list_t *connectedUrls,
                                             celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (receiver->isPassive) {
        char* interface_url = pubsub_sktHandler_get_interface_url(receiver->socketHandler);
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
            psa_udp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
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

void pubsub_udpTopicReceiver_connectTo(pubsub_udp_topic_receiver_t *receiver, const char *url) {
    if (!url) return;
    char *connectUrl =  celix_utils_strdup(url);
    pubsub_utils_url_t *urlInfo = pubsub_utils_url_parse(connectUrl);
    bool is_multicast = pubsub_utils_url_is_multicast(urlInfo->hostname);
    bool is_broadcast = pubsub_utils_url_is_broadcast(urlInfo->hostname);
    pubsub_utils_url_free(urlInfo);

    if (is_multicast || is_broadcast) {
        L_DEBUG("[PSA_udp] TopicReceiver %s/%s connecting to udp url %s",
                receiver->scope == NULL ? "(null)" : receiver->scope,
                receiver->topic,
                url);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        psa_udp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
        if (entry == NULL) {
            entry = calloc(1, sizeof(*entry));
            entry->url = celix_utils_strdup(url);
            entry->connected = false;
            entry->statically = false;
            entry->parent = receiver;
            hashMap_put(receiver->requestedConnections.map, (void *) entry->url, entry);
            receiver->requestedConnections.allConnected = false;
        }
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
        psa_udp_connectToAllRequestedConnections(receiver);
    }
}

void pubsub_udpTopicReceiver_disconnectFrom(pubsub_udp_topic_receiver_t *receiver, const char *url) {
    if (!url) return;
    L_DEBUG("[PSA udp] TopicReceiver %s/%s disconnect from udp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udp_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, url);
    if (entry != NULL) {
        int rc = pubsub_sktHandler_disconnect(receiver->socketHandler, entry->url);
        if (rc < 0)
            L_WARN("[PSA_udp] Error disconnecting from udp url %s. (%s)", url, strerror(errno));
    }
    if (entry != NULL) {
        free(entry->url);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_udpTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_udp_topic_receiver_t *receiver = handle;

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

    psa_udp_subscriber_entry_t *entry = calloc(1, sizeof(*entry));
    entry->subscriberSvc = svc;
    entry->initialized = false;

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hashMap_put(receiver->subscribers.map, (void*)svcId, entry);
    receiver->subscribers.allInitialized = false;
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_udpTopicReceiver_removeSubscriber(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    pubsub_udp_topic_receiver_t *receiver = handle;

    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_udp_subscriber_entry_t *entry = hashMap_remove(receiver->subscribers.map, (void *)svcId);
    free(entry);
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void callReceivers(pubsub_udp_topic_receiver_t *receiver, const char* msgFqn, const pubsub_protocol_message_t *message, void** msg, bool* release, const celix_properties_t* metadata) {
    *release = true;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_udp_subscriber_entry_t* entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL && entry->subscriberSvc->receive != NULL) {
            entry->subscriberSvc->receive(entry->subscriberSvc->handle, msgFqn, message->header.msgId, *msg, metadata, release);
            if (!(*release) && hashMapIterator_hasNext(&iter)) { //receive function has taken ownership, deserialize again for new message
                struct iovec deSerializeBuffer;
                deSerializeBuffer.iov_base = message->payload.payload;
                deSerializeBuffer.iov_len = message->payload.length;
                celix_status_t status = pubsub_serializerHandler_deserialize(receiver->serializerHandler,
                                                                             message->header.msgId,
                                                                             message->header.msgMajorVersion,
                                                                             message->header.msgMinorVersion,
                                                                             &deSerializeBuffer, 0, msg);
                if (status != CELIX_SUCCESS) {
                    L_WARN("[PSA_UDP_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgFqn,
                           receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
                    break;
                }
            } else if (!(*release)) { //receive function has taken ownership, but no receive left anymore. set msg to null
                *msg = NULL;
            }
            *release = true;
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static inline void processMsg(void* handle, const pubsub_protocol_message_t *message, bool* releaseMsg, struct timespec *receiveTime) {
    pubsub_udp_topic_receiver_t *receiver = handle;

    const char *msgFqn = pubsub_serializerHandler_getMsgFqn(receiver->serializerHandler, message->header.msgId);
    if (msgFqn == NULL) {
        L_WARN("Cannot find msg fqn for msg id %u", message->header.msgId);
        return;
    }
    void *deSerializedMsg = NULL;
    bool validVersion = pubsub_serializerHandler_isMessageSupported(receiver->serializerHandler, message->header.msgId,
                                                                    message->header.msgMajorVersion,
                                                                    message->header.msgMinorVersion);
    if (validVersion) {
        struct iovec deSerializeBuffer;
        deSerializeBuffer.iov_base = message->payload.payload;
        deSerializeBuffer.iov_len = message->payload.length;
        celix_status_t status = pubsub_serializerHandler_deserialize(receiver->serializerHandler, message->header.msgId,
                                                                     message->header.msgMajorVersion,
                                                                     message->header.msgMinorVersion,
                                                                     &deSerializeBuffer, 0, &deSerializedMsg);

        // When received payload pointer is the same as deserializedMsg, set ownership of pointer to topic receiver
        if (message->payload.payload == deSerializedMsg) {
            *releaseMsg = true;
        }

        if (status == CELIX_SUCCESS) {
            celix_properties_t *metadata = message->metadata.metadata;
            bool metadataWasNull = metadata == NULL;
            bool cont = pubsubInterceptorHandler_invokePreReceive(receiver->interceptorsHandler, msgFqn, message->header.msgId, deSerializedMsg, &metadata);
            bool release = true;
            if (cont) {
                callReceivers(receiver, msgFqn, message, &deSerializedMsg, &release, metadata);
                if (pubsubInterceptorHandler_nrOfInterceptors(receiver->interceptorsHandler) > 0) {
                    if (deSerializedMsg == NULL) { //message deleted, but still need to call interceptors -> deserialize new message
                        release = true;
                        status = pubsub_serializerHandler_deserialize(receiver->serializerHandler, message->header.msgId,
                                                                                 message->header.msgMajorVersion,
                                                                                 message->header.msgMinorVersion,
                                                                                 &deSerializeBuffer, 0, &deSerializedMsg);
                        if (status != CELIX_SUCCESS) {
                            L_WARN("[PSA_UDP_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgFqn,
                                   receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
                        } else {
                            pubsubInterceptorHandler_invokePostReceive(receiver->interceptorsHandler, msgFqn, message->header.msgId, deSerializedMsg, metadata);
                        }
                    } else {
                        pubsubInterceptorHandler_invokePostReceive(receiver->interceptorsHandler, msgFqn, message->header.msgId, deSerializedMsg, metadata);
                    }
                }
            } else {
                L_TRACE("Skipping receive for msg type %s, based on pre receive interceptor result", msgFqn);
            }
            if (release) {
                pubsub_serializerHandler_freeDeserializedMsg(receiver->serializerHandler, message->header.msgId, deSerializedMsg);
            }
            if (metadataWasNull) {
                //note that if the metadata was created by the pubsubInterceptorHandler_invokePreReceive, this needs to be deallocated
                celix_properties_destroy(metadata);
            }
        } else {
            L_WARN("[PSA_UDP_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgFqn,
                   receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
        }
    } else {
        L_WARN("[PSA_UDP_TR] Cannot deserialize message '%s' using %s, version mismatch. Version received: %i.%i.x, version local: %i.%i.x",
               msgFqn,
               pubsub_serializerHandler_getSerializationType(receiver->serializerHandler),
               (int) message->header.msgMajorVersion,
               (int) message->header.msgMinorVersion,
               pubsub_serializerHandler_getMsgMajorVersion(receiver->serializerHandler, message->header.msgId),
               pubsub_serializerHandler_getMsgMinorVersion(receiver->serializerHandler, message->header.msgId));
    }
}

static void *psa_udp_recvThread(void *data) {
    pubsub_udp_topic_receiver_t *receiver = data;

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
            psa_udp_connectToAllRequestedConnections(receiver);
        }
        if (!allInitialized) {
            psa_udp_initializeAllSubscribers(receiver);
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

static void psa_udp_connectToAllRequestedConnections(pubsub_udp_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udp_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if ((entry) && (!entry->connected) && (!receiver->isPassive)) {
                int rc = pubsub_sktHandler_udp_connect(entry->parent->socketHandler, entry->url);
                if (rc < 0) {
                    allConnected = false;
                }
            }
        }
        receiver->requestedConnections.allConnected = allConnected;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_udp_connectHandler(void *handle, const char *url, bool lock) {
    pubsub_udp_topic_receiver_t *receiver = handle;
    L_DEBUG("[PSA_udp] TopicReceiver %s/%s connecting to udp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);
    if (lock)
        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->parent = receiver;
        entry->url = celix_utils_strdup(url);
        entry->statically = true;
        hashMap_put(receiver->requestedConnections.map, (void *) entry->url, entry);
        receiver->requestedConnections.allConnected = false;
    }
    entry->connected = true;
    if (lock)
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_udp_disConnectHandler(void *handle, const char *url, bool lock) {
    pubsub_udp_topic_receiver_t *receiver = handle;
    L_DEBUG("[PSA udp] TopicReceiver %s/%s disconnect from udp url %s",
            receiver->scope == NULL ? "(null)" : receiver->scope,
            receiver->topic,
            url);
    if (lock)
        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udp_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry != NULL) {
        entry->connected = false;
        receiver->requestedConnections.allConnected = false;
    }
    if (lock)
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_udp_initializeAllSubscribers(pubsub_udp_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    if (!receiver->subscribers.allInitialized) {
        bool allInitialized = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udp_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->initialized) {
                int rc = 0;
                if (entry->subscriberSvc->init != NULL) {
                    rc = entry->subscriberSvc->init(entry->subscriberSvc->handle);
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
