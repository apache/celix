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
#include <arpa/inet.h>
#include <czmq.h>
#include <uuid/uuid.h>

#include "pubsub_endpoint.h"
#include "celix_log_helper.h"
#include "pubsub_zmq_topic_receiver.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_admin_metrics.h"
#include "pubsub_utils.h"
#include "celix_api.h"
#include "celix_version.h"
#include "pubsub_serializer_handler.h"
#include "pubsub_interceptors_handler.h"
#include "celix_utils_api.h"
#include "pubsub_zmq_admin.h"

#define PSA_ZMQ_RECV_TIMEOUT 1000

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37
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

struct pubsub_zmq_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    void *admin;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    char *scope;
    char *topic;

    pubsub_serializer_handler_t* serializerHandler;
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
        hash_map_t *map; //key = long svc id, value = psa_zmq_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_zmq_requested_connection_entry {
    char *url;
    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_zmq_requested_connection_entry_t;

typedef struct psa_zmq_subscriber_entry {
    pubsub_subscriber_t* subscriberSvc;
    bool initialized; //true if the init function is called through the receive thread
} psa_zmq_subscriber_entry_t;


static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props);
static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props);
static void* psa_zmq_recvThread(void * data);
static void psa_zmq_connectToAllRequestedConnections(pubsub_zmq_topic_receiver_t *receiver);
static void psa_zmq_initializeAllSubscribers(pubsub_zmq_topic_receiver_t *receiver);
static void psa_zmq_setupZmqContext(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties);
static void psa_zmq_setupZmqSocket(pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *topicProperties);


pubsub_zmq_topic_receiver_t* pubsub_zmqTopicReceiver_create(celix_bundle_context_t *ctx,
                                                            celix_log_helper_t *logHelper,
                                                            const char *scope,
                                                            const char *topic,
                                                            const celix_properties_t *topicProperties,
                                                            pubsub_serializer_handler_t* serHandler,
                                                            void *admin,
                                                            long protocolSvcId,
                                                            pubsub_protocol_service_t *protocol) {
    pubsub_zmq_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerHandler = serHandler;
    receiver->admin = admin;
    receiver->protocolSvcId = protocolSvcId;
    receiver->protocol = protocol;
    receiver->scope = scope == NULL ? NULL : celix_utils_strdup(scope);
    receiver->topic = celix_utils_strdup(topic);

    receiver->interceptorsHandler = pubsubInterceptorsHandler_create(ctx, scope, topic, PUBSUB_ZMQ_ADMIN_TYPE,
                                                                     pubsub_serializerHandler_getSerializationType(serHandler));

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
        char *urlsCopy = celix_utils_strdup(staticConnectUrls);
        char* url;
        char* save = urlsCopy;

        while ((url = strtok_r(save, " ", &save))) {
            psa_zmq_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
            entry->statically = true;
            entry->connected = false;
            entry->url = celix_utils_strdup(url);
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
        opts.addWithProperties = pubsub_zmqTopicReceiver_addSubscriber;
        opts.removeWithProperties = pubsub_zmqTopicReceiver_removeSubscriber;

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
        hashMap_destroy(receiver->subscribers.map, false, true);
        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
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

const char* pubsub_zmqTopicReceiver_serializerType(pubsub_zmq_topic_receiver_t *receiver) {
    return pubsub_serializerHandler_getSerializationType(receiver->serializerHandler);
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
        entry->url = celix_utils_strdup(url);
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

static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

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

    psa_zmq_subscriber_entry_t *entry = calloc(1, sizeof(*entry));
    entry->subscriberSvc = svc;
    entry->initialized = false;

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hashMap_put(receiver->subscribers.map, (void*)svcId, entry);
    receiver->subscribers.allInitialized = false;
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_remove(receiver->subscribers.map, (void*)svcId);
    free(entry);
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void callReceivers(pubsub_zmq_topic_receiver_t *receiver, const char* msgFqn, const pubsub_protocol_message_t *message, void** msg, bool* release, const celix_properties_t* metadata) {
    *release = true;
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_subscriber_entry_t* entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL && entry->subscriberSvc->receive != NULL) {
            entry->subscriberSvc->receive(entry->subscriberSvc->handle, msgFqn, message->header.msgId, *msg, metadata, release);
            if (!(*release)) {
                //receive function has taken ownership, deserialize again for new message
                struct iovec deSerializeBuffer;
                deSerializeBuffer.iov_base = message->payload.payload;
                deSerializeBuffer.iov_len = message->payload.length;
                celix_status_t status = pubsub_serializerHandler_deserialize(receiver->serializerHandler,
                                                             message->header.msgId,
                                                             message->header.msgMajorVersion,
                                                             message->header.msgMinorVersion,
                                                              &deSerializeBuffer, 0, msg);
                if (status != CELIX_SUCCESS) {
                    L_WARN("[PSA_ZMQ_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgFqn,
                           receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
                    break;
                }
            }
            *release = true;
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static inline void processMsg(pubsub_zmq_topic_receiver_t *receiver, pubsub_protocol_message_t *message, struct timespec *receiveTime) {
    const char *msgFqn = pubsub_serializerHandler_getMsgFqn(receiver->serializerHandler, message->header.msgId);
    if (msgFqn == NULL) {
        L_WARN("Cannot find msg fqn for msg id %u", message->header.msgId);
        return;
    }
    void *deserializedMsg = NULL;
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
                                                                     &deSerializeBuffer, 0, &deserializedMsg);
        if (status == CELIX_SUCCESS) {
            celix_properties_t *metadata = message->metadata.metadata;
            bool cont = pubsubInterceptorHandler_invokePreReceive(receiver->interceptorsHandler, msgFqn, message->header.msgId, deserializedMsg, &metadata);
            bool release = true;
            if (cont) {
                callReceivers(receiver, msgFqn, message, &deserializedMsg, &release, metadata);
                pubsubInterceptorHandler_invokePostReceive(receiver->interceptorsHandler, msgFqn, message->header.msgId, deserializedMsg, metadata);
            } else {
                L_TRACE("Skipping receive for msg type %s, based on pre receive interceptor result", msgFqn);
            }
            if (release) {
                pubsub_serializerHandler_freeDeserializedMsg(receiver->serializerHandler, message->header.msgId, deserializedMsg);
            }
        } else {
            L_WARN("[PSA_ZMQ_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgFqn,
                   receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
        }
    } else {
        L_WARN("[PSA_ZMQ_TR] Cannot deserialize message '%s' using %s, version mismatch. Version received: %i.%i.x, version local: %i.%i.x",
               msgFqn,
               pubsub_serializerHandler_getSerializationType(receiver->serializerHandler),
               (int) message->header.msgMajorVersion,
               (int) message->header.msgMinorVersion,
               pubsub_serializerHandler_getMsgMajorVersion(receiver->serializerHandler, message->header.msgId),
               pubsub_serializerHandler_getMsgMinorVersion(receiver->serializerHandler, message->header.msgId));
    }
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
                int rc = 0;
                if (entry->subscriberSvc != NULL && entry->subscriberSvc->init != NULL) {
                    rc = entry->subscriberSvc->init(entry->subscriberSvc->handle);
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
