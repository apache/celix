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

#include <memory.h>
#include <ip_utils.h>

#include "pubsub_endpoint.h"
#include "pubsub_serializer.h"
#include "pubsub_matching.h"
#include "pubsub_utils.h"
#include "pubsub_websocket_admin.h"
#include "pubsub_psa_websocket_constants.h"
#include "pubsub_websocket_topic_sender.h"
#include "pubsub_websocket_topic_receiver.h"
#include "pubsub_websocket_common.h"
#include "pubsub_serializer_handler.h"

#define L_DEBUG(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_websocket_admin {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *log;
    const char *fwUUID;

    double qosSampleScore;
    double qosControlScore;
    double defaultScore;

    bool verbose;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_websocket_topic_sender_t*
    } topicSenders;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_websocket_topic_sender_t*
    } topicReceivers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = endpoint uuid, value = celix_properties_t* (endpoint)
    } discoveredEndpoints;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = pubsub message serialization marker svc id (long), pubsub_serialization_handler_t*.
    } serializationHandlers;
};

static celix_status_t pubsub_websocketAdmin_connectEndpointToReceiver(pubsub_websocket_admin_t* psa, pubsub_websocket_topic_receiver_t *receiver, const celix_properties_t *endpoint);
static celix_status_t pubsub_websocketAdmin_disconnectEndpointFromReceiver(pubsub_websocket_admin_t* psa, pubsub_websocket_topic_receiver_t *receiver, const celix_properties_t *endpoint);

pubsub_websocket_admin_t* pubsub_websocketAdmin_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper) {
    pubsub_websocket_admin_t *psa = calloc(1, sizeof(*psa));
    psa->ctx = ctx;
    psa->log = logHelper;
    psa->verbose = celix_bundleContext_getPropertyAsBool(ctx, PUBSUB_WEBSOCKET_VERBOSE_KEY, PUBSUB_WEBSOCKET_VERBOSE_DEFAULT);
    psa->fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);

    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_WEBSOCKET_DEFAULT_SCORE_KEY, PSA_WEBSOCKET_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_WEBSOCKET_QOS_SAMPLE_SCORE_KEY, PSA_WEBSOCKET_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_WEBSOCKET_QOS_CONTROL_SCORE_KEY, PSA_WEBSOCKET_DEFAULT_QOS_CONTROL_SCORE);

    celixThreadMutex_create(&psa->topicSenders.mutex, NULL);
    psa->topicSenders.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->topicReceivers.mutex, NULL);
    psa->topicReceivers.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->discoveredEndpoints.mutex, NULL);
    psa->discoveredEndpoints.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->serializationHandlers.mutex, NULL);
    psa->serializationHandlers.map = hashMap_create(NULL, NULL, NULL, NULL);

    return psa;
}

void pubsub_websocketAdmin_destroy(pubsub_websocket_admin_t *psa) {
    if (psa == NULL) {
        return;
    }

    //note assuming all psa register services and service tracker are removed.
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_websocket_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        pubsub_websocketTopicSender_destroy(sender);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_websocket_topic_receiver_t *recv = hashMapIterator_nextValue(&iter);
        pubsub_websocketTopicReceiver_destroy(recv);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *ep = hashMapIterator_nextValue(&iter);
        celix_properties_destroy(ep);
    }
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    celixThreadMutex_lock(&psa->serializationHandlers.mutex);
    iter = hashMapIterator_construct(psa->serializationHandlers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_serializer_handler_t* entry = hashMapIterator_nextValue(&iter);
        pubsub_serializerHandler_destroy(entry);
    }
    celixThreadMutex_unlock(&psa->serializationHandlers.mutex);

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->discoveredEndpoints.mutex);
    hashMap_destroy(psa->discoveredEndpoints.map, false, false);

    celixThreadMutex_destroy(&psa->serializationHandlers.mutex);
    hashMap_destroy(psa->serializationHandlers.map, false, false);

    free(psa);
}

