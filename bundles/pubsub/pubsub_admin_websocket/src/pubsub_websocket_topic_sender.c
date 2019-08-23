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
#include <log_helper.h>
#include "pubsub_websocket_topic_sender.h"
#include "pubsub_psa_websocket_constants.h"
#include "pubsub_websocket_common.h"
#include <uuid/uuid.h>
#include <constants.h>
#include "http_admin/api.h"
#include "civetweb.h"

#define FIRST_SEND_DELAY_IN_SECONDS             2

#define L_DEBUG(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_websocket_topic_sender {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    uuid_t fwUUID;
    bool metricsEnabled;

    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *uri;

    celix_websocket_service_t websockSvc;
    long websockSvcId;
    struct mg_connection *sockConnection;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_websocket_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_websocket_send_msg_entry {
    pubsub_websocket_msg_header_t header; //partially filled header (only seqnr and time needs to be updated per send)
    pubsub_msg_serializer_t *msgSer;
    celix_thread_mutex_t sendLock; //protects send & Seqnr
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
} psa_websocket_send_msg_entry_t;

typedef struct psa_websocket_bounded_service_entry {
    pubsub_websocket_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes; //key = msg type id, value = pubsub_msg_serializer_t
    hash_map_t *msgEntries; //key = msg type id, value = psa_websocket_send_msg_entry_t
    int getCount;
} psa_websocket_bounded_service_entry_t;


static void* psa_websocket_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_websocket_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void delay_first_send_for_late_joiners(pubsub_websocket_topic_sender_t *sender);

static int psa_websocket_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg);

static void psa_websocketTopicSender_ready(struct mg_connection *connection, void *handle);
static void psa_websocketTopicSender_close(const struct mg_connection *connection, void *handle);

pubsub_websocket_topic_sender_t* pubsub_websocketTopicSender_create(
        celix_bundle_context_t *ctx,
        log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *ser) {
    pubsub_websocket_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;
    psa_websocket_setScopeAndTopicFilter(scope, topic, sender->scopeAndTopicFilter);
    const char* uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    sender->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_WEBSOCKET_METRICS_ENABLED, PSA_WEBSOCKET_DEFAULT_METRICS_ENABLED);

    sender->uri = psa_websocket_createURI(scope, topic);

    if (sender->uri != NULL) {
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, WEBSOCKET_ADMIN_URI, sender->uri);

        sender->websockSvc.handle = sender;
        sender->websockSvc.ready = psa_websocketTopicSender_ready;
        sender->websockSvc.close = psa_websocketTopicSender_close;
        sender->websockSvcId = celix_bundleContext_registerService(ctx, &sender->websockSvc,
                                                                   WEBSOCKET_ADMIN_SERVICE_NAME, props);
    } else {
        sender->websockSvcId = -1;
    }

    if (sender->websockSvcId > 0) {
        sender->scope = strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (sender->websockSvcId > 0) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_websocket_getPublisherService;
        sender->publisher.factory.ungetService = psa_websocket_ungetPublisherService;

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

    if (sender->websockSvcId < 0) {
        free(sender);
        sender = NULL;
    }

    return sender;
}

void pubsub_websocketTopicSender_destroy(pubsub_websocket_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        celixThreadMutex_lock(&sender->boundedServices.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);

                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
                while (hashMapIterator_hasNext(&iter2)) {
                    psa_websocket_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter2);
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

        celix_bundleContext_unregisterService(sender->ctx, sender->websockSvcId);

        free(sender->scope);
        free(sender->topic);
        free(sender->uri);
        free(sender);
    }
}

long pubsub_websocketTopicSender_serializerSvcId(pubsub_websocket_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

const char* pubsub_websocketTopicSender_scope(pubsub_websocket_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_websocketTopicSender_topic(pubsub_websocket_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_websocketTopicSender_url(pubsub_websocket_topic_sender_t *sender) {
    return sender->uri;
}


static void* psa_websocket_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_websocket_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_websocket_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;
        entry->msgEntries = hashMap_create(NULL, NULL, NULL, NULL);

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                hash_map_entry_t *hashMapEntry = hashMapIterator_nextEntry(&iter);
                void *key = hashMapEntry_getKey(hashMapEntry);
                psa_websocket_send_msg_entry_t *sendEntry = calloc(1, sizeof(*sendEntry));
                sendEntry->msgSer = hashMapEntry_getValue(hashMapEntry);
                sendEntry->header.type = (int32_t)sendEntry->msgSer->msgId;
                int major;
                int minor;
                version_getMajor(sendEntry->msgSer->msgVersion, &major);
                version_getMinor(sendEntry->msgSer->msgVersion, &minor);
                sendEntry->header.major = (uint8_t)major;
                sendEntry->header.minor = (uint8_t)minor;
                uuid_copy(sendEntry->header.originUUID, sender->fwUUID);
                celixThreadMutex_create(&sendEntry->metrics.mutex, NULL);
                hashMap_put(entry->msgEntries, key, sendEntry);
            }
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_websocket_localMsgTypeIdForMsgType;
            entry->service.send = psa_websocket_topicPublicationSend;
            hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for websocket TopicSender %s/%s", sender->scope, sender->topic);
        }
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return &entry->service;
}

