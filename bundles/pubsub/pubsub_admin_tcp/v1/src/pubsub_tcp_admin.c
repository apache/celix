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
#include <pubsub_endpoint.h>
#include <pubsub_serializer.h>
#include <ip_utils.h>

#include "pubsub_utils.h"
#include "pubsub_tcp_admin.h"
#include "pubsub_tcp_handler.h"
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_topic_sender.h"
#include "pubsub_tcp_topic_receiver.h"

#define L_DEBUG(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(psa->log, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_tcp_admin {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *log;
    const char *fwUUID;

    char *ipAddress;

    unsigned int basePort;

    double qosSampleScore;
    double qosControlScore;
    double defaultScore;

    bool verbose;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = svcId, value = psa_tcp_serializer_entry_t*
    } serializers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = svcId, value = psa_tcp_protocol_entry_t*
    } protocols;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_tcp_topic_sender_t*
    } topicSenders;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_tcp_topic_sender_t*
    } topicReceivers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = endpoint uuid, value = celix_properties_t* (endpoint)
    } discoveredEndpoints;

    pubsub_tcp_endPointStore_t endpointStore;
};

typedef struct psa_tcp_serializer_entry {
    const char *serType;
    long svcId;
    pubsub_serializer_service_t *svc;
} psa_tcp_serializer_entry_t;

typedef struct psa_tcp_protocol_entry {
    const char *protType;
    long svcId;
    pubsub_protocol_service_t *svc;
} psa_tcp_protocol_entry_t;

static celix_status_t
pubsub_tcpAdmin_connectEndpointToReceiver(pubsub_tcp_admin_t *psa, pubsub_tcp_topic_receiver_t *receiver,
                                          const celix_properties_t *endpoint);

static celix_status_t
pubsub_tcpAdmin_disconnectEndpointFromReceiver(pubsub_tcp_admin_t *psa, pubsub_tcp_topic_receiver_t *receiver,
                                               const celix_properties_t *endpoint);

static bool pubsub_tcpAdmin_endpointIsPublisher(const celix_properties_t *endpoint) {
    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
    return type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0;
}

pubsub_tcp_admin_t *pubsub_tcpAdmin_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper) {
    pubsub_tcp_admin_t *psa = calloc(1, sizeof(*psa));
    psa->ctx = ctx;
    psa->log = logHelper;
    psa->verbose = celix_bundleContext_getPropertyAsBool(ctx, PUBSUB_TCP_VERBOSE_KEY, PUBSUB_TCP_VERBOSE_DEFAULT);
    psa->fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    long basePort = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_BASE_PORT, PSA_TCP_DEFAULT_BASE_PORT);
    psa->basePort = (unsigned int) basePort;
    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_DEFAULT_SCORE_KEY, PSA_TCP_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_QOS_SAMPLE_SCORE_KEY,
                                                                  PSA_TCP_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_QOS_CONTROL_SCORE_KEY,
                                                                   PSA_TCP_DEFAULT_QOS_CONTROL_SCORE);

    celixThreadMutex_create(&psa->serializers.mutex, NULL);
    psa->serializers.map = hashMap_create(NULL, NULL, NULL, NULL);

    celixThreadMutex_create(&psa->protocols.mutex, NULL);
    psa->protocols.map = hashMap_create(NULL, NULL, NULL, NULL);
    celixThreadMutex_create(&psa->topicSenders.mutex, NULL);
    psa->topicSenders.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->topicReceivers.mutex, NULL);
    psa->topicReceivers.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->discoveredEndpoints.mutex, NULL);
    psa->discoveredEndpoints.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->endpointStore.mutex, NULL);
    psa->endpointStore.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    return psa;
}

