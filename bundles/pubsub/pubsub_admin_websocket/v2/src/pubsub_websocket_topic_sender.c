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
#include <jansson.h>
#include "celix_constants.h"
#include "http_admin/api.h"
#include "civetweb.h"
#include "pubsub_websocket_admin.h"
#include "pubsub_interceptors_handler.h"

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

    void *admin;
    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *uri;

    pubsub_serializer_handler_t* serializerHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;

    int seqNr; //atomic

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

typedef struct psa_websocket_bounded_service_entry {
    pubsub_websocket_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
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
        pubsub_serializer_handler_t* serializerHandler,
        void *admin) {
    pubsub_websocket_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerHandler = serializerHandler;
    sender->interceptorsHandler = pubsubInterceptorsHandler_create(ctx, scope, topic, PUBSUB_WEBSOCKET_ADMIN_TYPE, pubsub_serializerHandler_getSerializationType(serializerHandler));

    psa_websocket_setScopeAndTopicFilter(scope, topic, sender->scopeAndTopicFilter);
    sender->uri = psa_websocket_createURI(scope, topic);
    sender->admin = admin;

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

        celixThreadMutex_lock(&sender->boundedServices.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_websocket_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
            free(entry);
        }
        hashMap_destroy(sender->boundedServices.map, false, false);
        celixThreadMutex_unlock(&sender->boundedServices.mutex);

        celixThreadMutex_destroy(&sender->boundedServices.mutex);

        celix_bundleContext_unregisterService(sender->ctx, sender->websockSvcId);

        if (sender->scope != NULL) {
            free(sender->scope);
        }
        pubsubInterceptorsHandler_destroy(sender->interceptorsHandler);
        free(sender->topic);
        free(sender->uri);
        free(sender);
    }
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

const char* pubsub_websocketTopicSender_serializerType(pubsub_websocket_topic_sender_t *sender) {
    return pubsub_serializerHandler_getSerializationType(sender->serializerHandler);
}

static int psa_websocket_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId) {
    psa_websocket_bounded_service_entry_t *entry = (psa_websocket_bounded_service_entry_t *) handle;
    uint32_t msgId = pubsub_serializerHandler_getMsgId(entry->parent->serializerHandler, msgType);
    if (msgId != 0) {
        *msgTypeId = msgId;
        return 0;
    }
    return -1;
}

static void* psa_websocket_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_websocket_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_websocket_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void *) bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;
        entry->service.handle = entry;
        entry->service.localMsgTypeIdForMsgType = psa_websocket_localMsgTypeIdForMsgType;
        entry->service.send = psa_websocket_topicPublicationSend;
        hashMap_put(sender->boundedServices.map, (void *) bndId, entry);
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
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static int psa_websocket_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    psa_websocket_bounded_service_entry_t *bound = handle;
    pubsub_websocket_topic_sender_t *sender = bound->parent;

    const char* msgFqn;
    int majorVersion;
    int minorVersion;
    celix_status_t status = pubsub_serializerHandler_getMsgInfo(sender->serializerHandler, msgTypeId, &msgFqn, &majorVersion, &minorVersion);
    if (status != CELIX_SUCCESS) {
        L_WARN("Cannot find serializer for msg id %u for serializer %s", msgTypeId, pubsub_serializerHandler_getSerializationType(sender->serializerHandler));
        celix_properties_destroy(metadata);
        return status;
    }

    bool cont = pubsubInterceptorHandler_invokePreSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, &metadata);
    if (!cont) {
        L_DEBUG("Cancel send based on pubsub interceptor cancel return");
        celix_properties_destroy(metadata);
        return status;
    }

    if (sender->sockConnection != NULL) {
        delay_first_send_for_late_joiners(sender);
        size_t serializedOutputLen = 0;
        struct iovec* serializedOutput = NULL;
        status = pubsub_serializerHandler_serialize(sender->serializerHandler, msgTypeId, inMsg, &serializedOutput, &serializedOutputLen);
        if (status == CELIX_SUCCESS /*ser ok*/) {
            json_error_t jsError;

            json_t *jsMsg = json_object();
            json_object_set_new_nocheck(jsMsg, "id", json_string(msgFqn));
            json_object_set_new_nocheck(jsMsg, "major", json_integer(majorVersion));
            json_object_set_new_nocheck(jsMsg, "minor", json_integer(minorVersion));
            uint32_t seqNr = __atomic_fetch_add(&sender->seqNr, 1, __ATOMIC_RELAXED);
            json_object_set_new_nocheck(jsMsg, "seqNr", json_integer(seqNr));

            json_t *jsData;
            jsData = json_loadb((const char *)serializedOutput->iov_base, serializedOutput->iov_len, 0, &jsError);
            if (jsData != NULL) {
                json_object_set_new_nocheck(jsMsg, "data", jsData);
                const char *msg = json_dumps(jsMsg, 0);
                size_t bytes_to_write = strlen(msg);
                int bytes_written = mg_websocket_write(sender->sockConnection, MG_WEBSOCKET_OPCODE_TEXT, msg,
                                                              bytes_to_write);
                free((void *) msg);
                json_decref(jsData); //Decrease ref count means freeing the object
                if (bytes_written != (int) bytes_to_write) {
                    L_WARN("[PSA_WEBSOCKET_TS] Error sending websocket, written %d of total %lu bytes", bytes_written, bytes_to_write);
                }
            } else {
                L_WARN("[PSA_WEBSOCKET_TS] Error sending websocket, serialized data corrupt. Error(%d;%d;%d): %s", jsError.column, jsError.line, jsError.position, jsError.text);
            }

            json_decref(jsMsg); //Decrease ref count means freeing the object
            pubsub_serializerHandler_freeSerializedMsg(sender->serializerHandler, msgTypeId, serializedOutput, serializedOutputLen);
        } else {
            L_WARN("[PSA_WEBSOCKET_TS] Error serialize message of type %u for scope/topic %s/%s",
                   msgTypeId, sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        }
    } else { // when (sender->sockConnection == NULL) we dont have a client, but we do have a valid entry
    	status = CELIX_SUCCESS; // Not an error, just nothing to do
    }

    pubsubInterceptorHandler_invokePostSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, metadata);
    celix_properties_destroy(metadata);
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
