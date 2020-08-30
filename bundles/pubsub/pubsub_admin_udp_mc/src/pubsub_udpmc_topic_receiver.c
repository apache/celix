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
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
#include <assert.h>
#include <pubsub_endpoint.h>
#include <arpa/inet.h>
#include <celix_log_helper.h>
#include <pubsub_utils.h>
#include <celix_api.h>
#include "pubsub_udpmc_topic_receiver.h"
#include "pubsub_psa_udpmc_constants.h"
#include "large_udp.h"
#include "pubsub_udpmc_common.h"

#define MAX_EVENTS        10
#define RECV_THREAD_TIMEOUT     5
#define UDP_BUFFER_SIZE         65535
#define MAX_UDP_SESSIONS        16

#define L_DEBUG(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(receiver->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_udpmc_topic_receiver {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char* ifIpAddress;
    largeUdp_t *largeUdpHandle;
    int topicEpollFd; // EPOLL filedescriptor where the sockets are registered.

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = socketAddress, value = psa_udpmc_requested_connection_entry_t*
        bool allConnected; //true if all requestedConnectection are connected
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_udpmc_subscriber_entry_t
        bool allInitialized;
    } subscribers;
};

typedef struct psa_udpmc_requested_connection_entry {
    char *key;
    char *socketAddress;
    long socketPort;
    int recvSocket;

    bool connected;
    bool statically; //true if the connection is statically configured through the topic properties.
} psa_udpmc_requested_connection_entry_t;

typedef struct psa_udpmc_subscriber_entry {
    hash_map_t *msgTypes; //map from serializer svc
    hash_map_t *subscriberServices; //key = servide id, value = pubsub_subscriber_t*
    bool initialized; //true if the init function is called through the receive thread
} psa_udpmc_subscriber_entry_t;

typedef struct pubsub_udp_msg {
    pubsub_udp_msg_header_t header;
    unsigned int payloadSize;
    char payload[];
} pubsub_udp_msg_t;

static void pubsub_udpmcTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_udpmcTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void psa_udpmc_processMsg(pubsub_udpmc_topic_receiver_t *receiver, pubsub_udp_msg_t *msg);
static void* psa_udpmc_recvThread(void * data);
static void psa_udpmc_connectToAllRequestedConnections(pubsub_udpmc_topic_receiver_t *receiver);
static void psa_udpmc_initializeAllSubscribers(pubsub_udpmc_topic_receiver_t *receiver);

pubsub_udpmc_topic_receiver_t* pubsub_udpmcTopicReceiver_create(celix_bundle_context_t *ctx,
                                                                celix_log_helper_t *logHelper,
                                                                const char *scope,
                                                                const char *topic,
                                                                const char *ifIP,
                                                                const celix_properties_t *topicProperties,
                                                                long serializerSvcId,
                                                                pubsub_serializer_service_t *serializer) {
    pubsub_udpmc_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;
    receiver->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
    receiver->topic = strndup(topic, 1024 * 1024);
    receiver->ifIpAddress = strndup(ifIP, 1024 * 1024);
    receiver->recvThread.running = true;
    receiver->largeUdpHandle = largeUdp_create(MAX_UDP_SESSIONS);
#if defined(__APPLE__)
    receiver->topicEpollFd = kqueue();
#else
    receiver->topicEpollFd = epoll_create1(0);
#endif
    celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
    celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
    celixThreadMutex_create(&receiver->recvThread.mutex, NULL);

    receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
    receiver->subscribers.allInitialized = false;
    receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    receiver->requestedConnections.allConnected = false;

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

    const char *staticConnects = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDPMC_STATIC_CONNECT_URLS_FOR, topic, scope);
    if(staticConnects == NULL) {
        staticConnects = celix_properties_get(topicProperties, PUBSUB_UDPMC_STATIC_CONNECT_SOCKET_ADDRESSES, NULL);
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
                psa_udpmc_requested_connection_entry_t *entry = calloc(1, sizeof(*entry));
                entry->key = key;
                entry->socketAddress = sockAddr;
                entry->socketPort = sockPort;
                entry->connected = false;
                entry->statically = true;
                hashMap_put(receiver->requestedConnections.map, (void *) entry->key, entry);
            } else {
                L_WARN("[PSA_UDPMC_TR] Invalid static socket address %s", addr);
                free(key);
                free(sockAddr);
            }
        }
        free(copy);
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
                hashMap_destroy(entry->subscriberServices, false, false);
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

