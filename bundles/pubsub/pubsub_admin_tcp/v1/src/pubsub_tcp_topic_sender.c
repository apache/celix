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
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <celix_log_helper.h>
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_topic_sender.h"
#include "pubsub_tcp_handler.h"
#include "pubsub_tcp_common.h"
#include <uuid/uuid.h>
#include "celix_constants.h"
#include <pubsub_utils.h>
#include "pubsub_interceptors_handler.h"

#define TCP_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_tcp_topic_sender {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    uuid_t fwUUID;
    bool metricsEnabled;
    pubsub_tcpHandler_t *socketHandler;
    pubsub_tcpHandler_t *sharedSocketHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;

    char *scope;
    char *topic;
    char *url;
    bool isStatic;
    bool isPassive;
    bool verbose;
    unsigned long send_delay;

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
    uint32_t type; //msg type id (hash of fqn)
    uint8_t major;
    uint8_t minor;
    unsigned char originUUID[16];
    pubsub_msg_serializer_t *msgSer;
    pubsub_protocol_service_t *protSer;
    struct iovec *serializedIoVecOutput;
    size_t serializedIoVecOutputLen;
    unsigned int seqNr;
    struct {
        celix_thread_mutex_t mutex; //protects entries in struct
        unsigned long nrOfMessagesSend;
        unsigned long nrOfMessagesSendFailed;
        unsigned long nrOfSerializationErrors;
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

static void *psa_tcp_getPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                         const celix_properties_t *svcProperties);

static void psa_tcp_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                          const celix_properties_t *svcProperties);

static void delay_first_send_for_late_joiners(pubsub_tcp_topic_sender_t *sender);

static int
psa_tcp_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *msg, celix_properties_t *metadata);

