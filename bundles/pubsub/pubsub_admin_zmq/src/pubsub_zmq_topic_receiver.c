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
#include <pubsub_protocol.h>
#include <stdlib.h>
#include <pubsub/subscriber.h>
#include <memory.h>
#include <pubsub_constants.h>
#if !defined(__APPLE__)
    #include <sys/epoll.h>
#endif
#include <assert.h>
#include <pubsub_endpoint.h>
#include <arpa/inet.h>
#include <czmq.h>
#include <celix_log_helper.h>
#include "pubsub_zmq_topic_receiver.h"
#include "pubsub_psa_zmq_constants.h"

#include <uuid/uuid.h>
#include <pubsub_admin_metrics.h>
#include <pubsub_utils.h>
#include <celix_api.h>

#include "pubsub_interceptors_handler.h"

#include "celix_utils_api.h"

#define PSA_ZMQ_RECV_TIMEOUT 1000

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

struct pubsub_zmq_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    char *scope;
    char *topic;
    bool metricsEnabled;

    pubsub_interceptors_handler_t *interceptorsHandler;

    void *zmqCtx;
    void *zmqSock;

    char sync[8];

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = zmq url, value = psa_zmq_requested_connection_entry_t*
        bool allConnected; //true if all requestedConnectection are connected
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_zmq_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_zmq_requested_connection_entry {
    char *url;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_zmq_requested_connection_entry_t;

typedef struct psa_zmq_subscriber_metrics_entry_t {
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
} psa_zmq_subscriber_metrics_entry_t;

typedef struct psa_zmq_subscriber_entry {
    hash_map_t *msgTypes; //map from serializer svc
    hash_map_t *metrics; //key = msg type id, value = hash_map (key = origin uuid, value = psa_zmq_subscriber_metrics_entry_t*
    hash_map_t *subscriberServices; //key = servide id, value = pubsub_subscriber_t*
    bool initialized; //true if the init function is called through the receive thread
} psa_zmq_subscriber_entry_t;


static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void* psa_zmq_recvThread(void * data);
static void psa_zmq_connectToAllRequestedConnections(pubsub_zmq_topic_receiver_t *receiver);
static void psa_zmq_initializeAllSubscribers(pubsub_zmq_topic_receiver_t *receiver);
static void psa_zmq_setupZmqContext(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties);
static void psa_zmq_setupZmqSocket(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties);
static bool psa_zmq_checkVersion(version_pt msgVersion, uint16_t major, uint16_t minor);


pubsub_zmq_topic_receiver_t* pubsub_zmqTopicReceiver_create(celix_bundle_context_t *ctx,
                                                              celix_log_helper_t *logHelper,
                                                              const char *scope,
                                                              const char *topic,
                                                              const celix_properties_t *topicProperties,
                                                              long serializerSvcId,
                                                              pubsub_serializer_service_t *serializer,
                                                              long protocolSvcId,
                                                              pubsub_protocol_service_t *protocol) {
    pubsub_zmq_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    receiver->protocolSvcId = protocolSvcId;
    receiver->protocol = protocol;
    receiver->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    receiver->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_ZMQ_METRICS_ENABLED, PSA_ZMQ_DEFAULT_METRICS_ENABLED);

    pubsubInterceptorsHandler_create(ctx, scope, topic, &receiver->interceptorsHandler);

#ifdef BUILD_WITH_ZMQ_SECURITY
    char* keys_bundle_dir = pubsub_getKeysBundleDir(bundle_context);
    if (keys_bundle_dir == NULL) {
        return CELIX_SERVICE_EXCEPTION;
    }

    const char* keys_file_path = NULL;
    const char* keys_file_name = NULL;
    bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_PATH, &keys_file_path);
    bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_NAME, &keys_file_name);

    char sub_cert_path[MAX_CERT_PATH_LENGTH];
    char pub_cert_path[MAX_CERT_PATH_LENGTH];

    //certificate path ".cache/bundle{id}/version0.0/./META-INF/keys/subscriber/private/sub_{topic}.key.enc"
    snprintf(sub_cert_path, MAX_CERT_PATH_LENGTH, "%s/META-INF/keys/subscriber/private/sub_%s.key.enc", keys_bundle_dir, topic);
    snprintf(pub_cert_path, MAX_CERT_PATH_LENGTH, "%s/META-INF/keys/publisher/public/pub_%s.pub", keys_bundle_dir, topic);
    free(keys_bundle_dir);

    printf("PSA_ZMQ_PSA_ZMQ_TS: Loading subscriber key '%s'\n", sub_cert_path);
    printf("PSA_ZMQ_PSA_ZMQ_TS: Loading publisher key '%s'\n", pub_cert_path);

    zcert_t* sub_cert = get_zcert_from_encoded_file((char *) keys_file_path, (char *) keys_file_name, sub_cert_path);
    if (sub_cert == NULL) {
        printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", sub_cert_path);
        return CELIX_SERVICE_EXCEPTION;
    }

    zcert_t* pub_cert = zcert_load(pub_cert_path);
    if (pub_cert == NULL) {
        zcert_destroy(&sub_cert);
        printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", pub_cert_path);
        return CELIX_SERVICE_EXCEPTION;
    }

    const char* pub_key = zcert_public_txt(pub_cert);
