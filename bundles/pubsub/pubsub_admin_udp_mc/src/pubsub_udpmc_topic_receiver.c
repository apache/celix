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
#include "pubsub_udpmc_topic_receiver.h"
#include "pubsub_psa_udpmc_constants.h"
#include "large_udp.h"
#include "pubsub_udpmc_common.h"

#define MAX_EPOLL_EVENTS        10
#define RECV_THREAD_TIMEOUT     5
#define UDP_BUFFER_SIZE         65535
#define MAX_UDP_SESSIONS        16

struct pubsub_udpmc_topic_receiver {
    celix_bundle_context_t *ctx;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char* ifIpAddress;
    largeUdp_pt largeUdpHandle;
    int topicEpollFd; // EPOLL filedescriptor where the sockets are registered.

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = socketAddress, value = psa_udpmc_requested_connection_entry_t*
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_udpmc_subscriber_entry_t
    } subscribers;
};

typedef struct psa_udpmc_requested_connection_entry {
    char *key;
    char *socketAddress;
    long socketPort;
    bool connected;
    int recvSocket;
} psa_udpmc_requested_connection_entry_t;

typedef struct psa_udpmc_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //map from serializer svc
    pubsub_subscriber_t *svc;
} psa_udpmc_subscriber_entry_t;


typedef struct pubsub_msg{
    pubsub_msg_header_t *header;
    char* payload;
    unsigned int payloadSize;
} pubsub_msg_t;

static void pubsub_udpmcTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_udpmcTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void psa_udpmc_processMsg(pubsub_udpmc_topic_receiver_t *receiver, pubsub_udp_msg_t *msg);
static void* psa_udpmc_recvThread(void * data);


pubsub_udpmc_topic_receiver_t* pubsub_udpmcTopicReceiver_create(celix_bundle_context_t *ctx,
                                                                const char *scope,
                                                                const char *topic,
                                                                const char *ifIP,
                                                                long serializerSvcId,
                                                                pubsub_serializer_service_t *serializer) {
    pubsub_udpmc_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    receiver->scope = strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    receiver->ifIpAddress = strndup(ifIP, 1024 * 1024);
    receiver->recvThread.running = true;
    receiver->largeUdpHandle = largeUdp_create(MAX_UDP_SESSIONS);
    receiver->topicEpollFd = epoll_create1(0);


    celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
    celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
    celixThreadMutex_create(&receiver->recvThread.mutex, NULL);

    receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
    receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    //track subscribers
    {
        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size+1];
        snprintf(buf, (size_t)size+1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_udpmcTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_udpmcTopicReceiver_removeSubscriber;

        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    celixThread_create(&receiver->recvThread.thread, NULL, psa_udpmc_recvThread, receiver);

    return receiver;
}

void pubsub_udpmcTopicReceiver_destroy(pubsub_udpmc_topic_receiver_t *receiver) {
    if (receiver != NULL) {
        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        receiver->recvThread.running = false;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
        celixThread_join(receiver->recvThread.thread, NULL);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udpmc_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                free(entry->socketAddress);
                free(entry->key);
                free(entry);
            }
        }
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
        hashMap_destroy(receiver->requestedConnections.map, false, false);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udpmc_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                if (receiver->serializer != NULL && entry->msgTypes != NULL) {
                    receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                }
                free(entry);
            }
        }
        celixThreadMutex_unlock(&receiver->subscribers.mutex);
        hashMap_destroy(receiver->subscribers.map, false, false);



        celixThreadMutex_destroy(&receiver->subscribers.mutex);
        celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        celixThreadMutex_destroy(&receiver->recvThread.mutex);

        largeUdp_destroy(receiver->largeUdpHandle);

        free(receiver->scope);
        free(receiver->topic);
        free(receiver->ifIpAddress);
    }
    free(receiver);
}

const char* pubsub_udpmcTopicReceiver_scope(pubsub_udpmc_topic_receiver_t *receiver) {
    return receiver->scope;
}
const char* pubsub_udpmcTopicReceiver_topic(pubsub_udpmc_topic_receiver_t *receiver) {
    return receiver->topic;
}