pubsub_tcp_topic_sender_t *pubsub_tcpTopicSender_create(
    celix_bundle_context_t *ctx,
    celix_log_helper_t *logHelper,
    const char *scope,
    const char *topic,
    const celix_properties_t *topicProperties,
    pubsub_tcp_endPointStore_t *handlerStore,
    long serializerSvcId,
    pubsub_serializer_service_t *ser,
    long protocolSvcId,
    pubsub_protocol_service_t *protocol) {
    pubsub_tcp_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;
    sender->protocolSvcId = protocolSvcId;
    sender->protocol = protocol;
    const char *uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    pubsubInterceptorsHandler_create(ctx, scope, topic, &sender->interceptorsHandler);
    sender->isPassive = false;
    sender->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_TCP_METRICS_ENABLED, PSA_TCP_DEFAULT_METRICS_ENABLED);
    char *urls = NULL;
    const char *ip = celix_bundleContext_getProperty(ctx, PUBSUB_TCP_PSA_IP_KEY, NULL);
    const char *discUrl = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_STATIC_BIND_URL_FOR, topic, scope);
    const char *isPassive = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_PASSIVE_ENABLED, topic, scope);
    const char *passiveKey = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_TCP_PASSIVE_SELECTION_KEY, topic, scope);

    if (isPassive) {
        sender->isPassive = psa_tcp_isPassive(isPassive);
    }
    if (topicProperties != NULL) {
        if (discUrl == NULL) {
            discUrl = celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_DISCOVER_URL, NULL);
        }
        if (isPassive == NULL) {
            sender->isPassive = celix_properties_getAsBool(topicProperties, PUBSUB_TCP_PASSIVE_CONFIGURED, false);
        }
        if (passiveKey == NULL) {
            passiveKey = celix_properties_get(topicProperties, PUBSUB_TCP_PASSIVE_KEY, NULL);
        }
    }
    /* When it's an endpoint share the socket with the receiver */
    if (passiveKey != NULL) {
        celixThreadMutex_lock(&handlerStore->mutex);
        pubsub_tcpHandler_t *entry = hashMap_get(handlerStore->map, passiveKey);
        if (entry == NULL) {
            if (sender->socketHandler == NULL)
                sender->socketHandler = pubsub_tcpHandler_create(sender->protocol, sender->logHelper);
            entry = sender->socketHandler;
            sender->sharedSocketHandler = sender->socketHandler;
            hashMap_put(handlerStore->map, (void *) passiveKey, entry);
        } else {
            sender->socketHandler = entry;
            sender->sharedSocketHandler = entry;
        }
        celixThreadMutex_unlock(&handlerStore->mutex);
    } else {
        sender->socketHandler = pubsub_tcpHandler_create(sender->protocol, sender->logHelper);
    }

    if ((sender->socketHandler != NULL) && (topicProperties != NULL)) {
        long prio = celix_properties_getAsLong(topicProperties, PUBSUB_TCP_THREAD_REALTIME_PRIO, -1L);
        const char *sched = celix_properties_get(topicProperties, PUBSUB_TCP_THREAD_REALTIME_SCHED, NULL);
        long retryCnt = celix_properties_getAsLong(topicProperties, PUBSUB_TCP_PUBLISHER_RETRY_CNT_KEY, PUBSUB_TCP_PUBLISHER_RETRY_CNT_DEFAULT);
        double sendTimeout = celix_properties_getAsDouble(topicProperties, PUBSUB_TCP_PUBLISHER_SNDTIMEO_KEY, PUBSUB_TCP_PUBLISHER_SNDTIMEO_DEFAULT);
        long maxMsgSize = celix_properties_getAsLong(topicProperties, PSA_TCP_MAX_MESSAGE_SIZE, PSA_TCP_DEFAULT_MAX_MESSAGE_SIZE);
        long timeout = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_TIMEOUT, PSA_TCP_DEFAULT_TIMEOUT);
        sender->send_delay = celix_bundleContext_getPropertyAsLong(ctx,  PUBSUB_UTILS_PSA_SEND_DELAY, PUBSUB_UTILS_PSA_DEFAULT_SEND_DELAY);
        pubsub_tcpHandler_setThreadName(sender->socketHandler, topic, scope);
        pubsub_tcpHandler_setThreadPriority(sender->socketHandler, prio, sched);
        pubsub_tcpHandler_setSendRetryCnt(sender->socketHandler, (unsigned int) retryCnt);
        pubsub_tcpHandler_setSendTimeOut(sender->socketHandler, sendTimeout);
        pubsub_tcpHandler_setMaxMsgSize(sender->socketHandler, (unsigned int) maxMsgSize);
        // Hhen passiveKey is specified, enable receive event for full-duplex connection using key.
        // Because the topic receiver is already started, enable the receive event.
        pubsub_tcpHandler_enableReceiveEvent(sender->socketHandler, (passiveKey) ? true : false);
        pubsub_tcpHandler_setTimeout(sender->socketHandler, (unsigned int) timeout);
    }

    if (!sender->isPassive) {
        //setting up tcp socket for TCP TopicSender
        if (discUrl != NULL) {
            urls = strndup(discUrl, 1024 * 1024);
            sender->isStatic = true;
        } else if (ip != NULL) {
            urls = strndup(ip, 1024 * 1024);
        } else {
            struct sockaddr_in *sin = pubsub_utils_url_getInAddr(NULL, 0);
            urls = pubsub_utils_url_get_url(sin, NULL);
            free(sin);
        }
        if (!sender->url) {
            char *urlsCopy = strndup(urls, 1024 * 1024);
            char *url;
            char *save = urlsCopy;
            while ((url = strtok_r(save, " ", &save))) {
                int retry = 0;
                while (url && retry < TCP_BIND_MAX_RETRY) {
                    pubsub_utils_url_t *urlInfo = pubsub_utils_url_parse(url);
                    int rc = pubsub_tcpHandler_listen(sender->socketHandler, urlInfo->url);
                    if (rc < 0) {
                        L_WARN("Error for tcp_bind using dynamic bind url '%s'. %s", urlInfo->url, strerror(errno));
                    } else {
                        url = NULL;
                    }
                    pubsub_utils_url_free(urlInfo);
                    retry++;
                }
            }
            free(urlsCopy);
            sender->url = pubsub_tcpHandler_get_interface_url(sender->socketHandler);
        }
        free(urls);
    }

    //register publisher services using a service factory
    if ((sender->url != NULL) ||  (sender->isPassive)) {
        sender->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);

        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_tcp_getPublisherService;
        sender->publisher.factory.ungetService = psa_tcp_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        if (sender->scope != NULL) {
            celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);
        }

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.factory = &sender->publisher.factory;
        opts.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_PUBLISHER_SERVICE_VERSION;
        opts.properties = props;

        sender->publisher.svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    } else {
        free(sender);
        sender = NULL;
    }

    return sender;
}

