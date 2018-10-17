/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
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
#include <czmq.h>
#include <log_helper.h>
#include "pubsub_zmq_topic_receiver.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_zmq_common.h"
#include "../../pubsub_topology_manager/src/pubsub_topology_manager.h"

//TODO see if block and wakeup (reset) also works
#define PSA_ZMQ_RECV_TIMEOUT 1000


#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_zmq_topic_receiver {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];

    zsock_t *zmqSocket;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = zmq url, value = psa_zmq_requested_connection_entry_t*
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_zmq_subscriber_entry_t
    } subscribers;
};

typedef struct psa_zmq_requested_connection_entry {
    char *url;
    bool connected;
} psa_zmq_requested_connection_entry_t;

typedef struct psa_zmq_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //map from serializer svc
    pubsub_subscriber_t *svc;
} psa_zmq_subscriber_entry_t;


static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void* psa_zmq_recvThread(void * data);


pubsub_zmq_topic_receiver_t* pubsub_zmqTopicReceiver_create(celix_bundle_context_t *ctx,
                                                              log_helper_t *logHelper,
                                                              const char *scope,
                                                                const char *topic,
                                                                long serializerSvcId,
                                                                pubsub_serializer_service_t *serializer) {
    pubsub_zmq_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    psa_zmq_setScopeAndTopicFilter(scope, topic, receiver->scopeAndTopicFilter);


#ifdef BUILD_WITH_ZMQ_SECURITY
    char* keys_bundle_dir = pubsub_getKeysBundleDir(bundle_context);
    if (keys_bundle_dir == NULL){
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
    if (sub_cert == NULL){
        printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", sub_cert_path);
        return CELIX_SERVICE_EXCEPTION;
    }

    zcert_t* pub_cert = zcert_load(pub_cert_path);
    if (pub_cert == NULL){
        zcert_destroy(&sub_cert);
        printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", pub_cert_path);
        return CELIX_SERVICE_EXCEPTION;
    }

    const char* pub_key = zcert_public_txt(pub_cert);
#endif
    receiver->zmqSocket = zsock_new (ZMQ_SUB);
    if (receiver->zmqSocket == NULL) {
#ifdef BUILD_WITH_ZMQ_SECURITY
        zcert_destroy(&sub_cert);
        zcert_destroy(&pub_cert);
#endif
    }

    if (receiver->zmqSocket != NULL) {
        int timeout = PSA_ZMQ_RECV_TIMEOUT;
        void *zmqSocket =  zsock_resolve(receiver->zmqSocket);
        int res = zmq_setsockopt(zmqSocket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        if (res) {
            L_ERROR("[PSA_ZMQ] Cannot set ZMQ socket option ZMQ_RCVTIMEO errno=%d", errno);
        }
#ifdef BUILD_WITH_ZMQ_SECURITY

        zcert_apply (sub_cert, zmq_s);
        zsock_set_curve_serverkey (zmq_s, pub_key); //apply key of publisher to socket of subscriber
#endif
        char subscribeFilter[5];
        psa_zmq_setScopeAndTopicFilter(scope, topic, subscribeFilter);
        zsock_set_subscribe(receiver->zmqSocket, subscribeFilter);

#ifdef BUILD_WITH_ZMQ_SECURITY
        ts->zmq_cert = sub_cert;
	    ts->zmq_pub_cert = pub_cert;
#endif
    }

    if (receiver->zmqSocket != NULL) {
        receiver->scope = strndup(scope, 1024 * 1024);
        receiver->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
        celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
        celixThreadMutex_create(&receiver->recvThread.mutex, NULL);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    }

    //track subscribers
    if (receiver->zmqSocket != NULL ) {
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

    if (receiver->zmqSocket != NULL ) {
        receiver->recvThread.running = true;
        celixThread_create(&receiver->recvThread.thread, NULL, psa_zmq_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "ZMQ TR %s/%s", scope, topic);
        celixThread_setName(&receiver->recvThread.thread, name);
    }

    if (receiver->zmqSocket == NULL) {
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_ZMQ] Cannot create TopicReceiver for %s/%s", scope, topic);
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
                free(entry);
            }
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

        zsock_destroy(&receiver->zmqSocket);

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

void pubsub_zmqTopicReceiver_listConnections(pubsub_zmq_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry->connected) {
            celix_arrayList_add(connectedUrls, strndup(entry->url, 1024));
        } else {
            celix_arrayList_add(unconnectedUrls, strndup(entry->url, 1024));
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}


void pubsub_zmqTopicReceiver_connectTo(
        pubsub_zmq_topic_receiver_t *receiver,
        const char *url) {
    L_DEBUG("[PSA_ZMQ] TopicReceiver %s/%s connecting to zmq url %s", receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->url = strndup(url, 1024*1024);
        entry->connected = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->url, entry);
    }
    if (!entry->connected) {
        if (zsock_connect(receiver->zmqSocket, "%s", url) == 0) {
            entry->connected = true;
        } else {
            L_WARN("[PSA_ZMQ] Error connecting to zmq url %s. (%s)", url, strerror(errno));
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

void pubsub_zmqTopicReceiver_disconnectFrom(pubsub_zmq_topic_receiver_t *receiver, const char *url) {
    L_DEBUG("[PSA ZMQ] TopicReceiver %s/%s disconnect from zmq url %s", receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, url);
    if (entry != NULL && entry->connected) {
        if (zsock_disconnect(receiver->zmqSocket,"%s",url) == 0) {
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
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
        //not the same scope. ignore
        return;
    }

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount += 1;
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->usageCount = 1;
        entry->svc = svc;

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);
        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_ZMQ] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
            free(entry);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_ZMQ] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static inline void processMsgForSubscriberEntry(pubsub_zmq_topic_receiver_t *receiver, psa_zmq_subscriber_entry_t* entry, const pubsub_zmq_msg_header_t *hdr, const byte* payload, size_t payloadSize) {
    pubsub_msg_serializer_t* msgSer = hashMap_get(entry->msgTypes, (void*)(uintptr_t)(hdr->type));
    pubsub_subscriber_t *svc = entry->svc;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = psa_zmq_checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            celix_status_t status = msgSer->deserialize(msgSer, payload, payloadSize, &deserializedMsg);
            if(status == CELIX_SUCCESS) {
                bool release = true;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, NULL, &release);
                if (release) {
                    msgSer->freeMsg(msgSer->handle, deserializedMsg);
                }
            } else {
                L_WARN("[PSA_ZMQ_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_ZMQ_TR] Cannot find serializer for type id %i", hdr->type);
    }
}

static inline void processMsg(pubsub_zmq_topic_receiver_t *receiver, const pubsub_zmq_msg_header_t *hdr, const byte *payload, size_t payloadSize) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            processMsgForSubscriberEntry(receiver, entry, hdr, payload, payloadSize);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void* psa_zmq_recvThread(void * data) {
    pubsub_zmq_topic_receiver_t *receiver = data;

    celixThreadMutex_lock(&receiver->recvThread.mutex);
    bool running = receiver->recvThread.running;
    celixThreadMutex_unlock(&receiver->recvThread.mutex);

    while (running) {
        zmsg_t *zmsg = zmsg_recv(receiver->zmqSocket);
        if (zmsg != NULL) {
            if (zmsg_size(zmsg) != 3) {
                L_WARN("[PSA_ZMQ_TR] Always expecting 2 frames per zmsg (header + payload), got %i frames", (int)zmsg_size(zmsg));
            } else {
                zframe_t *filter = zmsg_pop(zmsg); //char[5] filter
                zframe_t *header = zmsg_pop(zmsg); //pubsub_zmq_msg_header_t
                zframe_t *payload = zmsg_pop(zmsg); //serialized payload
                if (header != NULL && payload != NULL) {
                    processMsg(receiver, (pubsub_zmq_msg_header_t*)zframe_data(header), zframe_data(payload), zframe_size(payload));
                }
                zframe_destroy(&filter);
                zframe_destroy(&header);
                zframe_destroy(&payload);
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
    } // while

    return NULL;
}