#endif
    receiver->zmqCtx = zmq_ctx_new();
    if (receiver->zmqCtx != NULL) {
        psa_zmq_setupZmqContext(receiver, topicProperties);
        receiver->zmqSock = zmq_socket(receiver->zmqCtx, ZMQ_SUB);
    } else {
        //LOG ctx problem
    }
    if (receiver->zmqSock != NULL) {
        psa_zmq_setupZmqSocket(receiver, topicProperties);
    } else if (receiver->zmqCtx != NULL) {
        //LOG sock problem
    }

    if (receiver->zmqSock == NULL) {
#ifdef BUILD_WITH_ZMQ_SECURITY
        zcert_destroy(&sub_cert);
        zcert_destroy(&pub_cert);
#endif
    }


    if (receiver->zmqSock != NULL) {
        celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
        celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
        celixThreadMutex_create(&receiver->recvThread.mutex, NULL);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    }

    const char *staticConnectUrls = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_ZMQ_STATIC_CONNECT_URLS_FOR, topic, scope);
    if(staticConnectUrls == NULL) {
        staticConnectUrls = celix_properties_get(topicProperties, PUBSUB_ZMQ_STATIC_CONNECT_URLS, NULL);
    }
    if (receiver->zmqSock != NULL && staticConnectUrls != NULL) {
        char *urlsCopy = strndup(staticConnectUrls, 1024*1024);
        char* url;
        char* save = urlsCopy;

        while ((url = strtok_r(save, " ", &save))) {
            psa_zmq_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
            entry->statically = true;
            entry->connected = false;
            entry->url = strndup(url, 1024*1024);
            hashMap_put(receiver->requestedConnections.map, entry->url, entry);
            receiver->requestedConnections.allConnected = false;
        }
        free(urlsCopy);
    }

    //track subscribers
    if (receiver->zmqSock != NULL ) {
        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size+1];
        snprintf(buf, (size_t)size+1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_zmqTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_zmqTopicReceiver_removeSubscriber;

        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    if (receiver->zmqSock != NULL ) {
        receiver->recvThread.running = true;
        celixThread_create(&receiver->recvThread.thread, NULL, psa_zmq_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "ZMQ TR %s/%s", scope == NULL ? "(null)" : scope, topic);
        celixThread_setName(&receiver->recvThread.thread, name);
    }

    if (receiver->zmqSock == NULL) {
        if (receiver->scope != NULL) {
            free(receiver->scope);
        }
        free(receiver->topic);
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_ZMQ] Cannot create TopicReceiver for %s/%s", scope == NULL ? "(null)" : scope, topic);
    }

    return receiver;
}