void pubsub_tcpTopicSender_destroy(pubsub_tcp_topic_sender_t *sender) {
    if (sender != NULL) {

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
                    if (msgEntry->serializedIoVecOutput)
                        free(msgEntry->serializedIoVecOutput);
                    msgEntry->serializedIoVecOutput = NULL;
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

        pubsubInterceptorsHandler_destroy(sender->interceptorsHandler);
        if ((sender->socketHandler) && (sender->sharedSocketHandler == NULL)) {
            pubsub_tcpHandler_destroy(sender->socketHandler);
            sender->socketHandler = NULL;
        }

        if (sender->scope != NULL) {
            free(sender->scope);
        }
        free(sender->topic);
        free(sender->url);
        free(sender);
    }
}

long pubsub_tcpTopicSender_serializerSvcId(pubsub_tcp_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

long pubsub_tcpTopicSender_protocolSvcId(pubsub_tcp_topic_sender_t *sender) {
    return sender->protocolSvcId;
}

const char *pubsub_tcpTopicSender_scope(pubsub_tcp_topic_sender_t *sender) {
    return sender->scope;
}

const char *pubsub_tcpTopicSender_topic(pubsub_tcp_topic_sender_t *sender) {
    return sender->topic;
}

const char *pubsub_tcpTopicSender_url(pubsub_tcp_topic_sender_t *sender) {
    if (sender->isPassive) {
        return pubsub_tcpHandler_get_connection_url(sender->socketHandler);
    } else {
        return sender->url;
    }
}
bool pubsub_tcpTopicSender_isStatic(pubsub_tcp_topic_sender_t *sender) {
    return sender->isStatic;
}

bool pubsub_tcpTopicSender_isPassive(pubsub_tcp_topic_sender_t *sender) {
    return sender->isPassive;
}

void pubsub_tcpTopicSender_connectTo(pubsub_tcp_topic_sender_t *sender __attribute__((unused)), const celix_properties_t *endpoint __attribute__((unused))) {
    //TODO subscriber count -> topic info
}

void pubsub_tcpTopicSender_disconnectFrom(pubsub_tcp_topic_sender_t *sender __attribute__((unused)), const celix_properties_t *endpoint __attribute__((unused))) {
    //TODO
}

static int psa_tcp_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId) {
    psa_tcp_bounded_service_entry_t *entry = (psa_tcp_bounded_service_entry_t *) handle;
    *msgTypeId = (unsigned int) (uintptr_t) hashMap_get(entry->msgTypeIds, msgType);
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

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle,
                                                         (celix_bundle_t *) requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                hash_map_entry_t *hashMapEntry = hashMapIterator_nextEntry(&iter);
                void *key = hashMapEntry_getKey(hashMapEntry);
                psa_tcp_send_msg_entry_t *sendEntry = calloc(1, sizeof(*sendEntry));
                sendEntry->msgSer = hashMapEntry_getValue(hashMapEntry);
                sendEntry->protSer = sender->protocol;
                sendEntry->type = (int32_t) sendEntry->msgSer->msgId;
                int major;
                int minor;
                version_getMajor(sendEntry->msgSer->msgVersion, &major);
                version_getMinor(sendEntry->msgSer->msgVersion, &minor);
                sendEntry->major = (uint8_t) major;
                sendEntry->minor = (uint8_t) minor;
                uuid_copy(sendEntry->originUUID, sender->fwUUID);
                celixThreadMutex_create(&sendEntry->metrics.mutex, NULL);
                hashMap_put(entry->msgEntries, key, sendEntry);
                hashMap_put(entry->msgTypeIds, strndup(sendEntry->msgSer->msgName, 1024),
                            (void *) (uintptr_t) sendEntry->msgSer->msgId);
            }
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_tcp_localMsgTypeIdForMsgType;
            entry->service.send = psa_tcp_topicPublicationSend;
            hashMap_put(sender->boundedServices.map, (void *) bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for TCP TopicSender %s/%s", sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
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
            if (msgEntry->serializedIoVecOutput)
                free(msgEntry->serializedIoVecOutput);
            msgEntry->serializedIoVecOutput = NULL;
            celixThreadMutex_destroy(&msgEntry->metrics.mutex);
            free(msgEntry);
        }
        hashMap_destroy(entry->msgEntries, false, false);

        hashMap_destroy(entry->msgTypeIds, true, false);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

}