celix_status_t pubsub_websocketAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, celix_properties_t **topicProperties, double *outScore, long *outSerializerSvcId, long *outProtocolSvcId) {
    pubsub_websocket_admin_t *psa = handle;
    L_DEBUG("[PSA_WEBSOCKET_V2] pubsub_websocketAdmin_matchPublisher");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_WEBSOCKET_ADMIN_TYPE,
                                               psa->qosSampleScore, psa->qosControlScore, psa->defaultScore,
                                               false, topicProperties, outSerializerSvcId, outProtocolSvcId);
    *outScore = score;

    return status;
}

celix_status_t pubsub_websocketAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, celix_properties_t **topicProperties, double *outScore, long *outSerializerSvcId, long *outProtocolSvcId) {
    pubsub_websocket_admin_t *psa = handle;
    L_DEBUG("[PSA_WEBSOCKET_V2] pubsub_websocketAdmin_matchSubscriber");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_WEBSOCKET_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore,
                                                false, topicProperties, outSerializerSvcId, outProtocolSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t pubsub_websocketAdmin_matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *outMatch) {
    pubsub_websocket_admin_t *psa = handle;
    L_DEBUG("[PSA_WEBSOCKET_V2] pubsub_websocketAdmin_matchEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    bool match = pubsub_utils_matchEndpoint(psa->ctx, psa->log, endpoint, PUBSUB_WEBSOCKET_ADMIN_TYPE, false, NULL, NULL);
    if (outMatch != NULL) {
        *outMatch = match;
    }
    return status;
}

static pubsub_serializer_handler_t* pubsub_websocketAdmin_getSerializationHandler(pubsub_websocket_admin_t* psa, long msgSerializationMarkerSvcId) {
    pubsub_serializer_handler_t* handler = NULL;
    celixThreadMutex_lock(&psa->serializationHandlers.mutex);
    handler = hashMap_get(psa->serializationHandlers.map, (void*)msgSerializationMarkerSvcId);
    if (handler == NULL) {
        handler = pubsub_serializerHandler_createForMarkerService(psa->ctx, msgSerializationMarkerSvcId, psa->log);
        if (handler != NULL) {
            hashMap_put(psa->serializationHandlers.map, (void*)msgSerializationMarkerSvcId, handler);
        }
    }
    celixThreadMutex_unlock(&psa->serializationHandlers.mutex);
    return handler;
}


celix_status_t pubsub_websocketAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **outPublisherEndpoint) {
    pubsub_websocket_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Get serialization handler
    //2) Create TopicSender
    //3) Store TopicSender
    //4) Connect existing endpoints
    //5) set outPublisherEndpoint

    pubsub_serializer_handler_t* handler = pubsub_websocketAdmin_getSerializationHandler(psa, serializerSvcId);
    if (handler == NULL) {
        L_ERROR("Cannot create topic sender without serialization handler");
        return CELIX_ILLEGAL_STATE;
    }

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);

    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_websocket_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        sender = pubsub_websocketTopicSender_create(psa->ctx, psa->log, scope, topic, handler, psa);
        if (sender != NULL) {
            const char *psaType = PUBSUB_WEBSOCKET_ADMIN_TYPE;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType,
                                                pubsub_serializerHandler_getSerializationType(handler), NULL, NULL);

            //Set endpoint visibility to local because the http server handles discovery
            celix_properties_set(newEndpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_LOCAL_VISIBILITY);

            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
            hashMap_put(psa->topicSenders.map, key, sender);
        } else {
            L_ERROR("[PSA_WEBSOCKET_V2] Error creating a TopicSender");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_WEBSOCKET_V2] Cannot setup already existing TopicSender for scope/topic %s/%s!", scope == NULL ? "(null)" : scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    if (newEndpoint != NULL && outPublisherEndpoint != NULL) {
        *outPublisherEndpoint = newEndpoint;
    }

    return status;
}