void pubsub_zmqTopicReceiver_destroy(pubsub_zmq_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        receiver->recvThread.running = false;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
        celixThread_join(receiver->recvThread.thread, NULL);

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL)  {
                receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                hashMap_destroy(entry->subscriberServices, false, false);
                free(entry);
            }

            hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
            while (hashMapIterator_hasNext(&iter2)) {
                hash_map_t *origins = hashMapIterator_nextValue(&iter2);
                hashMap_destroy(origins, true, true);
            }
            hashMap_destroy(entry->metrics, false, false);
        }
        hashMap_destroy(receiver->subscribers.map, false, false);


        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                free(entry->url);
                free(entry);
            }
        }
        hashMap_destroy(receiver->requestedConnections.map, false, false);
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_destroy(&receiver->subscribers.mutex);
        celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        celixThreadMutex_destroy(&receiver->recvThread.mutex);

        zmq_close(receiver->zmqSock);
        zmq_ctx_term(receiver->zmqCtx);

        pubsubInterceptorsHandler_destroy(receiver->interceptorsHandler);

        free(receiver->scope);
        free(receiver->topic);
    }
    free(receiver);
}

const char* pubsub_zmqTopicReceiver_scope(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->scope;
}
const char* pubsub_zmqTopicReceiver_topic(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->topic;
}