pubsub_admin_sender_metrics_t *pubsub_tcpTopicSender_metrics(pubsub_tcp_topic_sender_t *sender) {
    pubsub_admin_sender_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->scope == NULL ? PUBSUB_DEFAULT_ENDPOINT_SCOPE : sender->scope);
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
            result->msgMetrics[i].typeId = mEntry->type;
            snprintf(result->msgMetrics[i].typeFqn, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", mEntry->msgSer->msgName);
            i += 1;
            celixThreadMutex_unlock(&mEntry->metrics.mutex);
        }
    }

    celixThreadMutex_unlock(&sender->boundedServices.mutex);
    result->nrOfmsgMetrics = (int) count;
    return result;
}

static int
psa_tcp_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    int status = CELIX_SUCCESS;
    psa_tcp_bounded_service_entry_t *bound = handle;
    pubsub_tcp_topic_sender_t *sender = bound->parent;
    bool monitor = sender->metricsEnabled;

    psa_tcp_send_msg_entry_t *entry = hashMap_get(bound->msgEntries, (void *) (uintptr_t) (msgTypeId));

    //metrics updates
    struct timespec sendTime = {0, 0};
    struct timespec serializationStart;
    struct timespec serializationEnd;

    int sendErrorUpdate = 0;
    int serializationErrorUpdate = 0;
    int sendCountUpdate = 0;

    if (entry != NULL) {
        delay_first_send_for_late_joiners(sender);
        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationStart);
        }

        size_t serializedIoVecOutputLen = 0; //entry->serializedIoVecOutputLen;
        struct iovec *serializedIoVecOutput = NULL;
        status = entry->msgSer->serialize(entry->msgSer->handle, inMsg, &serializedIoVecOutput,
                                          &serializedIoVecOutputLen);
        entry->serializedIoVecOutputLen = MAX(serializedIoVecOutputLen, entry->serializedIoVecOutputLen);

        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationEnd);
        }

        bool cont = false;
        if (status == CELIX_SUCCESS) /*ser ok*/ {
            cont = pubsubInterceptorHandler_invokePreSend(sender->interceptorsHandler, entry->msgSer->msgName, msgTypeId, inMsg, &metadata);
        }
        if (cont) {
            pubsub_protocol_message_t message;
            message.metadata.metadata = NULL;
            message.payload.payload = NULL;
            message.payload.length = 0;
            if (serializedIoVecOutput) {
                message.payload.payload = serializedIoVecOutput->iov_base;
                message.payload.length = serializedIoVecOutput->iov_len;
            }
            message.header.msgId = msgTypeId;
            message.header.seqNr = entry->seqNr;
            message.header.msgMajorVersion = entry->major;
            message.header.msgMinorVersion = entry->minor;
            message.header.payloadSize = 0;
            message.header.payloadPartSize = 0;
            message.header.payloadOffset = 0;
            message.header.metadataSize = 0;
            if (metadata != NULL)
                message.metadata.metadata = metadata;
            entry->seqNr++;
            bool sendOk = true;
            {
                int rc = pubsub_tcpHandler_write(sender->socketHandler, &message, serializedIoVecOutput, serializedIoVecOutputLen, 0);
                if (rc < 0) {
                    status = -1;
                    sendOk = false;
                }
                pubsubInterceptorHandler_invokePostSend(sender->interceptorsHandler, entry->msgSer->msgName, msgTypeId, inMsg, metadata);
                if (message.metadata.metadata)
                    celix_properties_destroy(message.metadata.metadata);
                if (serializedIoVecOutput) {
                    entry->msgSer->freeSerializeMsg(entry->msgSer->handle,
                                                    serializedIoVecOutput,
                                                    serializedIoVecOutputLen);
                    serializedIoVecOutput = NULL;
                }
            }

            if (sendOk) {
                sendCountUpdate = 1;
            } else {
                sendErrorUpdate = 1;
                L_WARN("[PSA_TCP_TS] Error sending msg. %s", strerror(errno));
            }
        } else {
            serializationErrorUpdate = 1;
            L_WARN("[PSA_TCP_TS] Error serialize message of type %s for scope/topic %s/%s", entry->msgSer->msgName,
                   sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        }
    } else {
        //unknownMessageCountUpdate = 1;
        status = CELIX_SERVICE_EXCEPTION;
        L_WARN("[PSA_TCP_TS] Error cannot serialize message with msg type id %i for scope/topic %s/%s", msgTypeId,
               sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
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
        if (sender->send_delay ) {
            L_INFO("PSA_TCP_TP: Delaying first send for late joiners...\n");
        }
        usleep(sender->send_delay * 1000);
        firstSend = false;
    }
}