celix_status_t pubsub_websocketAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic) {
    pubsub_websocket_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Find and remove TopicSender from map
    //2) destroy topic sender

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicSenders.map, key);
    if (entry != NULL) {
        char *mapKey = hashMapEntry_getKey(entry);
        pubsub_websocket_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        pubsub_websocketTopicSender_destroy(sender);
    } else {
        L_ERROR("[PSA_WEBSOCKET_V2] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists", scope == NULL ? "(null)" : scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_websocketAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **outSubscriberEndpoint) {
    pubsub_websocket_admin_t *psa = handle;
    celix_properties_t *newEndpoint = NULL;

    pubsub_serializer_handler_t* handler = pubsub_websocketAdmin_getSerializationHandler(psa, serializerSvcId);
    if (handler == NULL) {
        L_ERROR("Cannot create topic receiver without serialization handler");
        return CELIX_ILLEGAL_STATE;
    }


    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    pubsub_websocket_topic_receiver_t *receiver = hashMap_get(psa->topicReceivers.map, key);
    if (receiver == NULL) {
        receiver = pubsub_websocketTopicReceiver_create(psa->ctx, psa->log, scope, topic, topicProperties, handler, psa);
        if (receiver != NULL) {
            const char *psaType = PUBSUB_WEBSOCKET_ADMIN_TYPE;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic,
                                                PUBSUB_SUBSCRIBER_ENDPOINT_TYPE, psaType,
                                                pubsub_serializerHandler_getSerializationType(handler), NULL, NULL);

            //Set endpoint visibility to local because the http server handles discovery
            celix_properties_set(newEndpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_LOCAL_VISIBILITY);

            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
            hashMap_put(psa->topicReceivers.map, key, receiver);
        } else {
            L_ERROR("[PSA_WEBSOCKET_V2] Error creating a TopicReceiver.");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_WEBSOCKET_V2] Cannot setup already existing TopicReceiver for scope/topic %s/%s!", scope == NULL ? "(null)" : scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    if (receiver != NULL && newEndpoint != NULL) {
        celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
        while (hashMapIterator_hasNext(&iter)) {
            celix_properties_t *endpoint = hashMapIterator_nextValue(&iter);
            const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
            if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0 && pubsubEndpoint_matchWithTopicAndScope(endpoint, topic, scope)) {
                pubsub_websocketAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
            }
        }
        celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);
    }

    if (newEndpoint != NULL && outSubscriberEndpoint != NULL) {
        *outSubscriberEndpoint = newEndpoint;
    }

    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_websocketAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic) {
    pubsub_websocket_admin_t *psa = handle;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicReceivers.map, key);
    free(key);
    if (entry != NULL) {
        char *receiverKey = hashMapEntry_getKey(entry);
        pubsub_websocket_topic_receiver_t *receiver = hashMapEntry_getValue(entry);
        hashMap_remove(psa->topicReceivers.map, receiverKey);

        free(receiverKey);
        pubsub_websocketTopicReceiver_destroy(receiver);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

static celix_status_t pubsub_websocketAdmin_connectEndpointToReceiver(pubsub_websocket_admin_t* psa, pubsub_websocket_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
    const char *sockAddress = celix_properties_get(endpoint, PUBSUB_WEBSOCKET_ADDRESS_KEY, NULL);
    long sockPort = celix_properties_getAsLong(endpoint, PUBSUB_WEBSOCKET_PORT_KEY, -1L);

    bool publisher = type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0;

    if (publisher && (sockAddress == NULL || sockPort < 0)) {
        L_WARN("[PSA WEBSOCKET] Error got endpoint without websocket address/port or endpoint type. Properties:");
        const char *key = NULL;
        CELIX_PROPERTIES_FOR_EACH(endpoint, key) {
            L_WARN("[PSA WEBSOCKET] |- %s=%s\n", key, celix_properties_get(endpoint, key, NULL));
        }
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        pubsub_websocketTopicReceiver_connectTo(receiver, sockAddress, sockPort);
    }

    return status;
}

celix_status_t pubsub_websocketAdmin_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_websocket_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_websocket_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            if (pubsubEndpoint_matchWithTopicAndScope(endpoint, pubsub_websocketTopicReceiver_topic(receiver), pubsub_websocketTopicReceiver_scope(receiver))) {
                pubsub_websocketAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    }

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    celix_properties_t *cpy = celix_properties_copy(endpoint);
    const char *uuid = celix_properties_get(cpy, PUBSUB_ENDPOINT_UUID, NULL);
    hashMap_put(psa->discoveredEndpoints.map, (void*)uuid, cpy);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}