long pubsub_zmqTopicReceiver_serializerSvcId(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

long pubsub_zmqTopicReceiver_protocolSvcId(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->protocolSvcId;
}

void pubsub_zmqTopicReceiver_listConnections(pubsub_zmq_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        char *url = NULL;
        asprintf(&url, "%s%s", entry->url, entry->statically ? " (static)" : "");
        if (entry->connected) {
            celix_arrayList_add(connectedUrls, url);
        } else {
            celix_arrayList_add(unconnectedUrls, url);
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}


void pubsub_zmqTopicReceiver_connectTo(
        pubsub_zmq_topic_receiver_t *receiver,
        const char *url) {
    L_DEBUG("[PSA_ZMQ] TopicReceiver %s/%s connecting to zmq url %s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->url = strndup(url, 1024*1024);
        entry->connected = false;
        entry->statically = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->url, entry);
        receiver->requestedConnections.allConnected = false;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    psa_zmq_connectToAllRequestedConnections(receiver);
}

void pubsub_zmqTopicReceiver_disconnectFrom(pubsub_zmq_topic_receiver_t *receiver, const char *url) {
    L_DEBUG("[PSA ZMQ] TopicReceiver %s/%s disconnect from zmq url %s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, url);
    if (entry != NULL && entry->connected) {
        if (zmq_disconnect(receiver->zmqSock, url) == 0) {
            entry->connected = false;
        } else {
            L_WARN("[PSA_ZMQ] Error disconnecting from zmq url %s. (%s)", url, strerror(errno));
        }
    }
    if (entry != NULL) {
        free(entry->url);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

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
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        hashMap_put(entry->subscriberServices, (void*)svcId, svc);
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->subscriberServices = hashMap_create(NULL, NULL, NULL, NULL);
        entry->initialized = false;
        hashMap_put(entry->subscriberServices, (void*)svcId, svc);

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
            L_ERROR("[PSA_ZMQ] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
            free(entry);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        hashMap_remove(entry->subscriberServices, (void*)svcId);
    }
    if (entry != NULL && hashMap_size(entry->subscriberServices) == 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_ZMQ] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
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

static inline void processMsgForSubscriberEntry(pubsub_zmq_topic_receiver_t *receiver, psa_zmq_subscriber_entry_t* entry, pubsub_protocol_message_t *message, struct timespec *receiveTime) {
    //NOTE receiver->subscribers.mutex locked
    pubsub_msg_serializer_t* msgSer = hashMap_get(entry->msgTypes, (void*)(uintptr_t)(message->header.msgId));
    bool monitor = receiver->metricsEnabled;

    //monitoring
    struct timespec beginSer;
    struct timespec endSer;
    int updateReceiveCount = 0;
    int updateSerError = 0;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = psa_zmq_checkVersion(msgSer->msgVersion, message->header.msgMajorVersion, message->header.msgMinorVersion);
        if (validVersion) {
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &beginSer);
            }
            struct iovec deSerializeBuffer;
            deSerializeBuffer.iov_base = message->payload.payload;
            deSerializeBuffer.iov_len  = message->payload.length;
            celix_status_t status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 0, &deserializedMsg);
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &endSer);
            }
            if (status == CELIX_SUCCESS) {

                const char *msgType = msgSer->msgName;
                uint32_t msgId = message->header.msgId;
                celix_properties_t *metadata = message->metadata.metadata;
                bool cont = pubsubInterceptorHandler_invokePreReceive(receiver->interceptorsHandler, msgType, msgId, deserializedMsg, &metadata);
                bool release = true;
                if (cont) {
                    hash_map_iterator_t iter2 = hashMapIterator_construct(entry->subscriberServices);
                    while (hashMapIterator_hasNext(&iter2)) {
                        pubsub_subscriber_t *svc = hashMapIterator_nextValue(&iter2);
                        svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, metadata, &release);
                        pubsubInterceptorHandler_invokePostReceive(receiver->interceptorsHandler, msgType, msgId, deserializedMsg, metadata);
                        if (!release && hashMapIterator_hasNext(&iter2)) {
                            //receive function has taken ownership and still more receive function to come ..
                            //deserialize again for new message
                            status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 0, &deserializedMsg);
                            if (status != CELIX_SUCCESS) {
                                L_WARN("[PSA_ZMQ_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
                                break;
                            }
                            release = true;
                        }
                    }
                    if (release) {
                        msgSer->freeDeserializeMsg(msgSer->handle, deserializedMsg);
                    }
                    updateReceiveCount += 1;
                }
            } else {
                updateSerError += 1;
                L_WARN("[PSA_ZMQ_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_ZMQ_TR] Cannot find serializer for type id 0x%X", message->header.msgId);
    }

    if (msgSer != NULL && monitor) {
        // TODO disabled for now, should move to an interceptor?
//        hash_map_t *origins = hashMap_get(entry->metrics, (void*)(uintptr_t )message->header.msgId);
//        char uuidStr[UUID_STR_LEN+1];
//        uuid_unparse(hdr->originUUID, uuidStr);
//        psa_zmq_subscriber_metrics_entry_t *metrics = hashMap_get(origins, uuidStr);
//
//        if (metrics == NULL) {
//            metrics = calloc(1, sizeof(*metrics));
//            hashMap_put(origins, strndup(uuidStr, UUID_STR_LEN+1), metrics);
//            uuid_copy(metrics->origin, hdr->originUUID);
//            metrics->msgTypeId = hdr->type;
//            metrics->maxDelayInSeconds = -INFINITY;
//            metrics->minDelayInSeconds = INFINITY;
//            metrics->lastSeqNr = 0;
//        }
//
//        double diff = celix_difftime(&beginSer, &endSer);
//        long n = metrics->nrOfMessagesReceived;
//        metrics->averageSerializationTimeInSeconds = (metrics->averageSerializationTimeInSeconds * n + diff) / (n+1);
//
//        diff = celix_difftime(&metrics->lastMessageReceived, receiveTime);
//        n = metrics->nrOfMessagesReceived;
//        if (metrics->nrOfMessagesReceived >= 1) {
//            metrics->averageTimeBetweenMessagesInSeconds = (metrics->averageTimeBetweenMessagesInSeconds * n + diff) / (n + 1);
//        }
//        metrics->lastMessageReceived = *receiveTime;
//
//
//        int incr = hdr->seqNr - metrics->lastSeqNr;
//        if (metrics->lastSeqNr >0 && incr > 1) {
//            metrics->nrOfMissingSeqNumbers += (incr - 1);
//            L_WARN("Missing message seq nr went from %i to %i", metrics->lastSeqNr, hdr->seqNr);
//        }
//        metrics->lastSeqNr = hdr->seqNr;
//
//        struct timespec sendTime;
//        sendTime.tv_sec = (time_t)hdr->sendtimeSeconds;
//        sendTime.tv_nsec = (long)hdr->sendTimeNanoseconds; //TODO FIXME the tv_nsec is not correct
//        diff = celix_difftime(&sendTime, receiveTime);
//        metrics->averageDelayInSeconds = (metrics->averageDelayInSeconds * n + diff) / (n+1);
//        if (diff < metrics->minDelayInSeconds) {
//            metrics->minDelayInSeconds = diff;
//        }
//        if (diff > metrics->maxDelayInSeconds) {
//            metrics->maxDelayInSeconds = diff;
//        }
//
//        metrics->nrOfMessagesReceived += updateReceiveCount;
//        metrics->nrOfSerializationErrors += updateSerError;
    }
}

