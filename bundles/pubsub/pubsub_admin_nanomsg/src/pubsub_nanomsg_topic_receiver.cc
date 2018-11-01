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

#include <mutex>
#include <memory.h>
#include <vector>
#include <string>
#include <sstream>

#include <stdlib.h>
#include <assert.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <pubsub_serializer.h>
#include <pubsub/subscriber.h>
#include <pubsub_constants.h>
#include <pubsub_endpoint.h>
#include <log_helper.h>

#include "pubsub_nanomsg_topic_receiver.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"
#include "pubsub_topology_manager.h"

//TODO see if block and wakeup (reset) also works
#define PSA_NANOMSG_RECV_TIMEOUT 1000

/*
#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)
*/
#define L_DEBUG printf
#define L_INFO printf
#define L_WARN printf
#define L_ERROR printf

struct pubsub_nanomsg_topic_receiver {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];

    int nanoMsgSocket;

    struct {
        celix_thread_t thread;
        std::mutex mutex;
        bool running;
    } recvThread;

    struct {
        std::mutex mutex;
        hash_map_t *map; //key = zmq url, value = psa_zmq_requested_connection_entry_t*
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        std::mutex mutex;
        hash_map_t *map; //key = bnd id, value = psa_zmq_subscriber_entry_t
    } subscribers;
};

typedef struct psa_zmq_requested_connection_entry {
    char *url;
    bool connected;
    int id;
} psa_nanomsg_requested_connection_entry_t;

typedef struct psa_zmq_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //map from serializer svc
    pubsub_subscriber_t *svc;
} psa_nanomsg_subscriber_entry_t;


static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_nanoMsgTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props,
                                                         const celix_bundle_t *owner);
static void* psa_nanomsg_recvThread(void *data);


pubsub_nanomsg_topic_receiver_t* pubsub_nanoMsgTopicReceiver_create(celix_bundle_context_t *ctx,
                                                                    log_helper_t *logHelper, const char *scope,
                                                                    const char *topic, long serializerSvcId,
                                                                    pubsub_serializer_service_t *serializer) {
    pubsub_nanomsg_topic_receiver_t *receiver = static_cast<pubsub_nanomsg_topic_receiver*>(calloc(1, sizeof(*receiver)));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    psa_nanomsg_setScopeAndTopicFilter(scope, topic, receiver->scopeAndTopicFilter);


    receiver->nanoMsgSocket = nn_socket(AF_SP, NN_BUS);
    if (receiver->nanoMsgSocket < 0) {
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for %s/%s", scope, topic);
    } else {
        int timeout = PSA_NANOMSG_RECV_TIMEOUT;
        if (nn_setsockopt(receiver->nanoMsgSocket , NN_SOL_SOCKET, NN_RCVTIMEO, &timeout,
                          sizeof (timeout)) < 0) {
            free(receiver);
            receiver = NULL;
            L_ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for %s/%s, set sockopt RECV_TIMEO failed", scope, topic);
        }

        char subscribeFilter[5];
        psa_nanomsg_setScopeAndTopicFilter(scope, topic, subscribeFilter);
        //zsock_set_subscribe(receiver->nanoMsgSocket, subscribeFilter);

        receiver->scope = strndup(scope, 1024 * 1024);
        receiver->topic = strndup(topic, 1024 * 1024);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size + 1];
        snprintf(buf, (size_t) size + 1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts{};
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_zmqTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_nanoMsgTopicReceiver_removeSubscriber;

        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
        receiver->recvThread.running = true;
        celixThread_create(&receiver->recvThread.thread, NULL, psa_nanomsg_recvThread, receiver);
        std::stringstream namestream;
        namestream << "NANOMSG TR " << scope << "/" << topic;
        celixThread_setName(&receiver->recvThread.thread, namestream.str().c_str());
    }
    return receiver;
}