static celix_status_t pubsub_websocketAdmin_disconnectEndpointFromReceiver(pubsub_websocket_admin_t* psa, pubsub_websocket_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_websocketTopicReceiver_scope(receiver);
    const char *topic = pubsub_websocketTopicReceiver_topic(receiver);

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *sockAddress = celix_properties_get(endpoint, PUBSUB_WEBSOCKET_ADDRESS_KEY, NULL);
    long sockPort = celix_properties_getAsLong(endpoint, PUBSUB_WEBSOCKET_PORT_KEY, -1L);

    bool publisher = type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0;

    if (publisher && (sockAddress == NULL || sockPort < 0)) {
        L_WARN("[PSA WEBSOCKET] Error got endpoint without websocket address/port or endpoint type. Properties:");
        const char *key = NULL;
        CELIX_PROPERTIES_FOR_EACH(endpoint, key) {
            L_WARN("[PSA WEBSOCKET] |- %s=%s\n", key, celix_properties_get(endpoint, key, NULL));
        }
        status = CELIX_BUNDLE_EXCEPTION;
    } else if (eTopic != NULL && strncmp(eTopic, topic, 1024 * 1024) == 0) {
        if ((scope == NULL && eScope == NULL) || (scope != NULL && eScope != NULL && strncmp(eScope, scope, 1024 * 1024) == 0)) {
            pubsub_websocketTopicReceiver_disconnectFrom(receiver, sockAddress, sockPort);
        }
    }

    return status;
}

celix_status_t pubsub_websocketAdmin_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_websocket_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_websocket_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            pubsub_websocketAdmin_disconnectEndpointFromReceiver(psa, receiver, endpoint);
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    }

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
    celix_properties_t *found = hashMap_remove(psa->discoveredEndpoints.map, (void*)uuid);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    if (found != NULL) {
        celix_properties_destroy(found);
    }

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

bool pubsub_websocketAdmin_executeCommand(void *handle, const char *commandLine __attribute__((unused)), FILE *out, FILE *errStream __attribute__((unused))) {
    pubsub_websocket_admin_t *psa = handle;

    fprintf(out, "\n");
    fprintf(out, "Topic Senders:\n");
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_websocket_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        const char *serType = pubsub_websocketTopicSender_serializerType(sender);
        const char *scope = pubsub_websocketTopicSender_scope(sender);
        const char *topic = pubsub_websocketTopicSender_topic(sender);
        const char *url = pubsub_websocketTopicSender_url(sender);
        fprintf(out, "|- Topic Sender %s/%s\n", scope == NULL ? "(null)" : scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- url             = %s\n", url);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    fprintf(out, "\n");
    fprintf(out, "\nTopic Receivers:\n");
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_websocket_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        const char *serType = pubsub_websocketTopicReceiver_serializerType(receiver);
        const char *scope = pubsub_websocketTopicReceiver_scope(receiver);
        const char *topic = pubsub_websocketTopicReceiver_topic(receiver);
        const char *urlEndp = pubsub_websocketTopicReceiver_url(receiver);

        celix_array_list_t *connected = celix_arrayList_create();
        celix_array_list_t *unconnected = celix_arrayList_create();
        pubsub_websocketTopicReceiver_listConnections(receiver, connected, unconnected);

        fprintf(out, "|- Topic Receiver %s/%s\n", scope == NULL ? "(null)" : scope, topic);
        fprintf(out, "   |- serializer type      = %s\n", serType);
        fprintf(out, "   |- url                  = %s\n", urlEndp);
        for (int i = 0; i < celix_arrayList_size(connected); ++i) {
            char *url = celix_arrayList_get(connected, i);
            fprintf(out, "   |- connected endpoint   = %s\n", url);
            free(url);
        }
        for (int i = 0; i < celix_arrayList_size(unconnected); ++i) {
            char *url = celix_arrayList_get(unconnected, i);
            fprintf(out, "   |- unconnected endpoint = %s\n", url);
            free(url);
        }
        celix_arrayList_destroy(connected);
        celix_arrayList_destroy(unconnected);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    fprintf(out, "\n");

    return true;
}