static inline void processMsg(pubsub_zmq_topic_receiver_t *receiver, pubsub_protocol_message_t *message, struct timespec *receiveTime) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            processMsgForSubscriberEntry(receiver, entry, message, receiveTime);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void* psa_zmq_recvThread(void * data) {
    pubsub_zmq_topic_receiver_t *receiver = data;

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
            psa_zmq_connectToAllRequestedConnections(receiver);
        }
        if (!allInitialized) {
            psa_zmq_initializeAllSubscribers(receiver);
        }

        zmsg_t *zmsg = zmsg_recv(receiver->zmqSock);
        if (zmsg != NULL) {
            if (zmsg_size(zmsg) < 2) {
                L_WARN("[PSA_ZMQ_TR] Always expecting at least frames per zmsg (header + payload (+ metadata) (+ footer)), got %i frames", (int)zmsg_size(zmsg));
            } else {
                zframe_t *header = zmsg_pop(zmsg); // header
                zframe_t *payload = NULL;
                zframe_t *metadata = NULL;
                zframe_t *footer = NULL;

                pubsub_protocol_message_t message;
                size_t footerSize = 0;
                receiver->protocol->getFooterSize(receiver->protocol->handle, &footerSize);
                receiver->protocol->decodeHeader(receiver->protocol->handle, zframe_data(header), zframe_size(header), &message);
                if (message.header.payloadSize > 0) {
                    payload = zmsg_pop(zmsg);
                    receiver->protocol->decodePayload(receiver->protocol->handle, zframe_data(payload), zframe_size(payload), &message);
                } else {
                    message.payload.payload = NULL;
                    message.payload.length = 0;
                }
                if (message.header.metadataSize > 0) {
                    metadata = zmsg_pop(zmsg);
                    receiver->protocol->decodeMetadata(receiver->protocol->handle, zframe_data(metadata), zframe_size(metadata), &message);
                } else {
                    message.metadata.metadata = NULL;
                }
                if (footerSize > 0) {
                    footer = zmsg_pop(zmsg); // footer
                    receiver->protocol->decodeFooter(receiver->protocol->handle, zframe_data(footer), zframe_size(footer), &message);
                }
                if (header != NULL && payload != NULL) {
                    struct timespec receiveTime;
                    clock_gettime(CLOCK_REALTIME, &receiveTime);
                    processMsg(receiver, &message, &receiveTime);
                }
                celix_properties_destroy(message.metadata.metadata);
                zframe_destroy(&header);
                zframe_destroy(&payload);
                zframe_destroy(&metadata);
                zframe_destroy(&footer);
            }
            zmsg_destroy(&zmsg);
        } else {
            if (errno == EAGAIN) {
                //nop
            } else if (errno == EINTR) {
                L_DEBUG("[PSA_ZMQ_TR] zmsg_recv interrupted");
            } else {
                L_WARN("[PSA_ZMQ_TR] Error receiving zmq message: %s", strerror(errno));
            }
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

pubsub_admin_receiver_metrics_t* pubsub_zmqTopicReceiver_metrics(pubsub_zmq_topic_receiver_t *receiver) {
    pubsub_admin_receiver_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->scope == NULL ? PUBSUB_DEFAULT_ENDPOINT_SCOPE : receiver->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", receiver->topic);

    int msgTypesCount = 0;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
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
        psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->metrics);
        while (hashMapIterator_hasNext(&iter2)) {
            hash_map_t *origins = hashMapIterator_nextValue(&iter2);
            result->msgTypes[i].origins = calloc((size_t)hashMap_size(origins), sizeof(*(result->msgTypes[i].origins)));
            result->msgTypes[i].nrOfOrigins = hashMap_size(origins);
            int k = 0;
            hash_map_iterator_t iter3 = hashMapIterator_construct(origins);
            while (hashMapIterator_hasNext(&iter3)) {
                psa_zmq_subscriber_metrics_entry_t *metrics = hashMapIterator_nextValue(&iter3);
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
                    L_WARN("[PSA_ZMQ]: Error cannot find key 0x%X in msg map during metrics collection!\n", metrics->msgTypeId);
                }
            }
            i +=1 ;
        }
    }

    celixThreadMutex_unlock(&receiver->subscribers.mutex);

    return result;
}


