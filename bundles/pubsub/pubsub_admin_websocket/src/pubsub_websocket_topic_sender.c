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
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <zconf.h>
#include <celix_log_helper.h>
#include "pubsub_websocket_topic_sender.h"
#include "pubsub_psa_websocket_constants.h"
#include "pubsub_websocket_common.h"
#include <uuid/uuid.h>
#include <jansson.h>
#include "celix_constants.h"
#include "http_admin/api.h"
#include "civetweb.h"

#define FIRST_SEND_DELAY_IN_SECONDS             2

#define L_DEBUG(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_websocket_topic_sender {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;

    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *uri;

    celix_websocket_service_t websockSvc;
    long websockSvcId;
    celix_thread_mutex_t senderMutex;
    struct mg_connection *sockConnection;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        hash_map_t *map;  //key = bndId, value = psa_websocket_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_websocket_send_msg_entry {
    pubsub_websocket_msg_header_t header; //partially filled header (only seqnr and time needs to be updated per send)
    pubsub_msg_serializer_t *msgSer;
    celix_thread_mutex_t sendLock; //protects send & header(.seqNr)
} psa_websocket_send_msg_entry_t;

typedef struct psa_websocket_bounded_service_entry {
    pubsub_websocket_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes; //key = msg type id, value = pubsub_msg_serializer_t
    hash_map_t *msgTypeIds; //key = msg name, value = msg type id
    hash_map_t *msgEntries; //key = msg type id, value = psa_websocket_send_msg_entry_t
    int getCount;
} psa_websocket_bounded_service_entry_t;

static int psa_websocket_localMsgTypeIdForMsgType(void* handle __attribute__((unused)), const char* msgType, unsigned int* msgTypeId);
static void* psa_websocket_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_websocket_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void delay_first_send_for_late_joiners(pubsub_websocket_topic_sender_t *sender);

static int psa_websocket_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg, celix_properties_t *metadata);

static void psa_websocketTopicSender_ready(struct mg_connection *connection, void *handle);
static void psa_websocketTopicSender_close(const struct mg_connection *connection, void *handle);

pubsub_websocket_topic_sender_t* pubsub_websocketTopicSender_create(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
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
        sender->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->senderMutex, NULL);

        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (sender->websockSvcId > 0) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_websocket_getPublisherService;
        sender->publisher.factory.ungetService = psa_websocket_ungetPublisherService;

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

        celixThreadMutex_lock(&sender->senderMutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);

                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
                while (hashMapIterator_hasNext(&iter2)) {
                    psa_websocket_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter2);
                    free(msgEntry);

                }
                hashMap_destroy(entry->msgEntries, false, false);

                free(entry);
            }
        }
        hashMap_destroy(sender->boundedServices.map, false, false);
        celixThreadMutex_unlock(&sender->senderMutex);

        celixThreadMutex_destroy(&sender->senderMutex);

        celix_bundleContext_unregisterService(sender->ctx, sender->websockSvcId);

        if (sender->scope != NULL) {
            free(sender->scope);
        }
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

static int psa_websocket_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId) {
    psa_websocket_bounded_service_entry_t *entry = (psa_websocket_bounded_service_entry_t *) handle;
    *msgTypeId = (unsigned int)(uintptr_t) hashMap_get(entry->msgTypeIds, msgType);
    return 0;
}

static void* psa_websocket_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_websocket_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->senderMutex);
    psa_websocket_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;
        entry->msgEntries = hashMap_create(NULL, NULL, NULL, NULL);
        entry->msgTypeIds = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                hash_map_entry_t *hashMapEntry = hashMapIterator_nextEntry(&iter);
                void *key = hashMapEntry_getKey(hashMapEntry);
                psa_websocket_send_msg_entry_t *sendEntry = calloc(1, sizeof(*sendEntry));
                sendEntry->msgSer = hashMapEntry_getValue(hashMapEntry);
                sendEntry->header.id = sendEntry->msgSer->msgName;
                int major;
                int minor;
                version_getMajor(sendEntry->msgSer->msgVersion, &major);
                version_getMinor(sendEntry->msgSer->msgVersion, &minor);
                sendEntry->header.major = (uint8_t)major;
                sendEntry->header.minor = (uint8_t)minor;
                hashMap_put(entry->msgEntries, key, sendEntry);
                hashMap_put(entry->msgTypeIds, strndup(sendEntry->msgSer->msgName, 1024), (void *)(uintptr_t) sendEntry->msgSer->msgId);
            }
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_websocket_localMsgTypeIdForMsgType;
            entry->service.send = psa_websocket_topicPublicationSend;
            hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for websocket TopicSender %s/%s", sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        }
    }
    celixThreadMutex_unlock(&sender->senderMutex);

    return &entry->service;
}