void pubsub_tcpAdmin_destroy(pubsub_tcp_admin_t *psa) {
    if (psa == NULL) {
        return;
    }

    celixThreadMutex_lock(&psa->endpointStore.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->endpointStore.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcpHandler_t *tcpHandler = hashMapIterator_nextValue(&iter);
        pubsub_tcpHandler_destroy(tcpHandler);
    }
    celixThreadMutex_unlock(&psa->endpointStore.mutex);

    celixThreadMutex_lock(&psa->topicSenders.mutex);
    iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        pubsub_tcpTopicSender_destroy(sender);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_receiver_t *recv = hashMapIterator_nextValue(&iter);
        pubsub_tcpTopicReceiver_destroy(recv);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
    while (hashMapIterator_hasNext(&iter)) {
        celix_properties_t *ep = hashMapIterator_nextValue(&iter);
        celix_properties_destroy(ep);
    }
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    celixThreadMutex_lock(&psa->serializers.mutex);
    iter = hashMapIterator_construct(psa->serializers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_serializer_entry_t *entry = hashMapIterator_nextValue(&iter);
        free(entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);

    celixThreadMutex_lock(&psa->protocols.mutex);
    iter = hashMapIterator_construct(psa->protocols.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_protocol_entry_t *entry = hashMapIterator_nextValue(&iter);
        free(entry);
    }
    celixThreadMutex_unlock(&psa->protocols.mutex);
    //note assuming al psa register services and service tracker are removed.
    celixThreadMutex_destroy(&psa->endpointStore.mutex);
    hashMap_destroy(psa->endpointStore.map, false, false);

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->discoveredEndpoints.mutex);
    hashMap_destroy(psa->discoveredEndpoints.map, false, false);

    celixThreadMutex_destroy(&psa->serializers.mutex);
    hashMap_destroy(psa->serializers.map, false, false);
    celixThreadMutex_destroy(&psa->protocols.mutex);
    hashMap_destroy(psa->protocols.map, false, false);

    free(psa->ipAddress);

    free(psa);
}

void pubsub_tcpAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_tcp_admin_t *psa = handle;

    const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    if (serType == NULL) {
        L_INFO("[PSA_TCP] Ignoring serializer service without %s property", PUBSUB_SERIALIZER_TYPE_KEY);
        return;
    }

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_tcp_serializer_entry_t *entry = hashMap_get(psa->serializers.map, (void *) svcId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->serType = serType;
        entry->svcId = svcId;
        entry->svc = svc;
        hashMap_put(psa->serializers.map, (void *) svcId, entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);
}

void pubsub_tcpAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_tcp_admin_t *psa = handle;
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    //remove serializer
    // 1) First find entry and
    // 2) loop and destroy all topic sender using the serializer and
    // 3) loop and destroy all topic receivers using the serializer
    // Note that it is the responsibility of the topology manager to create new topic senders/receivers

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_tcp_serializer_entry_t *entry = hashMap_remove(psa->serializers.map, (void *) svcId);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (entry != NULL) {
        celixThreadMutex_lock(&psa->topicSenders.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_tcp_topic_sender_t *sender = hashMapEntry_getValue(senderEntry);
            if (sender != NULL && entry->svcId == pubsub_tcpTopicSender_serializerSvcId(sender)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_tcpTopicSender_destroy(sender);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicSenders.mutex);

        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_tcp_topic_receiver_t *receiver = hashMapEntry_getValue(senderEntry);
            if (receiver != NULL && entry->svcId == pubsub_tcpTopicReceiver_serializerSvcId(receiver)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_tcpTopicReceiver_destroy(receiver);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);

        free(entry);
    }
}

void pubsub_tcpAdmin_addProtocolSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_tcp_admin_t *psa = handle;

    const char *protType = celix_properties_get(props, PUBSUB_PROTOCOL_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    if (protType == NULL) {
        L_INFO("[PSA_ZMQ] Ignoring protocol service without %s property", PUBSUB_PROTOCOL_TYPE_KEY);
        return;
    }

    celixThreadMutex_lock(&psa->protocols.mutex);
    psa_tcp_protocol_entry_t *entry = hashMap_get(psa->protocols.map, (void *) svcId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->protType = protType;
        entry->svcId = svcId;
        entry->svc = svc;
        hashMap_put(psa->protocols.map, (void *) svcId, entry);
    }
    celixThreadMutex_unlock(&psa->protocols.mutex);
}

void pubsub_tcpAdmin_removeProtocolSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_tcp_admin_t *psa = handle;
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    //remove protocol
    // 1) First find entry and
    // 2) loop and destroy all topic sender using the protocol and
    // 3) loop and destroy all topic receivers using the protocol
    // Note that it is the responsibility of the topology manager to create new topic senders/receivers

    celixThreadMutex_lock(&psa->protocols.mutex);
    psa_tcp_protocol_entry_t *entry = hashMap_remove(psa->protocols.map, (void *) svcId);
    celixThreadMutex_unlock(&psa->protocols.mutex);

    if (entry != NULL) {
        celixThreadMutex_lock(&psa->topicSenders.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_tcp_topic_sender_t *sender = hashMapEntry_getValue(senderEntry);
            if (sender != NULL && entry->svcId == pubsub_tcpTopicSender_protocolSvcId(sender)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_tcpTopicSender_destroy(sender);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicSenders.mutex);

        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_tcp_topic_receiver_t *receiver = hashMapEntry_getValue(senderEntry);
            if (receiver != NULL && entry->svcId == pubsub_tcpTopicReceiver_protocolSvcId(receiver)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_tcpTopicReceiver_destroy(receiver);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);

        free(entry);
    }
}

