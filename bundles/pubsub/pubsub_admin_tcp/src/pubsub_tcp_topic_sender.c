/**
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
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <log_helper.h>
#include "pubsub_tcp_topic_sender.h"
#include "pubsub_tcp_handler.h"
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_common.h"
#include "pubsub_endpoint.h"
#include <uuid/uuid.h>
#include <constants.h>
#include <signal.h>

#define FIRST_SEND_DELAY_IN_SECONDS             2
#define TCP_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_tcp_topic_sender {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    uuid_t fwUUID;
    bool metricsEnabled;
    pubsub_tcpHandler_t *socketHandler;
    pubsub_tcpHandler_t *sharedSocketHandler;

    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *url;
    bool isStatic;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } thread;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_tcp_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_tcp_send_msg_entry {
    pubsub_tcp_msg_header_t header; //partially filled header (only seqnr and time needs to be updated per send)
    pubsub_msg_serializer_t *msgSer;
    celix_thread_mutex_t sendLock; //protects send & Seqnr
    int seqNr;
    struct {
        celix_thread_mutex_t mutex; //protects entries in struct
        long nrOfMessagesSend;
        long nrOfMessagesSendFailed;
        long nrOfSerializationErrors;
        struct timespec lastMessageSend;
        double averageTimeBetweenMessagesInSeconds;
        double averageSerializationTimeInSeconds;
    } metrics;
} psa_tcp_send_msg_entry_t;

typedef struct psa_tcp_bounded_service_entry {
    pubsub_tcp_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes; //key = msg type id, value = pubsub_msg_serializer_t
    hash_map_t *msgTypeIds; // key = msg name, value = msg type id
    hash_map_t *msgEntries; //key = msg type id, value = psa_tcp_send_msg_entry_t
    int getCount;
} psa_tcp_bounded_service_entry_t;

static int psa_tcp_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId);
static void *psa_tcp_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_tcp_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub_tcp_topic_sender_t *sender);
static void *psa_tcp_sendThread(void *data);
static int psa_tcp_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *msg);

pubsub_tcp_topic_sender_t *pubsub_tcpTopicSender_create(
        celix_bundle_context_t *ctx,
        log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        const celix_properties_t *topicProperties,
        pubsub_tcp_endPointStore_t *endPointStore,
        long serializerSvcId,
        pubsub_serializer_service_t *ser,
        const char *bindIP,
        const char *staticBindUrl,
        unsigned int basePort,
        unsigned int maxPort) {
    pubsub_tcp_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;
    sender->socketHandler = pubsub_tcpHandler_create(sender->logHelper);
    psa_tcp_setScopeAndTopicFilter(scope, topic, sender->scopeAndTopicFilter);
    const char *uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    sender->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_TCP_METRICS_ENABLED, PSA_TCP_DEFAULT_METRICS_ENABLED);

    /* Check if it's a static endpoint */
    bool isEndPointTypeClient = false;
    bool isEndPointTypeServer = false;
    const char *endPointType = celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_ENDPOINT_TYPE, NULL);
    if (endPointType != NULL) {
        if (strncmp(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_CLIENT, endPointType, strlen(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_CLIENT)) == 0) {
            isEndPointTypeClient = true;
        }
        if (strncmp(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_SERVER, endPointType, strlen(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_SERVER)) == 0) {
            isEndPointTypeServer = true;
        }
    }

    // When endpoint is client, use the connection urls as a key.
    const char *staticConnectUrls = ((topicProperties != NULL) && isEndPointTypeClient) ? celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_CONNECT_URLS, NULL) : NULL;

    /* When it's an endpoint share the socket with the receiver */
    if (staticConnectUrls != NULL || (isEndPointTypeServer && staticBindUrl != NULL)) {
        celixThreadMutex_lock(&endPointStore->mutex);
        sender->sharedSocketHandler = sender->socketHandler;
        pubsub_tcpHandler_t *entry = hashMap_get(endPointStore->map, staticConnectUrls);
        if (entry == NULL) {
            entry = sender->socketHandler;
            hashMap_put(endPointStore->map, (void *) (isEndPointTypeClient ? staticConnectUrls : staticBindUrl), entry);
        }
        celixThreadMutex_unlock(&endPointStore->mutex);
    }

    //setting up tcp socket for TCP TopicSender
    {
        if (staticConnectUrls != NULL) {
            // Store url for client static endpoint
            sender->url = strndup(staticConnectUrls, 1024 * 1024);
            sender->isStatic = true;
        } else if (staticBindUrl != NULL) {
            int rv = pubsub_tcpHandler_listen(sender->socketHandler, (char *) staticBindUrl);
            if (rv == -1) {
                L_WARN("Error for tcp_bind using static bind url '%s'. %s", staticBindUrl, strerror(errno));
            } else {
                sender->url = strndup(staticBindUrl, 1024 * 1024);
                sender->isStatic = true;
            }
        } else {
            int retry = 0;
            while (sender->url == NULL && retry < TCP_BIND_MAX_RETRY) {
                /* Randomized part due to same bundle publishing on different topics */
                unsigned int port = rand_range(basePort, maxPort);
                char *url = NULL;
                asprintf(&url, "tcp://%s:%u", bindIP, port);
                char *bindUrl = NULL;
                asprintf(&bindUrl, "tcp://0.0.0.0:%u", port);
                int rv = pubsub_tcpHandler_listen(sender->socketHandler, bindUrl);
                if (rv == -1) {
                    L_WARN("Error for tcp_bind using dynamic bind url '%s'. %s", bindUrl, strerror(errno));
                    free(url);
                } else {
                    sender->url = url;
                }
                retry++;
                free(bindUrl);
            }
        }
    }

    if (sender->url != NULL) {
        sender->scope = strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        celixThreadMutex_create(&sender->thread.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    if (sender->socketHandler != NULL) {
        sender->thread.running = true;
        celixThread_create(&sender->thread.thread, NULL, psa_tcp_sendThread, sender);
        char name[64];
        snprintf(name, 64, "TCP TS %s/%s", scope, topic);
        celixThread_setName(&sender->thread.thread, name);
        psa_tcp_setupTcpContext(sender->logHelper, &sender->thread.thread, topicProperties);
    }


    //register publisher services using a service factory
    if (sender->url != NULL) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_tcp_getPublisherService;
        sender->publisher.factory.ungetService = psa_tcp_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.factory = &sender->publisher.factory;
        opts.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_PUBLISHER_SERVICE_VERSION;
        opts.properties = props;

        sender->publisher.svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    if (sender->url == NULL) {
        free(sender);
        sender = NULL;
    }

    return sender;
}

void pubsub_tcpTopicSender_destroy(pubsub_tcp_topic_sender_t *sender) {
    if (sender != NULL) {
        celixThreadMutex_lock(&sender->thread.mutex);
        if (sender->thread.running) {
            sender->thread.running = false;
            celixThreadMutex_unlock(&sender->thread.mutex);
            celixThread_join(sender->thread.thread, NULL);
        }
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        celixThreadMutex_lock(&sender->boundedServices.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
                while (hashMapIterator_hasNext(&iter2)) {
                    psa_tcp_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter2);
                    celixThreadMutex_destroy(&msgEntry->metrics.mutex);
                    free(msgEntry);
                }
                hashMap_destroy(entry->msgEntries, false, false);
                free(entry);
            }
        }
        hashMap_destroy(sender->boundedServices.map, false, false);
        celixThreadMutex_unlock(&sender->boundedServices.mutex);
        celixThreadMutex_destroy(&sender->boundedServices.mutex);
        celixThreadMutex_destroy(&sender->thread.mutex);

        if ((sender->socketHandler) && (sender->sharedSocketHandler == NULL)) {
            pubsub_tcpHandler_destroy(sender->socketHandler);
            sender->socketHandler = NULL;
        }

        free(sender->scope);
        free(sender->topic);
        free(sender->url);
        free(sender);
    }
}

