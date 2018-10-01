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
#include "pubsub_udpmc_admin.h"
#include "pubsub_psa_udpmc_constants.h"
#include "pubsub_udpmc_topic_sender.h"
#include "pubsub_udpmc_topic_receiver.h"

#define PUBSUB_UDPMC_MC_IP_DEFAULT                     "224.100.1.1"
#define PUBSUB_UDPMC_SOCKET_ADDRESS_KEY                "udpmc.socket_address"
#define PUBSUB_UDPMC_SOCKET_PORT_KEY                   "udpmc.socker_port"

#define L_DEBUG(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_INFO, __VA_ARGS__);
#define L_WARN(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, __VA_ARGS__);
#define L_ERROR(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_udpmc_admin {
    celix_bundle_context_t *ctx;
    log_helper_t *log;
    char *ifIpAddress; // The local interface which is used for multicast communication
    char *mcIpAddress; // The multicast IP address
    int sendSocket;
    double qosSampleScore;
    double qosControlScore;
    double defaultScore;
    bool verbose;
    const char *fwUUID;

    struct {
      celix_thread_mutex_t mutex;
      hash_map_t *map;
    } serializers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_udpmc_topic_sender_t*
    } topicSenders;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_udpmc_topic_receiver_t*
    } topicReceivers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = endpoint uuid, value = celix_properties_t*
    } discoveredEndpoints;

};

typedef struct psa_zmq_serializer_entry {
    const char *serType;
    long svcId;
    pubsub_serializer_service_t *svc;
} psa_udpmc_serializer_entry_t;

static celix_status_t udpmc_getIpAddress(const char* interface, char** ip);
static celix_status_t pubsub_udpmcAdmin_connectEndpointToReceiver(pubsub_udpmc_admin_t* psa, pubsub_udpmc_topic_receiver_t *receiver, const celix_properties_t *endpoint);
static celix_status_t pubsub_udpmcAdmin_disconnectEndpointFromReceiver(pubsub_udpmc_admin_t* psa, pubsub_udpmc_topic_receiver_t *receiver, const celix_properties_t *endpoint);


pubsub_udpmc_admin_t* pubsub_udpmcAdmin_create(celix_bundle_context_t *ctx, log_helper_t *logHelper) {
    pubsub_udpmc_admin_t *psa = calloc(1, sizeof(*psa));
    psa->ctx = ctx;
    psa->log = logHelper;
    psa->verbose = celix_bundleContext_getPropertyAsBool(ctx, PUBSUB_UDPMC_VERBOSE_KEY, PUBSUB_UDPMC_VERBOSE_DEFAULT);
    psa->fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);

    int b0 = 0, b1 = 0, b2 = 0, b3 = 0;

    char *mc_ip = NULL;
    char *if_ip = NULL;
    int sendSocket = -1;

    const char *mcIpProp = celix_bundleContext_getProperty(ctx,PUBSUB_UDPMC_IP_KEY , NULL);
    if(mcIpProp != NULL) {
        mc_ip = strdup(mcIpProp);
    }


    const char *mc_prefix = celix_bundleContext_getProperty(ctx,
            PUBSUB_UDPMC_MULTICAST_IP_PREFIX_KEY,
            PUBSUB_UDPMC_MULTICAST_IP_PREFIX_DEFAULT);
    const char *interface = celix_bundleContext_getProperty(ctx, PUBSUB_UDPMC_ITF_KEY, NULL);
    if (udpmc_getIpAddress(interface, &if_ip) != CELIX_SUCCESS) {
        L_WARN("[PSA_UDPMC] Could not retrieve IP address for interface %s", interface);
    } else if (psa->verbose) {
        L_INFO("[PSA_UDPMC] Using IP address %s", if_ip);
    }

    if(if_ip && sscanf(if_ip, "%i.%i.%i.%i", &b0, &b1, &b2, &b3) != 4) {
        logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, "[PSA_UDPMC] Could not parse IP address %s", if_ip);
        b2 = 1;
        b3 = 1;
    }

    asprintf(&mc_ip, "%s.%d.%d",mc_prefix, b2, b3);

    sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(sendSocket == -1) {
        L_ERROR("[PSA_UDPMC] Error creating socket: %s", strerror(errno));
    } else {
        char loop = 1;
        int rc = setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if(rc != 0) {
            L_ERROR("[PSA_UDPMC] Error setsockopt(IP_MULTICAST_LOOP): %s", strerror(errno));
        }
        if (rc == 0) {
            struct in_addr multicast_interface;
            inet_aton(if_ip, &multicast_interface);
            rc = setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_IF, &multicast_interface, sizeof(multicast_interface));
            if (rc != 0) {
                L_ERROR("[PSA_UDPMC] Error setsockopt(IP_MULTICAST_IF): %s", strerror(errno));
            }
        }
        if (rc == 0) {
            psa->sendSocket = sendSocket;
        }
    }

    if (if_ip != NULL) {
        psa->ifIpAddress = if_ip;
    } else {
        psa->ifIpAddress = strdup("127.0.0.1");

    }
    if (psa->verbose) {
        L_INFO("[PSA_UDPMC] Using %s as interface for multicast communication", psa->ifIpAddress);
    }


    if (mc_ip != NULL) {
        psa->mcIpAddress = mc_ip;
    } else {
        psa->mcIpAddress = strdup(PUBSUB_UDPMC_MC_IP_DEFAULT);
    }
    if (psa->verbose) {
        L_INFO("[PSA_UDPMC] Using %s for service annunciation", psa->mcIpAddress);
    }

    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_DEFAULT_SCORE_KEY, PSA_UDPMC_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_QOS_SAMPLE_SCORE_KEY, PSA_UDPMC_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_QOS_CONTROL_SCORE_KEY, PSA_UDPMC_DEFAULT_QOS_CONTROL_SCORE);

    celixThreadMutex_create(&psa->serializers.mutex, NULL);
    psa->serializers.map = hashMap_create(NULL, NULL, NULL, NULL);

    celixThreadMutex_create(&psa->topicSenders.mutex, NULL);
    psa->topicSenders.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->topicReceivers.mutex, NULL);
    psa->topicReceivers.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->discoveredEndpoints.mutex, NULL);
    psa->discoveredEndpoints.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    return psa;
}

