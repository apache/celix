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

#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <pubsub_endpoint.h>
#include <pubsub_serializer.h>

#include "pubsub_utils.h"
#include "pubsub_tcp_admin.h"
#include "pubsub_tcp_handler.h"
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_topic_sender.h"
#include "pubsub_tcp_topic_receiver.h"

#define L_DEBUG(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_tcp_admin {
    celix_bundle_context_t *ctx;
    log_helper_t *log;
    const char *fwUUID;

    char* ipAddress;

    unsigned int basePort;
    unsigned int maxPort;

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

static celix_status_t tcp_getIpAddress(const char* interface, char** ip);
static celix_status_t pubsub_tcpAdmin_connectEndpointToReceiver(pubsub_tcp_admin_t* psa, pubsub_tcp_topic_receiver_t *receiver, const celix_properties_t *endpoint);
static celix_status_t pubsub_tcpAdmin_disconnectEndpointFromReceiver(pubsub_tcp_admin_t* psa, pubsub_tcp_topic_receiver_t *receiver, const celix_properties_t *endpoint);


pubsub_tcp_admin_t* pubsub_tcpAdmin_create(celix_bundle_context_t *ctx, log_helper_t *logHelper) {
    pubsub_tcp_admin_t *psa = calloc(1, sizeof(*psa));
    psa->ctx = ctx;
    psa->log = logHelper;
    psa->verbose = celix_bundleContext_getPropertyAsBool(ctx, PUBSUB_TCP_VERBOSE_KEY, PUBSUB_TCP_VERBOSE_DEFAULT);
    psa->fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);

    char *ip = NULL;
    const char *confIp = celix_bundleContext_getProperty(ctx, PUBSUB_TCP_PSA_IP_KEY , NULL);
    if (confIp != NULL) {
        ip = strndup(confIp, 1024);
    }

    if (ip == NULL) {
        //TODO try to get ip from subnet (CIDR)
    }

    if (ip == NULL) {
        //try to get ip from itf
        const char *interface = celix_bundleContext_getProperty(ctx, PUBSUB_TCP_PSA_ITF_KEY, NULL);
        tcp_getIpAddress(interface, &ip);
    }

    if (ip == NULL) {
        L_WARN("[PSA_TCP] Could not determine IP address for PSA, using default ip (%s)", PUBSUB_TCP_DEFAULT_IP);
        ip = strndup(PUBSUB_TCP_DEFAULT_IP, 1024);
    }

    psa->ipAddress = ip;
    if (psa->verbose) {
        L_INFO("[PSA_TCP] Using %s for service annunciation", ip);
    }


    long basePort = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_BASE_PORT, PSA_TCP_DEFAULT_BASE_PORT);
    long maxPort = celix_bundleContext_getPropertyAsLong(ctx, PSA_TCP_MAX_PORT, PSA_TCP_DEFAULT_MAX_PORT);
    psa->basePort = (unsigned int)basePort;
    psa->maxPort = (unsigned int)maxPort;
    if (psa->verbose) {
        L_INFO("[PSA_TCP] Using base till max port: %i till %i", psa->basePort, psa->maxPort);
    }

    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_DEFAULT_SCORE_KEY, PSA_TCP_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_QOS_SAMPLE_SCORE_KEY, PSA_TCP_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_TCP_QOS_CONTROL_SCORE_KEY, PSA_TCP_DEFAULT_QOS_CONTROL_SCORE);

    celixThreadMutex_create(&psa->serializers.mutex, NULL);
    psa->serializers.map = hashMap_create(NULL, NULL, NULL, NULL);

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
      pubsub_tcpHandler_t* tcpHandler = hashMapIterator_nextValue(&iter);
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

    //note assuming al psa register services and service tracker are removed.
    celixThreadMutex_destroy(&psa->endpointStore.mutex);
    hashMap_destroy(psa->endpointStore.map, true, false);

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->discoveredEndpoints.mutex);
    hashMap_destroy(psa->discoveredEndpoints.map, false, false);

    celixThreadMutex_destroy(&psa->serializers.mutex);
    hashMap_destroy(psa->serializers.map, false, false);

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
    psa_tcp_serializer_entry_t *entry = hashMap_get(psa->serializers.map, (void*)svcId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->serType = serType;
        entry->svcId = svcId;
        entry->svc = svc;
        hashMap_put(psa->serializers.map, (void*)svcId, entry);
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
    psa_tcp_serializer_entry_t *entry = hashMap_remove(psa->serializers.map, (void*)svcId);
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

celix_status_t pubsub_tcpAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, celix_properties_t **topicProperties, double *outScore, long *outSerializerSvcId) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchPublisher");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_TCP_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, topicProperties, outSerializerSvcId);
    *outScore = score;

    return status;
}