long pubsub_tcpTopicSender_serializerSvcId(pubsub_tcp_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

const char *pubsub_tcpTopicSender_scope(pubsub_tcp_topic_sender_t *sender) {
    return sender->scope;
}

const char *pubsub_tcpTopicSender_topic(pubsub_tcp_topic_sender_t *sender) {
    return sender->topic;
}

const char *pubsub_tcpTopicSender_url(pubsub_tcp_topic_sender_t *sender) {
    return pubsub_tcpHandler_url(sender->socketHandler);
}

bool pubsub_tcpTopicSender_isStatic(pubsub_tcp_topic_sender_t *sender) {
    return sender->isStatic;
}

void pubsub_tcpTopicSender_connectTo(pubsub_tcp_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO subscriber count -> topic info
}

void pubsub_tcpTopicSender_disconnectFrom(pubsub_tcp_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

static int psa_tcp_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId) {
    psa_tcp_bounded_service_entry_t *entry = (psa_tcp_bounded_service_entry_t *) handle;
    *msgTypeId = (unsigned int)(uintptr_t) hashMap_get(entry->msgTypeIds, msgType);
    return 0;
}

static void *psa_tcp_getPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                         const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_tcp_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_tcp_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void *) bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;
        entry->msgEntries = hashMap_create(NULL, NULL, NULL, NULL);
        entry->msgTypeIds = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t *) requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                hash_map_entry_t *hashMapEntry = hashMapIterator_nextEntry(&iter);
                void *key = hashMapEntry_getKey(hashMapEntry);
                psa_tcp_send_msg_entry_t *sendEntry = calloc(1, sizeof(*sendEntry));
                sendEntry->msgSer = hashMapEntry_getValue(hashMapEntry);
                sendEntry->header.type = (int32_t) sendEntry->msgSer->msgId;
                int major;
                int minor;
                version_getMajor(sendEntry->msgSer->msgVersion, &major);
                version_getMinor(sendEntry->msgSer->msgVersion, &minor);
                sendEntry->header.major = (int8_t) major;
                sendEntry->header.minor = (int8_t) minor;
                uuid_copy(sendEntry->header.originUUID, sender->fwUUID);
                celixThreadMutex_create(&sendEntry->metrics.mutex, NULL);
                hashMap_put(entry->msgEntries, key, sendEntry);
                hashMap_put(entry->msgTypeIds, strndup(sendEntry->msgSer->msgName, 1024), (void *)(uintptr_t) sendEntry->msgSer->msgId);
            }
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_tcp_localMsgTypeIdForMsgType;
            entry->service.send = psa_tcp_topicPublicationSend;
            hashMap_put(sender->boundedServices.map, (void *) bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for TCP TopicSender %s/%s", sender->scope, sender->topic);
        }
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return &entry->service;
}