void pubsub_udpmcAdmin_destroy(pubsub_udpmc_admin_t *psa) {
    if (psa == NULL) {
        return;
    }

    //note assuming al psa register services and service tracker are removed.

    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        pubsub_udpmcTopicSender_destroy(sender);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_receiver_t *recv = hashMapIterator_nextValue(&iter);
        pubsub_udpmcTopicReceiver_destroy(recv);
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
        psa_udpmc_serializer_entry_t *entry = hashMapIterator_nextValue(&iter);
        free(entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->discoveredEndpoints.mutex);
    hashMap_destroy(psa->discoveredEndpoints.map, true, false);

    celixThreadMutex_destroy(&psa->serializers.mutex);
    hashMap_destroy(psa->serializers.map, false, false);

    free(psa->mcIpAddress);
    free(psa->ifIpAddress);
    free(psa);
}

celix_status_t pubsub_udpmcAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, double *outScore, long *outSerializerSvcId) {
    pubsub_udpmc_admin_t *psa = handle;
    L_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchPublisher");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_UDPMC_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    *outScore = score;

    return status;
}

celix_status_t pubsub_udpmcAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, double *outScore, long *outSerializerSvcId) {
    pubsub_udpmc_admin_t *psa = handle;
    L_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchSubscriber");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_UDPMC_ADMIN_TYPE,
            psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t pubsub_udpmcAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, bool *outMatch) {
    pubsub_udpmc_admin_t *psa = handle;
    L_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    bool match = pubsub_utils_matchEndpoint(psa->ctx, endpoint, PUBSUB_UDPMC_ADMIN_TYPE, NULL);
    if (outMatch != NULL) {
        *outMatch = match;
    }
    return status;
}

celix_status_t pubsub_udpmcAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **outPublisherEndpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Create TopicSender
    //2) Store TopicSender
    //3) Connect existing endpoints
    //4) set outPublisherEndpoint

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_udpmc_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        psa_udpmc_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            sender = pubsub_udpmcTopicSender_create(psa->ctx, scope, topic, serializerSvcId, serEntry->svc, psa->sendSocket, psa->mcIpAddress);
        }
        if (sender != NULL) {
            const char *psaType = PSA_UDPMC_PUBSUB_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic,
                    PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType, serType, NULL);
            celix_properties_set(newEndpoint, PUBSUB_UDPMC_SOCKET_ADDRESS_KEY, pubsub_udpmcTopicSender_socketAddress(sender));
            celix_properties_setLong(newEndpoint, PUBSUB_UDPMC_SOCKET_PORT_KEY, pubsub_udpmcTopicSender_socketPort(sender));
            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
            hashMap_put(psa->topicSenders.map, key, sender);
        } else {
            free(key);
            L_ERROR("[PSA UDPMC] Error creating a valid TopicSender. Endpoints are not valid");
        }
    } else {
        free(key);
        L_ERROR("[PSA_UDPMC] Cannot setup already existing TopicSender for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (sender != NULL && newEndpoint != NULL) {
        //TODO connect endpoints to sender, NOTE is this needed for a udpmc topic sender?
    }

    if (newEndpoint != NULL && outPublisherEndpoint != NULL) {
        *outPublisherEndpoint = newEndpoint;
    }

    return status;
}

