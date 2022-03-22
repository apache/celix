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
#include "pubsub_tcp_admin.h"

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
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    uuid_t fwUUID;
    pubsub_tcpHandler_t *socketHandler;
    pubsub_tcpHandler_t *sharedSocketHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;
    pubsub_serializer_handler_t* serializerHandler;

    void *admin;
    char *scope;
    char *topic;
    char *url;
    bool isStatic;
    bool isPassive;
    bool verbose;
    unsigned long send_delay;
    int seqNr; //atomic

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_tcp_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_tcp_bounded_service_entry {
    pubsub_tcp_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
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
    pubsub_serializer_handler_t* serializerHandler,
    void *admin,
    const celix_properties_t *topicProperties,
    pubsub_tcp_endPointStore_t *handlerStore,
    long protocolSvcId,
    pubsub_protocol_service_t *protocol) {
    pubsub_tcp_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerHandler = serializerHandler;
    sender->admin = admin;
    sender->protocolSvcId = protocolSvcId;
    sender->protocol = protocol;
    const char *uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    sender->interceptorsHandler = pubsubInterceptorsHandler_create(ctx, scope, topic, PUBSUB_TCP_ADMIN_TYPE,
                                                                   pubsub_serializerHandler_getSerializationType(serializerHandler));
    sender->isPassive = false;
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
        // When passiveKey is specified, enable receive event for full-duplex connection using key.
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
            free(entry);
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

long pubsub_tcpTopicSender_protocolSvcId(pubsub_tcp_topic_sender_t *sender) {
    return sender->protocolSvcId;
}

const char *pubsub_tcpTopicSender_scope(pubsub_tcp_topic_sender_t *sender) {
    return sender->scope;
}

const char *pubsub_tcpTopicSender_topic(pubsub_tcp_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_tcpTopicSender_serializerType(pubsub_tcp_topic_sender_t *sender) {
    return pubsub_serializerHandler_getSerializationType(sender->serializerHandler);
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
        entry->service.handle = entry;
        entry->service.localMsgTypeIdForMsgType = psa_tcp_localMsgTypeIdForMsgType;
        entry->service.send = psa_tcp_topicPublicationSend;
        hashMap_put(sender->boundedServices.map, (void *) bndId, entry);
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
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

}

static int
psa_tcp_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    psa_tcp_bounded_service_entry_t *bound = handle;
    pubsub_tcp_topic_sender_t *sender = bound->parent;
    const char* msgFqn;
    int majorVersion;
    int minorVersion;
    celix_status_t status = pubsub_serializerHandler_getMsgInfo(sender->serializerHandler, msgTypeId, &msgFqn, &majorVersion, &minorVersion);

    if (status != CELIX_SUCCESS) {
        L_WARN("Cannot find serializer for msg id %u for serializer %s", msgTypeId,
               pubsub_serializerHandler_getSerializationType(sender->serializerHandler));
        celix_properties_destroy(metadata);
        return status;
    }

    bool cont = pubsubInterceptorHandler_invokePreSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, &metadata);
    if (!cont) {
        L_DEBUG("Cancel send based on pubsub interceptor cancel return");
        celix_properties_destroy(metadata);
        return status;
    }

    size_t serializedIoVecOutputLen = 0; //entry->serializedIoVecOutputLen;
    struct iovec *serializedIoVecOutput = NULL;
    status = pubsub_serializerHandler_serialize(sender->serializerHandler, msgTypeId, inMsg, &serializedIoVecOutput, &serializedIoVecOutputLen);
    if (status != CELIX_SUCCESS) {
        L_WARN("[PSA_TCP_V2_TS] Error serialize message of type %s for scope/topic %s/%s", msgFqn,
               sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        celix_properties_destroy(metadata);
        return status;
    }

    delay_first_send_for_late_joiners(sender);

    pubsub_protocol_message_t message;
    message.metadata.metadata = NULL;
    message.payload.payload = NULL;
    message.payload.length = 0;
    if (serializedIoVecOutput) {
        message.payload.payload = serializedIoVecOutput->iov_base;
        message.payload.length = serializedIoVecOutput->iov_len;
    }
    message.header.msgId = msgTypeId;
    message.header.seqNr = __atomic_fetch_add(&sender->seqNr, 1, __ATOMIC_RELAXED);
    message.header.msgMajorVersion = (uint16_t)majorVersion;
    message.header.msgMinorVersion = (uint16_t)minorVersion;
    message.header.payloadSize = 0;
    message.header.payloadPartSize = 0;
    message.header.payloadOffset = 0;
    message.header.metadataSize = 0;
    if (metadata != NULL) {
        message.metadata.metadata = metadata;
    }
    bool sendOk = true;
    {
        int rc = pubsub_tcpHandler_write(sender->socketHandler, &message, serializedIoVecOutput, serializedIoVecOutputLen, 0);
        if (rc < 0) {
            status = -1;
            sendOk = false;
        }
        pubsubInterceptorHandler_invokePostSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, metadata);
        if (serializedIoVecOutput) {
            pubsub_serializerHandler_freeSerializedMsg(sender->serializerHandler, msgTypeId, serializedIoVecOutput, serializedIoVecOutputLen);
            serializedIoVecOutput = NULL;
        }
    }

    if (!sendOk) {
        L_WARN("[PSA_TCP_V2_TS] Error sending msg. %s", strerror(errno));
    }

    celix_properties_destroy(metadata);
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

static int psa_tcp_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId) {
    psa_tcp_bounded_service_entry_t* entry = handle;
    uint32_t msgId = pubsub_serializerHandler_getMsgId(entry->parent->serializerHandler, msgType);
    *msgTypeId = (unsigned int)msgId;
    return 0;
}