static void psa_tcp_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                          const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_tcp_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_tcp_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void *) bndId);
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(sender->boundedServices.map, (void *) bndId);
        int rc = sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }

        hash_map_iterator_t iter = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter);
            celixThreadMutex_destroy(&msgEntry->metrics.mutex);
            free(msgEntry);
        }
        hashMap_destroy(entry->msgEntries, false, false);

        hashMap_destroy(entry->msgTypeIds, true, false);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static void *psa_tcp_sendThread(void *data) {
    pubsub_tcp_topic_sender_t *sender = data;

    celixThreadMutex_lock(&sender->thread.mutex);
    bool running = sender->thread.running;
    celixThreadMutex_unlock(&sender->thread.mutex);

    while (running) {
        pubsub_tcpHandler_handler(sender->socketHandler);

        celixThreadMutex_lock(&sender->thread.mutex);
        running = sender->thread.running;
        celixThreadMutex_unlock(&sender->thread.mutex);

    } // while
    return NULL;
}

pubsub_admin_sender_metrics_t *pubsub_tcpTopicSender_metrics(pubsub_tcp_topic_sender_t *sender) {
    pubsub_admin_sender_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->topic);
    celixThreadMutex_lock(&sender->boundedServices.mutex);
    size_t count = 0;
    hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter2)) {
            hashMapIterator_nextValue(&iter2);
            count += 1;
        }
    }

    result->msgMetrics = calloc(count, sizeof(*result));

    iter = hashMapIterator_construct(sender->boundedServices.map);
    int i = 0;
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter2)) {
            psa_tcp_send_msg_entry_t *mEntry = hashMapIterator_nextValue(&iter2);
            celixThreadMutex_lock(&mEntry->metrics.mutex);
            result->msgMetrics[i].nrOfMessagesSend = mEntry->metrics.nrOfMessagesSend;
            result->msgMetrics[i].nrOfMessagesSendFailed = mEntry->metrics.nrOfMessagesSendFailed;
            result->msgMetrics[i].nrOfSerializationErrors = mEntry->metrics.nrOfSerializationErrors;
            result->msgMetrics[i].averageSerializationTimeInSeconds = mEntry->metrics.averageSerializationTimeInSeconds;
            result->msgMetrics[i].averageTimeBetweenMessagesInSeconds = mEntry->metrics.averageTimeBetweenMessagesInSeconds;
            result->msgMetrics[i].lastMessageSend = mEntry->metrics.lastMessageSend;
            result->msgMetrics[i].bndId = entry->bndId;
            result->msgMetrics[i].typeId = mEntry->header.type;
            snprintf(result->msgMetrics[i].typeFqn, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", mEntry->msgSer->msgName);
            i += 1;
            celixThreadMutex_unlock(&mEntry->metrics.mutex);
        }
    }

    celixThreadMutex_unlock(&sender->boundedServices.mutex);
    result->nrOfmsgMetrics = (int) count;
    return result;
}