void pubsub_udpmcTopicReceiver_connectTo(pubsub_udpmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort) {
    printf("[PSA UDPMC] TopicReceiver %s/%s connect to socket address = %s:%li\n", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, socketAddress, socketPort);

    char *key = NULL;
    asprintf(&key, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udpmc_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, key);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->key = key;
        entry->socketAddress = strndup(socketAddress, 1024 * 1024);
        entry->socketPort = socketPort;
        entry->connected = false;
        entry->statically = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->key, entry);
        receiver->requestedConnections.allConnected = false;
    } else {
        free(key);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

    psa_udpmc_connectToAllRequestedConnections(receiver);
}

void pubsub_udpmcTopicReceiver_disconnectFrom(pubsub_udpmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort) {
    printf("[PSA UDPMC] TopicReceiver %s/%s disconnect from socket address = %s:%li\n", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, socketAddress, socketPort);

    int len = snprintf(NULL, 0, "%s:%li", socketAddress, socketPort);
    char *key = calloc((size_t)len+1, sizeof(char));
    snprintf(key, (size_t)len+1, "%s:%li", socketAddress, socketPort);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_udpmc_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, key);
    free(key);
    if (entry != NULL && entry->connected) {
#if defined(__APPLE__)
        struct kevent ev;
        EV_SET (&ev, entry->recvSocket, EVFILT_READ, EV_DELETE, 0, 0, 0);
        int rc = kevent (receiver->topicEpollFd, &ev, 1, NULL, 0, NULL);
#else
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        int rc = epoll_ctl(receiver->topicEpollFd, EPOLL_CTL_DEL, entry->recvSocket, &ev);
#endif
        if (rc < 0) {
            fprintf(stderr, "[PSA UDPMC] Error disconnecting TopicReceiver %s/%s to %s:%li.\n%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, socketAddress, socketPort, strerror(errno));
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
    psa_udpmc_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        hashMap_put(entry->subscriberServices, (void*)svcId, svc);
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->subscriberServices = hashMap_create(NULL, NULL, NULL, NULL);
        entry->initialized = false;
        receiver->subscribers.allInitialized = false;
        hashMap_put(entry->subscriberServices, (void*)svcId, svc);

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);
        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            free(entry);
            fprintf(stderr, "Cannot find serializer for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_udpmcTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_udpmc_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_udpmc_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        hashMap_remove(entry->subscriberServices, (void*)svcId);
    }
    if (entry != NULL && hashMap_size(entry->subscriberServices) == 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc =  receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            fprintf(stderr, "Cannot find serializer for TopicReceiver %s/%s", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic);
        }
        hashMap_destroy(entry->subscriberServices, false, false);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void* psa_udpmc_recvThread(void * data) {
    pubsub_udpmc_topic_receiver_t *receiver = data;

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
            psa_udpmc_connectToAllRequestedConnections(receiver);
        }
        if (!allInitialized) {
            psa_udpmc_initializeAllSubscribers(receiver);
        }

        unsigned int timeout = RECV_THREAD_TIMEOUT * 1000;
#if defined(__APPLE__)
        struct kevent events[MAX_EVENTS];
        struct timespec ts = {timeout / 1000, (timeout  % 1000) * 1000000};
        int nfds = kevent (receiver->topicEpollFd, NULL, 0, &events[0], MAX_EVENTS, timeout ? &ts : NULL);
#else
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(receiver->topicEpollFd, events, MAX_EVENTS, timeout);
#endif
        int i;
        for (i = 0; i < nfds; i++ ) {
            unsigned int index;
            unsigned int size;
#if defined(__APPLE__)
            int fd = events[i].ident;
#else
            int fd = events[i].data.fd;
#endif
            if (largeUdp_dataAvailable(receiver->largeUdpHandle, fd, &index, &size) == true) {
                // Handle data
                pubsub_udp_msg_t *udpMsg = NULL;
                if (largeUdp_read(receiver->largeUdpHandle, index, (void**) &udpMsg, size) != 0) {
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

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        allConnected = receiver->requestedConnections.allConnected;
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        allInitialized = receiver->subscribers.allInitialized;
        celixThreadMutex_unlock(&receiver->subscribers.mutex);
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
            L_WARN("[PSA_UDPMC] Serializer not available for message %d.\n", msg->header.type);
        } else {
            void *msgInst = NULL;
            bool validVersion = psa_udpmc_checkVersion(msgSer->msgVersion, &msg->header);

            if (validVersion) {
                struct iovec deSerializeBuffer;
                deSerializeBuffer.iov_base = msg->payload;
                deSerializeBuffer.iov_len  = 0;
                celix_status_t status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 0, &msgInst);

                if (status == CELIX_SUCCESS) {
                    hash_map_iterator_t iter2 = hashMapIterator_construct(entry->subscriberServices);
                    bool release = true;
                    while (hashMapIterator_hasNext(&iter2)) {
                        pubsub_subscriber_t *svc = hashMapIterator_nextValue(&iter2);
                        svc->receive(svc->handle, msgSer->msgName, msg->header.type, msgInst, NULL, &release);
                        if (!release && hashMapIterator_hasNext(&iter2)) {
                            //receive function has taken ownership and still more receive function to come ..
                            //deserialize again for new message
                            status = msgSer->deserialize(msgSer->handle, &deSerializeBuffer, 0, &msgInst);
                            if (status != CELIX_SUCCESS) {
                                L_WARN("[PSA_UDPMC] Cannot deserialize msgType %s.\n",msgSer->msgName);
                                break;
                            }
                            release = true;
                        }
                    }
                    if (release) {
                        msgSer->freeDeserializeMsg(msgSer->handle, msgInst);
                    }
                } else {
                    L_WARN("[PSA_UDPMC] Cannot deserialize msgType %s.\n",msgSer->msgName);
                }

            } else {
                int major = 0, minor = 0;
                version_getMajor(msgSer->msgVersion, &major);
                version_getMinor(msgSer->msgVersion, &minor);
                L_WARN("[PSA_UDPMC] Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
                       msgSer->msgName,major,minor,msg->header.major,msg->header.minor);
            }

        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

void pubsub_udpmcTopicReceiver_listConnections(pubsub_udpmc_topic_receiver_t *receiver, celix_array_list_t *connections) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_udpmc_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        char *url = NULL;
        const char *post = entry->statically ? " (static)" : "";
        asprintf(&url, "%s:%li%s", entry->socketAddress, entry->socketPort, post);
        celix_arrayList_add(connections, url);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static bool psa_udpmc_connectToEntry(pubsub_udpmc_topic_receiver_t *receiver, psa_udpmc_requested_connection_entry_t *entry) {
    bool connected = true;
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
        L_INFO("[PSA_UDPMC_TR] Add MC %s at interface %s\n", entry->socketAddress, receiver->ifIpAddress);
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
#if defined(__APPLE__)
      struct kevent ev;
      EV_SET (&ev, entry->recvSocket, EVFILT_READ, EV_ADD | EV_ENABLE , 0, 0, 0);
      rc = kevent (receiver->topicEpollFd, &ev, 1, NULL, 0, NULL);
#else
      struct epoll_event ev;
      memset(&ev, 0, sizeof(ev));
      ev.events = EPOLLIN;
      ev.data.fd = entry->recvSocket;
      rc = epoll_ctl(receiver->topicEpollFd, EPOLL_CTL_ADD, entry->recvSocket, &ev);
#endif
    }

    if (entry->recvSocket < 0 || rc < 0) {
        L_WARN("[PSA UDPMC] Error connecting TopicReceiver %s/%s to %s:%li. (%s)\n", receiver->scope == NULL ? "(null)" : receiver->scope, receiver->topic, entry->socketAddress, entry->socketPort, strerror(errno));
        connected = false;
    }
    return connected;
}


static void psa_udpmc_connectToAllRequestedConnections(pubsub_udpmc_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    if (!receiver->requestedConnections.allConnected) {
        bool allConnected = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udpmc_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!entry->connected) {
                if (psa_udpmc_connectToEntry(receiver, entry)) {
                    entry->connected = true;
                } else {
                    L_WARN("[PSA_UDPMC_TR] Error connecting to address %s. (%s)", entry->socketAddress, strerror(errno));
                    allConnected = false;
                }
            }
        }
        receiver->requestedConnections.allConnected = allConnected;
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void psa_udpmc_initializeAllSubscribers(pubsub_udpmc_topic_receiver_t *receiver) {
    celixThreadMutex_lock(&receiver->subscribers.mutex);
    if (!receiver->subscribers.allInitialized) {
        bool allInitialized = true;
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_udpmc_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
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