celix_status_t pubsub_udpmcAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic) {
    pubsub_udpmc_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Find and remove TopicSender from map
    //2) destroy topic sender

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicSenders.map, key);
    if (entry != NULL) {
        char *mapKey = hashMapEntry_getKey(entry);
        pubsub_udpmc_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        //TODO disconnect endpoints to sender. note is this needed for a udpmc topic sender?
        pubsub_udpmcTopicSender_destroy(sender);
    } else {
        L_ERROR("[PSA UDPMC] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_udpmcAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **outSubscriberEndpoint) {
    pubsub_udpmc_admin_t *psa = handle;

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    pubsub_udpmc_topic_receiver_t *receiver = hashMap_get(psa->topicReceivers.map, key);
    if (receiver == NULL) {
        psa_udpmc_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            receiver = pubsub_udpmcTopicReceiver_create(psa->ctx, scope, topic, psa->ifIpAddress, serializerSvcId, serEntry->svc);
        }
        if (receiver != NULL) {
            const char *psaType = PSA_UDPMC_PUBSUB_ADMIN_TYPE;
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
            L_ERROR("[PSA UDPMC] Error creating a valid TopicReceiver. Endpoints are not valid");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_UDPMC] Cannot setup already existing TopicReceiver for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (receiver != NULL && newEndpoint != NULL) {
        celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->discoveredEndpoints.map);
        while (hashMapIterator_hasNext(&iter)) {
            celix_properties_t *endpoint = hashMapIterator_nextValue(&iter);
            pubsub_udpmcAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
        }
        celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);
    }

    if (newEndpoint != NULL && outSubscriberEndpoint != NULL) {
        *outSubscriberEndpoint = newEndpoint;
    }

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_udpmcAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic) {
    pubsub_udpmc_admin_t *psa = handle;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicReceivers.map, key);
    free(key);
    if (entry != NULL) {
        char *receiverKey = hashMapEntry_getKey(entry);
        pubsub_udpmc_topic_receiver_t *receiver = hashMapEntry_getValue(entry);
        hashMap_remove(psa->topicReceivers.map, receiverKey);

        free(receiverKey);
        pubsub_udpmcTopicReceiver_destroy(receiver);
    }
    celixThreadMutex_lock(&psa->topicReceivers.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

static celix_status_t pubsub_udpmcAdmin_connectEndpointToReceiver(pubsub_udpmc_admin_t* psa, pubsub_udpmc_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_udpmcTopicReceiver_scope(receiver);
    const char *topic = pubsub_udpmcTopicReceiver_topic(receiver);

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *sockAdress = celix_properties_get(endpoint, PUBSUB_PSA_UDPMC_SOCKET_ADDRESS_KEY, NULL);
    long sockPort = celix_properties_getAsLong(endpoint, PUBSUB_PSA_UDPMC_SOCKET_PORT_KEY, -1L);

    if (type == NULL || sockAdress == NULL || sockPort < 0) {
        fprintf(stderr, "[PSA UPDMC] Error got endpoint without udpmc socket address/port or endpoint type");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL && type != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0 &&
            strncmp(type, PUBSUB_PUBLISHER_ENDPOINT_TYPE, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
            pubsub_udpmcTopicReceiver_connectTo(receiver, sockAdress, sockPort);
        }
    }

    return status;
}

celix_status_t pubsub_udpmcAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        pubsub_udpmcAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

    celixThreadMutex_lock(&psa->discoveredEndpoints.mutex);
    celix_properties_t *cpy = celix_properties_copy(endpoint);
    const char *uuid = celix_properties_get(cpy, PUBSUB_ENDPOINT_UUID, NULL);
    hashMap_put(psa->discoveredEndpoints.map, (void*)uuid, cpy);
    celixThreadMutex_unlock(&psa->discoveredEndpoints.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}


static celix_status_t pubsub_udpmcAdmin_disconnectEndpointFromReceiver(pubsub_udpmc_admin_t* psa, pubsub_udpmc_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_udpmcTopicReceiver_scope(receiver);
    const char *topic = pubsub_udpmcTopicReceiver_topic(receiver);

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *sockAdress = celix_properties_get(endpoint, PUBSUB_PSA_UDPMC_SOCKET_ADDRESS_KEY, NULL);
    long sockPort = celix_properties_getAsLong(endpoint, PUBSUB_PSA_UDPMC_SOCKET_PORT_KEY, -1L);

    if (type == NULL || sockAdress == NULL || sockPort < 0) {
        fprintf(stderr, "[PSA UPDMC] Error got endpoint without udpmc socket address/port or endpoint type");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL && type != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0 &&
            strncmp(type, PUBSUB_PUBLISHER_ENDPOINT_TYPE, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
            pubsub_udpmcTopicReceiver_disconnectFrom(receiver, sockAdress, sockPort);
        }
    }

    return status;
}

celix_status_t pubsub_udpmcAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        pubsub_udpmcAdmin_disconnectEndpointFromReceiver(psa, receiver, endpoint);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);

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