static void psa_websocket_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_websocket_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->senderMutex);
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
            free(msgEntry);
        }
        hashMap_destroy(entry->msgEntries, false, false);

        hashMap_destroy(entry->msgTypeIds, true, false);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->senderMutex);
}

static int psa_websocket_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    int status = CELIX_SERVICE_EXCEPTION;
    psa_websocket_bounded_service_entry_t *bound = handle;
    pubsub_websocket_topic_sender_t *sender = bound->parent;
    psa_websocket_send_msg_entry_t *entry = hashMap_get(bound->msgEntries, (void *) (uintptr_t) (msgTypeId));

    celixThreadMutex_lock(&sender->senderMutex);
    if (sender->sockConnection != NULL && entry != NULL) {
        delay_first_send_for_late_joiners(sender);
        size_t serializedOutputLen = 0;
        struct iovec* serializedOutput = NULL;
        status = entry->msgSer->serialize(entry->msgSer->handle, inMsg, &serializedOutput, &serializedOutputLen);

        if (status == CELIX_SUCCESS /*ser ok*/) {
            json_error_t jsError;
            unsigned char *hdrEncoded = calloc(sizeof(pubsub_websocket_msg_header_t), sizeof(unsigned char));

            celixThreadMutex_lock(&entry->sendLock);

            json_t *jsMsg = json_object();
            json_object_set_new_nocheck(jsMsg, "id", json_string(entry->header.id));
            json_object_set_new_nocheck(jsMsg, "major", json_integer(entry->header.major));
            json_object_set_new_nocheck(jsMsg, "minor", json_integer(entry->header.minor));
            json_object_set_new_nocheck(jsMsg, "seqNr", json_integer(entry->header.seqNr++));

            json_t *jsData;
            jsData = json_loadb((const char *)serializedOutput->iov_base, serializedOutput->iov_len - 1, 0, &jsError);
            if(jsData != NULL) {
                json_object_set_new_nocheck(jsMsg, "data", jsData);
                const char *msg = json_dumps(jsMsg, 0);
                size_t bytes_to_write = strlen(msg);
                int bytes_written = mg_websocket_client_write(sender->sockConnection, MG_WEBSOCKET_OPCODE_TEXT, msg,
                                                              bytes_to_write);
                free((void *) msg);
                json_decref(jsData); //Decrease ref count means freeing the object
                if (bytes_written != (int) bytes_to_write) {
                    L_WARN("[PSA_WEBSOCKET_TS] Error sending websocket, written %d of total %lu bytes", bytes_written, bytes_to_write);
                }
            } else {
                L_WARN("[PSA_WEBSOCKET_TS] Error sending websocket, serialized data corrupt. Error(%d;%d;%d): %s", jsError.column, jsError.line, jsError.position, jsError.text);
            }
            celixThreadMutex_unlock(&entry->sendLock);

            json_decref(jsMsg); //Decrease ref count means freeing the object
            free(hdrEncoded);
            entry->msgSer->freeSerializeMsg(entry->msgSer->handle, serializedOutput, serializedOutputLen);
        } else {
            L_WARN("[PSA_WEBSOCKET_TS] Error serialize message of type %s for scope/topic %s/%s",
                   entry->msgSer->msgName, sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        }
    } else if (entry == NULL){
        L_WARN("[PSA_WEBSOCKET_TS] Error sending message with msg type id %i for scope/topic %s/%s", msgTypeId, sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
    }
    celixThreadMutex_unlock(&sender->senderMutex);

    return status;
}

static void psa_websocketTopicSender_ready(struct mg_connection *connection, void *handle) {
    //Connection succeeded so save connection to use for sending the messages
    pubsub_websocket_topic_sender_t *sender = (pubsub_websocket_topic_sender_t *) handle;
    celixThreadMutex_lock(&sender->senderMutex);
    sender->sockConnection = connection;
    celixThreadMutex_unlock(&sender->senderMutex);
}

static void psa_websocketTopicSender_close(const struct mg_connection *connection __attribute__((unused)), void *handle) {
    //Connection closed so reset connection
    pubsub_websocket_topic_sender_t *sender = (pubsub_websocket_topic_sender_t *) handle;
    celixThreadMutex_lock(&sender->senderMutex);
    sender->sockConnection = NULL;
    celixThreadMutex_unlock(&sender->senderMutex);
}

static void delay_first_send_for_late_joiners(pubsub_websocket_topic_sender_t *sender) {

    static bool firstSend = true;

    if (firstSend) {
        L_INFO("PSA_WEBSOCKET_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}