static void psa_zmq_connectToAllRequestedConnections(pubsub_zmq_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->connected) {
                if (zmq_connect(receiver->zmqSock, entry->url) == 0) {
                    entry->connected = true;
                } else {
                    L_WARN("[PSA_ZMQ] Error connecting to zmq url %s. (%s)", entry->url, strerror(errno));
                    allConnected = false;
                }
            }
        }
        receiver->requestedConnections.allConnected = allConnected;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_zmq_initializeAllSubscribers(pubsub_zmq_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    if (!receiver->subscribers.allInitialized) {
        bool allInitialized = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
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

static void psa_zmq_setupZmqContext(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties) {
    //NOTE. ZMQ will abort when performing a sched_setscheduler without permission.
    //As result permission has to be checked first.
    //TODO update this to use cap_get_pid and cap-get_flag instead of check user is root (note adds dep to -lcap)
    bool gotPermission = false;
    if (getuid() == 0) {
        gotPermission = true;
    }


    long prio = celix_properties_getAsLong(topicProperties, PUBSUB_ZMQ_THREAD_REALTIME_PRIO, -1L);
    if (prio > 0 && prio < 100) {
        if (gotPermission) {
            zmq_ctx_set(receiver->zmqCtx, ZMQ_THREAD_PRIORITY, (int) prio);
        } else {
            L_INFO("Skipping configuration of thread prio to %i. No permission\n", (int)prio);
        }
    }

    const char *sched = celix_properties_get(topicProperties, PUBSUB_ZMQ_THREAD_REALTIME_SCHED, NULL);
    if (sched != NULL) {
        int policy = ZMQ_THREAD_SCHED_POLICY_DFLT;
        if (strncmp("SCHED_OTHER", sched, 16) == 0) {
            policy = SCHED_OTHER;
#if !defined(__APPLE__)
        } else if (strncmp("SCHED_BATCH", sched, 16) == 0) {
            policy = SCHED_BATCH;
        } else if (strncmp("SCHED_IDLE", sched, 16) == 0) {
            policy = SCHED_IDLE;
#endif
        } else if (strncmp("SCHED_FIFO", sched, 16) == 0) {
            policy = SCHED_FIFO;
        } else if (strncmp("SCHED_RR", sched, 16) == 0) {
            policy = SCHED_RR;
        }
        if (gotPermission) {
            zmq_ctx_set(receiver->zmqCtx, ZMQ_THREAD_SCHED_POLICY, policy);
        } else {
            L_INFO("Skipping configuration of thread scheduling to %s. No permission\n", sched);
        }
    }
}

static void psa_zmq_setupZmqSocket(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties) {
    int timeout = PSA_ZMQ_RECV_TIMEOUT;
    int res = zmq_setsockopt(receiver->zmqSock, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (res) {
        L_ERROR("[PSA_ZMQ] Cannot set ZMQ socket option ZMQ_RCVTIMEO errno=%d", errno);
    }

#ifdef ZMQ_HWM
    long hwmProp = celix_properties_getAsLong(topicProperties, PUBSUB_ZMQ_HWM, -1L);
    if (hwmProp >= 0) {
        unsigned long hwm = (unsigned long)hwmProp;
        zmq_setsockopt(receiver->zmqSock, ZMQ_HWM, &hwm, sizeof(hwm));
    }
#endif

#ifdef BUILD_WITH_ZMQ_SECURITY

    zcert_apply (sub_cert, zmq_s);
    zsock_set_curve_serverkey (zmq_s, pub_key); //apply key of publisher to socket of subscriber
#endif
    receiver->protocol->getSyncHeader(receiver->protocol->handle, receiver->sync);
    zsock_set_subscribe(receiver->zmqSock, receiver->sync);

#ifdef BUILD_WITH_ZMQ_SECURITY
    ts->zmq_cert = sub_cert;
    ts->zmq_pub_cert = pub_cert;
#endif
}

static bool psa_zmq_checkVersion(version_pt msgVersion, uint16_t major, uint16_t minor) {
    bool check=false;

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