long pubsub_udpmcTopicReceiver_serializerSvcId(pubsub_udpmc_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

void pubsub_udpmcTopicReceiver_connectTo(
        pubsub_udpmc_topic_receiver_t *receiver,
        const char *socketAddress,
        long socketPort) {
    printf("[PSA UDPMC] TopicReceiver %s/%s connect to socket address = %s:%li\n", receiver->scope, receiver->topic, socketAddress, socketPort);

    int len = snprintf(NULL, 0, "%s:%li", socketAddress, socketPort);
    char *key = calloc((size_t)len+1, sizeof(char));
    snprintf(key, (size_t)len+1, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udpmc_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->key = key;
        entry->socketAddress = strndup(socketAddress, 1024 * 1024);
        entry->socketPort = socketPort;
        entry->connected = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->key, entry);
    } else {
        free(key);
    }
    if (!entry->connected) {
        int rc  = 0;
        entry->recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (entry->recvSocket >= 0) {
            int reuse = 1;
            rc = setsockopt(entry->recvSocket, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse));
        }
        if (entry->recvSocket >= 0 && rc >= 0) {
            struct ip_mreq mc_addr;
            mc_addr.imr_multiaddr.s_addr = inet_addr(entry->socketAddress);
            mc_addr.imr_interface.s_addr = inet_addr(receiver->ifIpAddress);
            printf("Adding MC %s at interface %s\n", entry->socketAddress, receiver->ifIpAddress);
            rc = setsockopt(entry->recvSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mc_addr, sizeof(mc_addr));
        }
        if (entry->recvSocket >= 0 && rc >= 0) {
            struct sockaddr_in mcListenAddr;
            mcListenAddr.sin_family = AF_INET;
            mcListenAddr.sin_addr.s_addr = INADDR_ANY;
            mcListenAddr.sin_port = htons((uint16_t )entry->socketPort);
            rc = bind(entry->recvSocket, (struct sockaddr*)&mcListenAddr, sizeof(mcListenAddr));
        }
        if (entry->recvSocket >= 0 && rc >= 0) {
            struct epoll_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLIN;
            ev.data.fd = entry->recvSocket;
            rc = epoll_ctl(receiver->topicEpollFd, EPOLL_CTL_ADD, entry->recvSocket, &ev);
        }

        if (entry->recvSocket < 0 || rc < 0) {
            fprintf(stderr, "[PSA UDPMC] Error connecting TopicReceiver %s/%s to %s:%li. (%s)\n", receiver->scope, receiver->topic, socketAddress, socketPort, strerror(errno));
        } else {
            entry->connected = true;
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

void pubsub_udpmcTopicReceiver_disconnectFrom(pubsub_udpmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort) {
    printf("[PSA UDPMC] TopicReceiver %s/%s disconnect from socket address = %s:%li\n", receiver->scope, receiver->topic, socketAddress, socketPort);

    int len = snprintf(NULL, 0, "%s:%li", socketAddress, socketPort);
    char *key = calloc((size_t)len+1, sizeof(char));
    snprintf(key, (size_t)len+1, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udpmc_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, key);
    free(key);
    if (entry != NULL && entry->connected) {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        int rc = epoll_ctl(receiver->topicEpollFd, EPOLL_CTL_DEL, entry->recvSocket, &ev);
        if (rc < 0) {
            fprintf(stderr, "[PSA UDPMC] Error disconnecting TopicReceiver %s/%s to %s:%li.\n%s", receiver->scope, receiver->topic, socketAddress, socketPort, strerror(errno));
        }
    }
    if (entry != NULL) {
        free(entry->key);
        free(entry->socketAddress);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_udpmcTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_udpmc_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
        //not the same scope. ignore
        return;
    }

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_udpmc_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
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
            free(entry);
            fprintf(stderr, "Cannot find serializer for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_udpmcTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_udpmc_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_udpmc_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc =  receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            fprintf(stderr, "Cannot find serializer for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void* psa_udpmc_recvThread(void * data) {
    pubsub_udpmc_topic_receiver_t *receiver = data;

    struct epoll_event events[MAX_EPOLL_EVENTS];

    celixThreadMutex_lock(&receiver->recvThread.mutex);
    bool running = receiver->recvThread.running;
    celixThreadMutex_unlock(&receiver->recvThread.mutex);

    while (running) {
        int nfds = epoll_wait(receiver->topicEpollFd, events, MAX_EPOLL_EVENTS, RECV_THREAD_TIMEOUT * 1000);
        int i;
        for(i = 0; i < nfds; i++ ) {
            unsigned int index;
            unsigned int size;
            if(largeUdp_dataAvailable(receiver->largeUdpHandle, events[i].data.fd, &index, &size) == true) {
                // Handle data
                pubsub_udp_msg_t *udpMsg = NULL;
                if(largeUdp_read(receiver->largeUdpHandle, index, (void**)&udpMsg, size) != 0) {
                    printf("[PSA_UDPMC]: ERROR largeUdp_read with index %d\n", index);
                    continue;
                }

                psa_udpmc_processMsg(receiver, udpMsg);

                free(udpMsg);
            }
        }
        celixThreadMutex_lock(&receiver->recvThread.mutex);
        running = receiver->recvThread.running;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
    }

    return NULL;
}

static void psa_udpmc_processMsg(pubsub_udpmc_topic_receiver_t *receiver, pubsub_udp_msg_t *msg) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_udpmc_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);

        pubsub_msg_serializer_t *msgSer = NULL;
        if (entry->msgTypes != NULL) {
            msgSer = hashMap_get(entry->msgTypes, (void *) (uintptr_t) msg->header.type);
        }
        if (msgSer == NULL) {
            printf("[PSA_UDPMC] Serializer not available for message %d.\n",msg->header.type);
        } else{
            void *msgInst = NULL;
            bool validVersion = psa_udpmc_checkVersion(msgSer->msgVersion, &msg->header);

            if(validVersion){

                celix_status_t status = msgSer->deserialize(msgSer, (const void *) msg->payload, 0, &msgInst);

                if (status == CELIX_SUCCESS) {
                    bool release = true;
                    pubsub_multipart_callbacks_t mp_callbacks;
                    mp_callbacks.handle = receiver;
                    mp_callbacks.localMsgTypeIdForMsgType = psa_udpmc_localMsgTypeIdForMsgType;
                    mp_callbacks.getMultipart = NULL;

                    pubsub_subscriber_t *svc = entry->svc;
                    svc->receive(svc->handle, msgSer->msgName, msg->header.type, msgInst, &mp_callbacks, &release);

                    if(release){
                        msgSer->freeMsg(msgSer,msgInst);
                    }
                }
                else{
                    printf("[PSA_UDPMC] Cannot deserialize msgType %s.\n",msgSer->msgName);
                }

            }
            else{
                int major=0,minor=0;
                version_getMajor(msgSer->msgVersion,&major);
                version_getMinor(msgSer->msgVersion,&minor);
                printf("[PSA_UDPMC] Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
                       msgSer->msgName,major,minor,msg->header.major,msg->header.minor);
            }

        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}