void pubsub_nanoMsgTopicReceiver_destroy(pubsub_nanomsg_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        {
            std::lock_guard<std::mutex> _lock(receiver->recvThread.mutex);
            receiver->recvThread.running = false;
        }
        celixThread_join(receiver->recvThread.thread, NULL);

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        hash_map_iterator_t iter=hash_map_iterator_t();
        {
            std::lock_guard<std::mutex> _lock(receiver->subscribers.mutex);
            iter = hashMapIterator_construct(receiver->subscribers.map);
            while (hashMapIterator_hasNext(&iter)) {
                psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMapIterator_nextValue(&iter));
                if (entry != NULL)  {
                    receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                    free(entry);
                }
            }
            hashMap_destroy(receiver->subscribers.map, false, false);
        }


        {
            std::lock_guard<std::mutex> _lock(receiver->requestedConnections.mutex);
            iter = hashMapIterator_construct(receiver->requestedConnections.map);
            while (hashMapIterator_hasNext(&iter)) {
                psa_nanomsg_requested_connection_entry_t *entry = static_cast<psa_nanomsg_requested_connection_entry_t*>(hashMapIterator_nextValue(&iter));
                if (entry != NULL) {
                    free(entry->url);
                    free(entry);
                }
            }
            hashMap_destroy(receiver->requestedConnections.map, false, false);
        }

        //celixThreadMutex_destroy(&receiver->subscribers.mutex);
        //celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        //celixThreadMutex_destroy(&receiver->recvThread.mutex);

        nn_close(receiver->nanoMsgSocket);

        free(receiver->scope);
        free(receiver->topic);
    }
    free(receiver);
}

const char* pubsub_nanoMsgTopicReceiver_scope(pubsub_nanomsg_topic_receiver_t *receiver) {
    return receiver->scope;
}
const char* pubsub_nanoMsgTopicReceiver_topic(pubsub_nanomsg_topic_receiver_t *receiver) {
    return receiver->topic;
}

long pubsub_nanoMsgTopicReceiver_serializerSvcId(pubsub_nanomsg_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

void pubsub_nanoMsgTopicReceiver_listConnections(pubsub_nanomsg_topic_receiver_t *receiver,
                                                 std::vector<std::string> &connectedUrls,
                                                 std::vector<std::string> &unconnectedUrls) {
    std::lock_guard<std::mutex> _lock(receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_nanomsg_requested_connection_entry_t *entry = static_cast<psa_nanomsg_requested_connection_entry_t *>(hashMapIterator_nextValue(&iter));
        if (entry->connected) {
            connectedUrls.push_back(std::string(entry->url));
        } else {
            unconnectedUrls.push_back(std::string(entry->url));
        }
    }
}


void pubsub_nanoMsgTopicReceiver_connectTo(
        pubsub_nanomsg_topic_receiver_t *receiver,
        const char *url) {
    L_DEBUG("[PSA_ZMQ] TopicReceiver %s/%s connecting to zmq url %s", receiver->scope, receiver->topic, url);

    std::lock_guard<std::mutex> _lock(receiver->requestedConnections.mutex);
    psa_nanomsg_requested_connection_entry_t *entry = static_cast<psa_nanomsg_requested_connection_entry_t*>(hashMap_get(receiver->requestedConnections.map, url));
    if (entry == NULL) {
        entry = static_cast<psa_nanomsg_requested_connection_entry_t*>(calloc(1, sizeof(*entry)));
        entry->url = strndup(url, 1024*1024);
        entry->connected = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->url, entry);
    }
    if (!entry->connected) {
        int connection_id = nn_connect(receiver->nanoMsgSocket, url);
        if (connection_id >= 0) {
            entry->connected = true;
            entry->id = connection_id;
        } else {
            L_WARN("[PSA_NANOMSG] Error connecting to NANOMSG url %s. (%s)", url, strerror(errno));
        }
    }
}

void pubsub_nanoMsgTopicReceiver_disconnectFrom(pubsub_nanomsg_topic_receiver_t *receiver, const char *url) {
    L_DEBUG("[PSA ZMQ] TopicReceiver %s/%s disconnect from zmq url %s", receiver->scope, receiver->topic, url);

    std::lock_guard<std::mutex> _lock(receiver->requestedConnections.mutex);
    psa_nanomsg_requested_connection_entry_t *entry = static_cast<psa_nanomsg_requested_connection_entry_t*>(hashMap_remove(receiver->requestedConnections.map, url));
    if (entry != NULL && entry->connected) {
        if (nn_shutdown(receiver->nanoMsgSocket, entry->id) == 0) {
            entry->connected = false;
        } else {
            L_WARN("[PSA_NANOMSG] Error disconnecting from nanomsg url %s, id %d. (%s)", url, entry->id, strerror(errno));
        }
    }
    if (entry != NULL) {
        free(entry->url);
        free(entry);
    }
}