celix_status_t pubsub_udpmcAdmin_executeCommand(void *handle, char *commandLine __attribute__((unused)), FILE *out, FILE *errStream __attribute__((unused))) {
    pubsub_udpmc_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    fprintf(out, "\n");
    fprintf(out, "Topic Senders:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_udpmcTopicSender_serializerSvcId(sender);
        psa_udpmc_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_udpmcTopicSender_scope(sender);
        const char *topic = pubsub_udpmcTopicSender_topic(sender);
        const char *sockAddr = pubsub_udpmcTopicSender_socketAddress(sender);
        fprintf(out, "|- Topic Sender %s/%s\n", scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- socket address  = %s\n", sockAddr);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    fprintf(out, "\n");
    fprintf(out, "\nTopic Receivers:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_udpmc_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_udpmcTopicReceiver_serializerSvcId(receiver);
        psa_udpmc_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_udpmcTopicReceiver_scope(receiver);
        const char *topic = pubsub_udpmcTopicReceiver_topic(receiver);
        fprintf(out, "|- Topic Receiver %s/%s\n", scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
    }
    celixThreadMutex_unlock(&psa->topicReceivers.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);
    fprintf(out, "\n");

    //TODO topic receivers/senders connection count

    return status;
}

void pubsub_udpmcAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_udpmc_admin_t *psa = handle;

    const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    if (serType == NULL) {
        L_INFO("[PSA_ZMQ] Ignoring serializer service without %s property", PUBSUB_SERIALIZER_TYPE_KEY);
        return;
    }

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_udpmc_serializer_entry_t *entry = hashMap_get(psa->serializers.map, (void*)svcId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->serType = serType;
        entry->svcId = svcId;
        entry->svc = svc;
        hashMap_put(psa->serializers.map, (void*)svcId, entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);
}

void pubsub_udpmcAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_udpmc_admin_t *psa = handle;
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    //remove serializer
    // 1) First find entry and
    // 2) loop and destroy all topic sender using the serializer and
    // 3) loop and destroy all topic receivers using the serializer
    // Note that it is the responsibility of the topology manager to create new topic senders/receivers

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_udpmc_serializer_entry_t *entry = hashMap_remove(psa->serializers.map, (void*)svcId);
    if (entry != NULL) {
        celixThreadMutex_lock(&psa->topicSenders.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_udpmc_topic_sender_t *sender = hashMapEntry_getValue(senderEntry);
            if (sender != NULL && entry->svcId == pubsub_udpmcTopicSender_serializerSvcId(sender)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_udpmcTopicSender_destroy(sender);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicSenders.mutex);

        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_udpmc_topic_receiver_t *receiver = hashMapEntry_getValue(senderEntry);
            if (receiver != NULL && entry->svcId == pubsub_udpmcTopicReceiver_serializerSvcId(receiver)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_udpmcTopicReceiver_destroy(receiver);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);

        free(entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);
}

#ifndef ANDROID
static celix_status_t udpmc_getIpAddress(const char* interface, char** ip) {
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