static void psa_websocket_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_websocket_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_websocket_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(sender->boundedServices.map, (void*)bndId);
        int rc = sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }

        hash_map_iterator_t iter = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter);
            celixThreadMutex_destroy(&msgEntry->metrics.mutex);
            free(msgEntry);
        }
        hashMap_destroy(entry->msgEntries, false, false);

        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

pubsub_admin_sender_metrics_t* pubsub_websocketTopicSender_metrics(pubsub_websocket_topic_sender_t *sender) {
    pubsub_admin_sender_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->topic);
    celixThreadMutex_lock(&sender->boundedServices.mutex);
    size_t count = 0;
    hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_websocket_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
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
        psa_websocket_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter2)) {
            psa_websocket_send_msg_entry_t *mEntry = hashMapIterator_nextValue(&iter2);
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
    result->nrOfmsgMetrics = (int)count;
    return result;
}

static int psa_websocket_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg) {
    int status = CELIX_SERVICE_EXCEPTION;
    psa_websocket_bounded_service_entry_t *bound = handle;
    pubsub_websocket_topic_sender_t *sender = bound->parent;
    bool monitor = sender->metricsEnabled;
    psa_websocket_send_msg_entry_t *entry = hashMap_get(bound->msgEntries, (void *) (uintptr_t) (msgTypeId));

    //metrics updates
    struct timespec sendTime;
    struct timespec serializationStart;
    struct timespec serializationEnd;
    //int unknownMessageCountUpdate = 0;
    int sendErrorUpdate = 0;
    int serializationErrorUpdate = 0;
    int sendCountUpdate = 0;

    if (sender->sockConnection != NULL && entry != NULL) {
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
            unsigned char *hdrEncoded = calloc(sizeof(pubsub_websocket_msg_header_t), sizeof(unsigned char));

            celixThreadMutex_lock(&entry->sendLock);

            pubsub_websocket_msg_t *msg = malloc(sizeof(*msg) + sizeof(char[serializedOutputLen]));
            pubsub_websocket_msg_header_t *msgHdr = &entry->header;
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &sendTime);
                msgHdr->sendtimeSeconds = (uint64_t) sendTime.tv_sec;
                msgHdr->sendTimeNanoseconds = (uint64_t) sendTime.tv_nsec;
                msgHdr->seqNr++;
            }
            memcpy(&msg->header, msgHdr, sizeof(pubsub_websocket_msg_header_t));

            msg->payloadSize = (unsigned int) serializedOutputLen;
            size_t hdr_size = sizeof(msg->header);
            size_t ps_size = sizeof(msg->payloadSize);
            size_t bytes_to_write = hdr_size + ps_size + serializedOutputLen;//sizeof(*msg);
            memcpy(msg->payload, serializedOutput, serializedOutputLen);
            int bytes_written = mg_websocket_client_write(sender->sockConnection, MG_WEBSOCKET_OPCODE_TEXT, (char *) msg, bytes_to_write);

            celixThreadMutex_unlock(&entry->sendLock);
            if (bytes_written == (int) bytes_to_write) {
                sendCountUpdate = 1;
            } else {
                sendErrorUpdate = 1;
                L_WARN("[PSA_WEBSOCKET_TS] Error sending websocket.");
            }

            free(msg);
            free(hdrEncoded);
            free(serializedOutput);
        } else {
            serializationErrorUpdate = 1;
            L_WARN("[PSA_WEBSOCKET_TS] Error serialize message of type %s for scope/topic %s/%s",
                   entry->msgSer->msgName, sender->scope, sender->topic);
        }
    } else if (entry == NULL){
        //unknownMessageCountUpdate = 1;
        L_WARN("[PSA_WEBSOCKET_TS] Error sending message with msg type id %i for scope/topic %s/%s", msgTypeId, sender->scope, sender->topic);
    }


    if (monitor && status == CELIX_SUCCESS) {
        celixThreadMutex_lock(&entry->metrics.mutex);

        long n = entry->metrics.nrOfMessagesSend + entry->metrics.nrOfMessagesSendFailed;
        double diff = celix_difftime(&serializationStart, &serializationEnd);
        double average = (entry->metrics.averageSerializationTimeInSeconds * n + diff) / (n+1);
        entry->metrics.averageSerializationTimeInSeconds = average;

        if (entry->metrics.nrOfMessagesSend > 2) {
            diff = celix_difftime(&entry->metrics.lastMessageSend, &sendTime);
            n = entry->metrics.nrOfMessagesSend;
            average = (entry->metrics.averageTimeBetweenMessagesInSeconds * n + diff) / (n+1);
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

static void psa_websocketTopicSender_ready(struct mg_connection *connection, void *handle) {
    //Connection succeeded so save connection to use for sending the messages
    pubsub_websocket_topic_sender_t *sender = (pubsub_websocket_topic_sender_t *) handle;
    sender->sockConnection = connection;
}

static void psa_websocketTopicSender_close(const struct mg_connection *connection __attribute__((unused)), void *handle) {
    //Connection closed so reset connection
    pubsub_websocket_topic_sender_t *sender = (pubsub_websocket_topic_sender_t *) handle;
    sender->sockConnection = NULL;
}

static void delay_first_send_for_late_joiners(pubsub_websocket_topic_sender_t *sender) {

    static bool firstSend = true;

    if (firstSend) {
        L_INFO("PSA_WEBSOCKET_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}