celix_status_t pubsub_tcpAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter,
                                              celix_properties_t **topicProperties, double *outScore,
                                              long *outSerializerSvcId, long *outProtocolSvcId) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchPublisher");
    celix_status_t status = CELIX_SUCCESS;
    double score = pubsubEndpoint_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_TCP_ADMIN_TYPE,
                                               psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, true,
                                               topicProperties, outSerializerSvcId, outProtocolSvcId);
    *outScore = score;

    return status;
}

celix_status_t
pubsub_tcpAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties,
                                celix_properties_t **topicProperties, double *outScore, long *outSerializerSvcId,
                                long *outProtocolSvcId) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchSubscriber");
    celix_status_t status = CELIX_SUCCESS;
    double score = pubsubEndpoint_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_TCP_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, true,
                                                topicProperties, outSerializerSvcId, outProtocolSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t
pubsub_tcpAdmin_matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *outMatch) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchEndpoint");
    celix_status_t status = CELIX_SUCCESS;
    bool match = pubsubEndpoint_match(psa->ctx, psa->log, endpoint, PUBSUB_TCP_ADMIN_TYPE, true, NULL, NULL);
    if (outMatch != NULL) {
        *outMatch = match;
    }
    return status;
}

celix_status_t pubsub_tcpAdmin_setupTopicSender(void *handle, const char *scope, const char *topic,
                                                const celix_properties_t *topicProperties, long serializerSvcId,
                                                long protocolSvcId, celix_properties_t **outPublisherEndpoint) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t status = CELIX_SUCCESS;

    //1) Create TopicSender
    //2) Store TopicSender
    //3) Connect existing endpoints
    //4) set outPublisherEndpoint

    celix_properties_t *newEndpoint = NULL;
    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->protocols.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_tcp_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void *) serializerSvcId);
        psa_tcp_protocol_entry_t *protEntry = hashMap_get(psa->protocols.map, (void *) protocolSvcId);
        if (serEntry != NULL && protEntry != NULL) {
            sender = pubsub_tcpTopicSender_create(psa->ctx, psa->log, scope, topic, topicProperties,
                                                  &psa->endpointStore, serializerSvcId, serEntry->svc, protocolSvcId,
                                                  protEntry->svc);
        }
        if (sender != NULL) {
            const char *psaType = PUBSUB_TCP_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            const char *protType = protEntry->protType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType, serType, protType, NULL);
            celix_properties_set(newEndpoint, PUBSUB_TCP_URL_KEY, pubsub_tcpTopicSender_url(sender));

            celix_properties_setBool(newEndpoint, PUBSUB_TCP_STATIC_CONFIGURED, pubsub_tcpTopicSender_isStatic(sender));
            if (pubsub_tcpTopicSender_isPassive(sender)) {
                celix_properties_set(newEndpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_LOCAL_VISIBILITY);
            } else {
                celix_properties_set(newEndpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_SYSTEM_VISIBILITY);
            }
            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL)
              celix_properties_set(newEndpoint, "container_name", cn);
            hashMap_put(psa->topicSenders.map, key, sender);
        } else {
            L_ERROR("[PSA TCP] Error creating a TopicSender");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_TCP] Cannot setup already existing TopicSender for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->protocols.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (newEndpoint != NULL && outPublisherEndpoint != NULL) {
        *outPublisherEndpoint = newEndpoint;
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t status = CELIX_SUCCESS;

    //1) Find and remove TopicSender from map
    //2) destroy topic sender

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicSenders.map, key);
    if (entry != NULL) {
        char *mapKey = hashMapEntry_getKey(entry);
        pubsub_tcp_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        pubsub_tcpTopicSender_destroy(sender);
    } else {
        L_ERROR("[PSA TCP] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists",
                scope == NULL ? "(null)" : scope,
                topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_tcpAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic,
                                                  const celix_properties_t *topicProperties, long serializerSvcId,
                                                  long protocolSvcId, celix_properties_t **outSubscriberEndpoint) {
    pubsub_tcp_admin_t *psa = handle;

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->protocols.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    pubsub_tcp_topic_receiver_t *receiver = hashMap_get(psa->topicReceivers.map, key);
    if (receiver == NULL) {
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void *) serializerSvcId);
        psa_tcp_protocol_entry_t *protEntry = hashMap_get(psa->protocols.map, (void *) protocolSvcId);
        if (serEntry != NULL && protEntry != NULL) {
            receiver = pubsub_tcpTopicReceiver_create(psa->ctx, psa->log, scope, topic, topicProperties,
                                                      &psa->endpointStore, serializerSvcId, serEntry->svc,
                                                      protocolSvcId, protEntry->svc);
        } else {
            L_ERROR("[PSA_TCP] Cannot find serializer or protocol for TopicSender %s/%s", scope == NULL ? "(null)" : scope, topic);
        }
        if (receiver != NULL) {
            const char *psaType = PUBSUB_TCP_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            const char *protType = protEntry->protType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic,
                                                PUBSUB_SUBSCRIBER_ENDPOINT_TYPE, psaType, serType, protType, NULL);
            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
            hashMap_put(psa->topicReceivers.map, key, receiver);
        } else {
            L_ERROR("[PSA TCP] Error creating a TopicReceiver.");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_TCP] Cannot setup already existing TopicReceiver for scope/topic %s/%s!",
                scope == NULL ? "(null)" : scope,
                topic);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    celixThreadMutex_unlock(&psa->protocols.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (receiver != NULL && newEndpoint != NULL) {
        celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
        while (hashMapIterator_hasNext(&iter)) {
            celix_properties_t *endpoint = hashMapIterator_nextValue(&iter);
            if (pubsub_tcpAdmin_endpointIsPublisher(endpoint) && pubsubEndpoint_matchWithTopicAndScope(endpoint, topic, scope)) {
                pubsub_tcpAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
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

celix_status_t pubsub_tcpAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic) {
    pubsub_tcp_admin_t *psa = handle;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicReceivers.map, key);
    free(key);
    if (entry != NULL) {
        char *receiverKey = hashMapEntry_getKey(entry);
        pubsub_tcp_topic_receiver_t *receiver = hashMapEntry_getValue(entry);
        hashMap_remove(psa->topicReceivers.map, receiverKey);

        free(receiverKey);
        pubsub_tcpTopicReceiver_destroy(receiver);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    celix_status_t status = CELIX_SUCCESS;
    return status;
}

static celix_status_t
pubsub_tcpAdmin_connectEndpointToReceiver(pubsub_tcp_admin_t *psa, pubsub_tcp_topic_receiver_t *receiver,
                                          const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *url = celix_properties_get(endpoint, PUBSUB_TCP_URL_KEY, NULL);

    if (url == NULL) {
        const char *admin = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
        L_WARN("[PSA TCP] Error got endpoint without a tcp url (admin: %s, type: %s)", admin, type);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        pubsub_tcpTopicReceiver_connectTo(receiver, url);
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_tcp_admin_t *psa = handle;

    if (pubsub_tcpAdmin_endpointIsPublisher(endpoint)) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            if (pubsubEndpoint_matchWithTopicAndScope(endpoint, pubsub_tcpTopicReceiver_topic(receiver), pubsub_tcpTopicReceiver_scope(receiver))) {
                pubsub_tcpAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    }

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    celix_properties_t *cpy = celix_properties_copy(endpoint);
    const char *uuid = celix_properties_get(cpy, PUBSUB_ENDPOINT_UUID, NULL);
    hashMap_put(psa->discoveredEndpoints.map, (void *) uuid, cpy);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    celix_status_t status = CELIX_SUCCESS;
    return status;
}

static celix_status_t
pubsub_tcpAdmin_disconnectEndpointFromReceiver(pubsub_tcp_admin_t *psa, pubsub_tcp_topic_receiver_t *receiver,
                                               const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *url = celix_properties_get(endpoint, PUBSUB_TCP_URL_KEY, NULL);

    if (url == NULL) {
        L_WARN("[PSA TCP] Error got endpoint without tcp url");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        pubsub_tcpTopicReceiver_disconnectFrom(receiver, url);
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_tcp_admin_t *psa = handle;

    if (pubsub_tcpAdmin_endpointIsPublisher(endpoint)) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            pubsub_tcpAdmin_disconnectEndpointFromReceiver(psa, receiver, endpoint);
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    }

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
    celix_properties_t *found = hashMap_remove(psa->discoveredEndpoints.map, (void *) uuid);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    if (found != NULL) {
        celix_properties_destroy(found);
    }

    celix_status_t status = CELIX_SUCCESS;
    return status;
}

bool pubsub_tcpAdmin_executeCommand(void *handle, const char *commandLine __attribute__((unused)), FILE *out,
                                    FILE *errStream __attribute__((unused))) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t status = CELIX_SUCCESS;
    char *line = celix_utils_strdup(commandLine);
    char *token = line;
    strtok_r(line, " ", &token); //first token is command name
    strtok_r(NULL, " ", &token); //second token is sub command

    if (celix_utils_stringEquals(token, "nr_of_receivers")) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        fprintf(out,"%i\n", hashMap_size(psa->topicReceivers.map));
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    }
    if (celix_utils_stringEquals(token, "nr_of_senders")) {
        celixThreadMutex_lock(&psa->topicSenders.mutex);
        fprintf(out, "%i\n", hashMap_size(psa->topicSenders.map));
        celixThreadMutex_unlock(&psa->topicSenders.mutex);
    }

    fprintf(out, "\n");
    fprintf(out, "Topic Senders:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->protocols.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_tcpTopicSender_serializerSvcId(sender);
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void *) serSvcId);
        long protSvcId = pubsub_tcpTopicSender_protocolSvcId(sender);
        psa_tcp_protocol_entry_t *protEntry = hashMap_get(psa->protocols.map, (void *) protSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *protType = protEntry == NULL ? "!Error!" : protEntry->protType;
        const char *scope = pubsub_tcpTopicSender_scope(sender);
        const char *topic = pubsub_tcpTopicSender_topic(sender);
        const char *url = pubsub_tcpTopicSender_url(sender);
        const char *isPassive = pubsub_tcpTopicSender_isPassive(sender) ? " (passive)" : "";
        const char *postUrl = pubsub_tcpTopicSender_isStatic(sender) ? " (static)" : "";
        fprintf(out, "|- Topic Sender %s/%s\n", scope == NULL ? "(null)" : scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- protocol type = %s\n", protType);
        fprintf(out, "   |- url            = %s%s%s\n", url, postUrl, isPassive);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->protocols.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    fprintf(out, "\n");
    fprintf(out, "\nTopic Receivers:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->protocols.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_tcpTopicReceiver_serializerSvcId(receiver);
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void *) serSvcId);
        long protSvcId = pubsub_tcpTopicReceiver_protocolSvcId(receiver);
        psa_tcp_protocol_entry_t *protEntry = hashMap_get(psa->protocols.map, (void *) protSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *protType = protEntry == NULL ? "!Error!" : protEntry->protType;
        const char *scope = pubsub_tcpTopicReceiver_scope(receiver);
        const char *topic = pubsub_tcpTopicReceiver_topic(receiver);

        celix_array_list_t *connected = celix_arrayList_create();
        celix_array_list_t *unconnected = celix_arrayList_create();
        pubsub_tcpTopicReceiver_listConnections(receiver, connected, unconnected);

        fprintf(out, "|- Topic Receiver %s/%s\n", scope == NULL ? "(null)" : scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- protocol type = %s\n", protType);
        for (int i = 0; i < celix_arrayList_size(connected); ++i) {
            char *url = celix_arrayList_get(connected, i);
            fprintf(out, "   |- connected url   = %s\n", url);
            free(url);
        }
        for (int i = 0; i < celix_arrayList_size(unconnected); ++i) {
            char *url = celix_arrayList_get(unconnected, i);
            fprintf(out, "   |- unconnected url = %s\n", url);
            free(url);
        }
        celix_arrayList_destroy(connected);
        celix_arrayList_destroy(unconnected);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    celixThreadMutex_unlock(&psa->protocols.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);
    fprintf(out, "\n");

    return status;
}

pubsub_admin_metrics_t *pubsub_tcpAdmin_metrics(void *handle) {
    pubsub_tcp_admin_t *psa = handle;
    pubsub_admin_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->psaType, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", PUBSUB_TCP_ADMIN_TYPE);
    result->senders = celix_arrayList_create();
    result->receivers = celix_arrayList_create();

    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        pubsub_admin_sender_metrics_t *metrics = pubsub_tcpTopicSender_metrics(sender);
        celix_arrayList_add(result->senders, metrics);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        pubsub_admin_receiver_metrics_t *metrics = pubsub_tcpTopicReceiver_metrics(receiver);
        celix_arrayList_add(result->receivers, metrics);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    return result;
}