static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_nanomsg_topic_receiver_t *receiver = static_cast<pubsub_nanomsg_topic_receiver_t*>(handle);

    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
        //not the same scope. ignore
        return;
    }

    std::lock_guard<std::mutex> _lock(receiver->subscribers.mutex);
    psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMap_get(receiver->subscribers.map, (void*)bndId));
    if (entry != NULL) {
        entry->usageCount += 1;
    } else {
        //new create entry
        entry = static_cast<psa_nanomsg_subscriber_entry_t*>(calloc(1, sizeof(*entry)));
        entry->usageCount = 1;
        entry->svc = static_cast<pubsub_subscriber_t*>(svc);

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);
        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_NANOMSG] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
            free(entry);
        }
    }
}

static void pubsub_nanoMsgTopicReceiver_removeSubscriber(void *handle, void */*svc*/,
                                                         const celix_properties_t */*props*/, const celix_bundle_t *bnd) {
    pubsub_nanomsg_topic_receiver_t *receiver = static_cast<pubsub_nanomsg_topic_receiver_t*>(handle);

    long bndId = celix_bundle_getId(bnd);

    std::lock_guard<std::mutex> _lock(receiver->subscribers.mutex);
    psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMap_get(receiver->subscribers.map, (void*)bndId));
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_NANOMSG] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
        free(entry);
    }
}

static inline void processMsgForSubscriberEntry(pubsub_nanomsg_topic_receiver_t *receiver, psa_nanomsg_subscriber_entry_t* entry, const pubsub_nanmosg_msg_header_t *hdr, const char* payload, size_t payloadSize) {
    pubsub_msg_serializer_t* msgSer = static_cast<pubsub_msg_serializer_t*>(hashMap_get(entry->msgTypes, (void*)(uintptr_t)(hdr->type)));
    pubsub_subscriber_t *svc = entry->svc;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = psa_nanomsg_checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            celix_status_t status = msgSer->deserialize(msgSer, payload, payloadSize, &deserializedMsg);
            if(status == CELIX_SUCCESS) {
                bool release = false;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, NULL, &release);
                if (release) {
                    msgSer->freeMsg(msgSer->handle, deserializedMsg);
                }
            } else {
                L_WARN("[PSA_NANOMSG_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, receiver->scope, receiver->topic);
            }
        }
    } else {
        L_WARN("[PSA_NANOMSG_TR] Cannot find serializer for type id %i", hdr->type);
    }
}

static inline void processMsg(pubsub_nanomsg_topic_receiver_t *receiver, const pubsub_nanmosg_msg_header_t *hdr, const char *payload, size_t payloadSize) {
    std::lock_guard<std::mutex> _lock(receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMapIterator_nextValue(&iter));
        if (entry != NULL) {
            processMsgForSubscriberEntry(receiver, entry, hdr, payload, payloadSize);
        }
    }
}

struct Message {
    pubsub_nanmosg_msg_header_t header;
    char payload[];
};
static void* psa_nanomsg_recvThread(void *data) {
    pubsub_nanomsg_topic_receiver_t *receiver = static_cast<pubsub_nanomsg_topic_receiver_t*>(data);
    bool running{};
    {
        std::lock_guard<std::mutex> _lock(receiver->recvThread.mutex);
        running = receiver->recvThread.running;
    }
    while (running) {
        Message *msg = nullptr;
        nn_iovec iov[2];
        iov[0].iov_base = &msg;
        iov[0].iov_len = NN_MSG;

        nn_msghdr msgHdr;
        memset(&msgHdr, 0, sizeof(msgHdr));

        msgHdr.msg_iov = iov;
        msgHdr.msg_iovlen = 1;

        msgHdr.msg_control = nullptr;
        msgHdr.msg_controllen = 0;

        errno = 0;
        int recvBytes = nn_recvmsg(receiver->nanoMsgSocket, &msgHdr, 0);
        if (msg && static_cast<unsigned long>(recvBytes) >= sizeof(pubsub_nanmosg_msg_header_t)) {
            processMsg(receiver, &msg->header, msg->payload, recvBytes-sizeof(msg->header));
            nn_freemsg(msg);
        } else if (recvBytes >= 0) {
            L_ERROR("[PSA_ZMQ_TR] Error receiving nanmosg msg, size (%d) smaller than header\n", recvBytes);
        } else if (errno == EAGAIN || errno == ETIMEDOUT) {
            //nop
        } else if (errno == EINTR) {
            L_DEBUG("[PSA_ZMQ_TR] zmsg_recv interrupted");
        } else {
            L_WARN("[PSA_ZMQ_TR] Error receiving zmq message: errno %d: %s\n", errno, strerror(errno));
        }
    } // while

    return NULL;
}