celix_status_t pubsub_tcpAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, celix_properties_t **topicProperties, double *outScore, long *outSerializerSvcId) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchSubscriber");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_TCP_ADMIN_TYPE,
            psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, topicProperties, outSerializerSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t pubsub_tcpAdmin_matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *outMatch) {
    pubsub_tcp_admin_t *psa = handle;
    L_DEBUG("[PSA_TCP] pubsub_tcpAdmin_matchEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    bool match = pubsub_utils_matchEndpoint(psa->ctx, endpoint, PUBSUB_TCP_ADMIN_TYPE, NULL);
    if (outMatch != NULL) {
        *outMatch = match;
    }
    return status;
}

celix_status_t pubsub_tcpAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, celix_properties_t **outPublisherEndpoint) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Create TopicSender
    //2) Store TopicSender
    //3) Connect existing endpoints
    //4) set outPublisherEndpoint

    celix_properties_t *newEndpoint = NULL;

    const char *staticBindUrl = topicProperties != NULL ? celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_BIND_URL, NULL) : NULL;
    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);

    /* Check if it's a static client endpoint, needs also be declared static  */
    bool isEndPointTypeClient = false;
    const char *endPointType = celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_ENDPOINT_TYPE, NULL);
    if (endPointType != NULL) {
      if (strncmp(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_CLIENT, endPointType, strlen(PUBSUB_TCP_STATIC_ENDPOINT_TYPE_CLIENT)) ==0) {
        isEndPointTypeClient = true;
      }
    }
    const char *staticConnectUrls = ((topicProperties != NULL) && isEndPointTypeClient) ? celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_CONNECT_URLS, NULL) : NULL;

    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_tcp_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            sender = pubsub_tcpTopicSender_create(psa->ctx, psa->log, scope, topic, topicProperties, &psa->endpointStore, serializerSvcId, serEntry->svc,
                     psa->ipAddress, staticBindUrl, psa->basePort, psa->maxPort);
        }
        if (sender != NULL) {
            const char *psaType = PUBSUB_TCP_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType, serType, NULL);
            celix_properties_set(newEndpoint, PUBSUB_TCP_URL_KEY, pubsub_tcpTopicSender_url(sender));

            //if configured use a static discover url
            const char *staticDiscUrl = celix_properties_get(topicProperties, PUBSUB_TCP_STATIC_DISCOVER_URL, NULL);
            if (staticDiscUrl != NULL) {
                celix_properties_set(newEndpoint, PUBSUB_TCP_URL_KEY, staticDiscUrl);
            }
            celix_properties_setBool(newEndpoint, PUBSUB_TCP_STATIC_CONFIGURED, staticBindUrl != NULL || staticDiscUrl != NULL || staticConnectUrls!=NULL);
            celix_properties_set(newEndpoint, PUBSUB_ENDPOINT_VISIBILITY, PUBSUB_ENDPOINT_SYSTEM_VISIBILITY);

            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
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
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (sender != NULL && newEndpoint != NULL) {
        //TODO connect endpoints to sender, NOTE is this needed for a tcp topic sender?
    }

    if (newEndpoint != NULL && outPublisherEndpoint != NULL) {
        *outPublisherEndpoint = newEndpoint;
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Find and remove TopicSender from map
    //2) destroy topic sender

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicSenders.map, key);
    if (entry != NULL) {
        char *mapKey = hashMapEntry_getKey(entry);
        pubsub_tcp_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        //TODO disconnect endpoints to sender. note is this needed for a tcp topic sender?
        pubsub_tcpTopicSender_destroy(sender);
    } else {
        L_ERROR("[PSA TCP] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_tcpAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, celix_properties_t **outSubscriberEndpoint) {
    pubsub_tcp_admin_t *psa = handle;

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    pubsub_tcp_topic_receiver_t *receiver = hashMap_get(psa->topicReceivers.map, key);
    if (receiver == NULL) {
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            receiver = pubsub_tcpTopicReceiver_create(psa->ctx, psa->log, scope, topic, topicProperties, &psa->endpointStore,
                    serializerSvcId, serEntry->svc);
        } else {
            L_ERROR("[PSA_TCP] Cannot find serializer for TopicSender %s/%s", scope, topic);
        }
        if (receiver != NULL) {
            const char *psaType = PUBSUB_TCP_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic,
                                                PUBSUB_SUBSCRIBER_ENDPOINT_TYPE, psaType, serType, NULL);
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
        L_ERROR("[PSA_TCP] Cannot setup already existing TopicReceiver for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (receiver != NULL && newEndpoint != NULL) {
        celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
        while (hashMapIterator_hasNext(&iter)) {
            celix_properties_t *endpoint = hashMapIterator_nextValue(&iter);
            const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
            if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
                pubsub_tcpAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
            }
        }
        celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);
    }

    if (newEndpoint != NULL && outSubscriberEndpoint != NULL) {
        *outSubscriberEndpoint = newEndpoint;
    }

    celix_status_t  status = CELIX_SUCCESS;
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
    celixThreadMutex_lock(&psa->topicReceivers.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

static celix_status_t pubsub_tcpAdmin_connectEndpointToReceiver(pubsub_tcp_admin_t* psa, pubsub_tcp_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_tcpTopicReceiver_scope(receiver);
    const char *topic = pubsub_tcpTopicReceiver_topic(receiver);

    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *url = celix_properties_get(endpoint, PUBSUB_TCP_URL_KEY, NULL);

    if (url == NULL) {
        const char *admin = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
        L_WARN("[PSA TCP] Error got endpoint without a tcp url (admin: %s, type: %s)", admin , type);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0) {
            pubsub_tcpTopicReceiver_connectTo(receiver, url);
        }
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_tcp_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            pubsub_tcpAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
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


static celix_status_t pubsub_tcpAdmin_disconnectEndpointFromReceiver(pubsub_tcp_admin_t* psa, pubsub_tcp_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_tcpTopicReceiver_scope(receiver);
    const char *topic = pubsub_tcpTopicReceiver_topic(receiver);

    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *url = celix_properties_get(endpoint, PUBSUB_TCP_URL_KEY, NULL);

    if (url == NULL) {
        L_WARN("[PSA TCP] Error got endpoint without tcp url");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0) {
            pubsub_tcpTopicReceiver_disconnectFrom(receiver, url);
        }
    }

    return status;
}

celix_status_t pubsub_tcpAdmin_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_tcp_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
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
    celix_properties_t *found = hashMap_remove(psa->discoveredEndpoints.map, (void*)uuid);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    if (found != NULL) {
        celix_properties_destroy(found);
    }

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_tcpAdmin_executeCommand(void *handle, char *commandLine __attribute__((unused)), FILE *out, FILE *errStream __attribute__((unused))) {
    pubsub_tcp_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    fprintf(out, "\n");
    fprintf(out, "Topic Senders:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_tcpTopicSender_serializerSvcId(sender);
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_tcpTopicSender_scope(sender);
        const char *topic = pubsub_tcpTopicSender_topic(sender);
        const char *url = pubsub_tcpTopicSender_url(sender);
        const char *postUrl = pubsub_tcpTopicSender_isStatic(sender) ? " (static)" : "";
        fprintf(out, "|- Topic Sender %s/%s\n", scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- url            = %s%s\n", url, postUrl);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    fprintf(out, "\n");
    fprintf(out, "\nTopic Receivers:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_tcp_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_tcpTopicReceiver_serializerSvcId(receiver);
        psa_tcp_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_tcpTopicReceiver_scope(receiver);
        const char *topic = pubsub_tcpTopicReceiver_topic(receiver);

        celix_array_list_t *connected = celix_arrayList_create();
        celix_array_list_t *unconnected = celix_arrayList_create();
        pubsub_tcpTopicReceiver_listConnections(receiver, connected, unconnected);

        fprintf(out, "|- Topic Receiver %s/%s\n", scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
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
    celixThreadMutex_unlock(&psa->serializers.mutex);
    fprintf(out, "\n");

    return status;
}

pubsub_admin_metrics_t* pubsub_tcpAdmin_metrics(void *handle) {
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

#ifndef ANDROID
static celix_status_t tcp_getIpAddress(const char* interface, char** ip) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != NULL && status != CELIX_SUCCESS; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
                if (interface == NULL) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
                else if (strcmp(ifa->ifa_name, interface) == 0) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
            }
        }

        freeifaddrs(ifaddr);
    }

    return status;
}
#endif