static int psa_tcp_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *inMsg) {
    int status = CELIX_SUCCESS;
    psa_tcp_bounded_service_entry_t *bound = handle;
    pubsub_tcp_topic_sender_t *sender = bound->parent;
    bool monitor = sender->metricsEnabled;

    psa_tcp_send_msg_entry_t *entry = hashMap_get(bound->msgEntries, (void *) (uintptr_t)(msgTypeId));

    //metrics updates
    struct timespec sendTime;
    struct timespec serializationStart;
    struct timespec serializationEnd;
    //int unknownMessageCountUpdate = 0;
    int sendErrorUpdate = 0;
    int serializationErrorUpdate = 0;
    int sendCountUpdate = 0;

    if (entry != NULL) {
        delay_first_send_for_late_joiners(sender);
        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationStart);
        }

        void *serializedOutput = NULL;
        size_t serializedOutputLen = 0;
        status = entry->msgSer->serialize(entry->msgSer->handle, inMsg, &serializedOutput, &serializedOutputLen);

        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationEnd);
        }

        if (status == CELIX_SUCCESS /*ser ok*/) {
            //celixThreadMutex_lock(&entry->sendLock);
            pubsub_tcp_msg_header_t msg_hdr = entry->header;
            msg_hdr.seqNr = -1;
            msg_hdr.sendtimeSeconds = 0;
            msg_hdr.sendTimeNanoseconds = 0;
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &sendTime);
                msg_hdr.sendtimeSeconds = (int64_t) sendTime.tv_sec;
                msg_hdr.sendTimeNanoseconds = (int64_t) sendTime.tv_nsec;
                msg_hdr.seqNr = entry->seqNr++;
            }

            errno = 0;
            bool sendOk = true;
            {
                int rc = pubsub_tcpHandler_write(sender->socketHandler, &msg_hdr, serializedOutput, serializedOutputLen, 0);
                if (rc < 0) {
                    status = -1;
                    sendOk = false;
                }
                free(serializedOutput);
            }

            //celixThreadMutex_unlock(&entry->sendLock);
            if (sendOk) {
                sendCountUpdate = 1;
            } else {
                sendErrorUpdate = 1;
                L_WARN("[PSA_TCP_TS] Error sending tcp. %s", strerror(errno));
            }
        } else {
            serializationErrorUpdate = 1;
            L_WARN("[PSA_TCP_TS] Error serialize message of type %s for scope/topic %s/%s", entry->msgSer->msgName,
                   sender->scope, sender->topic);
        }
    } else {
        //unknownMessageCountUpdate = 1;
        status = CELIX_SERVICE_EXCEPTION;
        L_WARN("[PSA_TCP_TS] Error cannot serialize message with msg type id %i for scope/topic %s/%s", msgTypeId,
               sender->scope, sender->topic);
    }


    if (monitor && entry != NULL) {
        celixThreadMutex_lock(&entry->metrics.mutex);

        long n = entry->metrics.nrOfMessagesSend + entry->metrics.nrOfMessagesSendFailed;
        double diff = celix_difftime(&serializationStart, &serializationEnd);
        double average = (entry->metrics.averageSerializationTimeInSeconds * n + diff) / (n + 1);
        entry->metrics.averageSerializationTimeInSeconds = average;

        if (entry->metrics.nrOfMessagesSend > 2) {
            diff = celix_difftime(&entry->metrics.lastMessageSend, &sendTime);
            n = entry->metrics.nrOfMessagesSend;
            average = (entry->metrics.averageTimeBetweenMessagesInSeconds * n + diff) / (n + 1);
            entry->metrics.averageTimeBetweenMessagesInSeconds = average;
        }

        entry->metrics.lastMessageSend = sendTime;
        entry->metrics.nrOfMessagesSend += sendCountUpdate;
        entry->metrics.nrOfMessagesSendFailed += sendErrorUpdate;
        entry->metrics.nrOfSerializationErrors += serializationErrorUpdate;

        celixThreadMutex_unlock(&entry->metrics.mutex);
    }

    return status;
}

static void delay_first_send_for_late_joiners(pubsub_tcp_topic_sender_t *sender) {

    static bool firstSend = true;

    if (firstSend) {
        L_INFO("PSA_TCP_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max) {
    double scaled = ((double) random()) / ((double) RAND_MAX);
    return (unsigned int) ((max - min + 1) * scaled + min);
}
