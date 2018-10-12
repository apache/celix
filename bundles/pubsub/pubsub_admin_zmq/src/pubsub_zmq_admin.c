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
#include <czmq.h>
#include <pubsub_serializer.h>

#include "pubsub_utils.h"
#include "pubsub_zmq_admin.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_zmq_topic_sender.h"
#include "pubsub_zmq_topic_receiver.h"

#define L_DEBUG(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_zmq_admin {
    celix_bundle_context_t *ctx;
    log_helper_t *log;
    const char *fwUUID;

    char* ipAddress;
    zactor_t* zmq_auth;

    unsigned int basePort;
    unsigned int maxPort;

    double qosSampleScore;
    double qosControlScore;
    double defaultScore;

    bool verbose;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = svcId, value = psa_zmq_serializer_entry_t*
    } serializers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_zmq_topic_sender_t*
    } topicSenders;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = scope:topic key, value = pubsub_zmq_topic_sender_t*
    } topicReceivers;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = endpoint uuid, value = celix_properties_t* (endpoint)
    } discoveredEndpoints;

};

typedef struct psa_zmq_serializer_entry {
    const char *serType;
    long svcId;
    pubsub_serializer_service_t *svc;
} psa_zmq_serializer_entry_t;

static celix_status_t zmq_getIpAddress(const char* interface, char** ip);
static celix_status_t pubsub_zmqAdmin_connectEndpointToReceiver(pubsub_zmq_admin_t* psa, pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *endpoint);
static celix_status_t pubsub_zmqAdmin_disconnectEndpointFromReceiver(pubsub_zmq_admin_t* psa, pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *endpoint);


pubsub_zmq_admin_t* pubsub_zmqAdmin_create(celix_bundle_context_t *ctx, log_helper_t *logHelper) {
    pubsub_zmq_admin_t *psa = calloc(1, sizeof(*psa));
    psa->ctx = ctx;
    psa->log = logHelper;
    psa->verbose = celix_bundleContext_getPropertyAsBool(ctx, PUBSUB_ZMQ_VERBOSE_KEY, PUBSUB_ZMQ_VERBOSE_DEFAULT);
    psa->fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);

    char *ip = NULL;
    const char *confIp = celix_bundleContext_getProperty(ctx, PUBSUB_ZMQ_PSA_IP_KEY , NULL);
    if (confIp != NULL) {
        ip = strndup(confIp, 1024);
    }

    if (ip == NULL) {
        //TODO try to get ip from subnet (CIDR)
    }

    if (ip == NULL) {
        //try to get ip from itf
        const char *interface = celix_bundleContext_getProperty(ctx, PUBSUB_ZMQ_PSA_ITF_KEY, NULL);
        zmq_getIpAddress(interface, &ip);
    }

    if (ip == NULL) {
        L_WARN("[PSA_ZMQ] Could not determine IP address for PSA, using default ip (%s)", PUBSUB_ZMQ_DEFAULT_IP);
        ip = strndup(PUBSUB_ZMQ_DEFAULT_IP, 1024);
    }

    psa->ipAddress = ip;
    if (psa->verbose) {
        L_INFO("[PSA_ZMQ] Using %s for service annunciation", ip);
    }


    long basePort = celix_bundleContext_getPropertyAsLong(ctx, PSA_ZMQ_BASE_PORT, PSA_ZMQ_DEFAULT_BASE_PORT);
    long maxPort = celix_bundleContext_getPropertyAsLong(ctx, PSA_ZMQ_MAX_PORT, PSA_ZMQ_DEFAULT_MAX_PORT);
    psa->basePort = (unsigned int)basePort;
    psa->maxPort = (unsigned int)maxPort;
    if (psa->verbose) {
        L_INFO("[PSA_ZMQ] Using base till max port: %i till %i", psa->basePort, psa->maxPort);
    }

    // Disable Signal Handling by CZMQ
    setenv("ZSYS_SIGHANDLER", "false", true);

    long nrThreads = celix_bundleContext_getPropertyAsLong(ctx, PUBSUB_ZMQ_NR_THREADS_KEY, 0);
    if (nrThreads > 0) {
        zsys_set_io_threads((size_t)nrThreads);
        L_INFO("[PSA_ZMQ] Using %d threads for ZMQ", (size_t)nrThreads);
    }


#ifdef BUILD_WITH_ZMQ_SECURITY
    // Setup authenticator
    zactor_t* auth = zactor_new (zauth, NULL);
    zstr_sendx(auth, "VERBOSE", NULL);

    // Load all public keys of subscribers into the application
    // This step is done for authenticating subscribers
    char curve_folder_path[MAX_KEY_FOLDER_PATH_LENGTH];
    char* keys_bundle_dir = pubsub_getKeysBundleDir(context);
    snprintf(curve_folder_path, MAX_KEY_FOLDER_PATH_LENGTH, "%s/META-INF/keys/subscriber/public", keys_bundle_dir);
    zstr_sendx (auth, "CURVE", curve_folder_path, NULL);
    free(keys_bundle_dir);

    (*admin)->zmq_auth = auth;
#endif

    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_ZMQ_DEFAULT_SCORE_KEY, PSA_ZMQ_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_ZMQ_QOS_SAMPLE_SCORE_KEY, PSA_ZMQ_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_ZMQ_QOS_CONTROL_SCORE_KEY, PSA_ZMQ_DEFAULT_QOS_CONTROL_SCORE);

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

void pubsub_zmqAdmin_destroy(pubsub_zmq_admin_t *psa) {
    if (psa == NULL) {
        return;
    }

    //note assuming al psa register services and service tracker are removed.

    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_zmq_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        pubsub_zmqTopicSender_destroy(sender);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);

    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_zmq_topic_receiver_t *recv = hashMapIterator_nextValue(&iter);
        pubsub_zmqTopicReceiver_destroy(recv);
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
        psa_zmq_serializer_entry_t *entry = hashMapIterator_nextValue(&iter);
        free(entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->discoveredEndpoints.mutex);
    hashMap_destroy(psa->discoveredEndpoints.map, false, false);

    celixThreadMutex_destroy(&psa->serializers.mutex);
    hashMap_destroy(psa->serializers.map, false, false);

    if (psa->zmq_auth != NULL){
        zactor_destroy(&psa->zmq_auth);
    }

    free(psa->ipAddress);

    free(psa);
}

void pubsub_zmqAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_zmq_admin_t *psa = handle;

    const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    if (serType == NULL) {
        L_INFO("[PSA_ZMQ] Ignoring serializer service without %s property", PUBSUB_SERIALIZER_TYPE_KEY);
        return;
    }

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_zmq_serializer_entry_t *entry = hashMap_get(psa->serializers.map, (void*)svcId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->serType = serType;
        entry->svcId = svcId;
        entry->svc = svc;
        hashMap_put(psa->serializers.map, (void*)svcId, entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);
}

void pubsub_zmqAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_zmq_admin_t *psa = handle;
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

    //remove serializer
    // 1) First find entry and
    // 2) loop and destroy all topic sender using the serializer and
    // 3) loop and destroy all topic receivers using the serializer
    // Note that it is the responsibility of the topology manager to create new topic senders/receivers

    celixThreadMutex_lock(&psa->serializers.mutex);
    psa_zmq_serializer_entry_t *entry = hashMap_remove(psa->serializers.map, (void*)svcId);
    if (entry != NULL) {
        celixThreadMutex_lock(&psa->topicSenders.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_zmq_topic_sender_t *sender = hashMapEntry_getValue(senderEntry);
            if (sender != NULL && entry->svcId == pubsub_zmqTopicSender_serializerSvcId(sender)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_zmqTopicSender_destroy(sender);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicSenders.mutex);

        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_t *senderEntry = hashMapIterator_nextEntry(&iter);
            pubsub_zmq_topic_receiver_t *receiver = hashMapEntry_getValue(senderEntry);
            if (receiver != NULL && entry->svcId == pubsub_zmqTopicReceiver_serializerSvcId(receiver)) {
                char *key = hashMapEntry_getKey(senderEntry);
                hashMapIterator_remove(&iter);
                pubsub_zmqTopicReceiver_destroy(receiver);
                free(key);
            }
        }
        celixThreadMutex_unlock(&psa->topicReceivers.mutex);

        free(entry);
    }
    celixThreadMutex_unlock(&psa->serializers.mutex);
}

celix_status_t pubsub_zmqAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, double *outScore, long *outSerializerSvcId) {
    pubsub_zmq_admin_t *psa = handle;
    L_DEBUG("[PSA_ZMQ] pubsub_zmqAdmin_matchPublisher");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_ZMQ_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    *outScore = score;

    return status;
}

celix_status_t pubsub_zmqAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, double *outScore, long *outSerializerSvcId) {
    pubsub_zmq_admin_t *psa = handle;
    L_DEBUG("[PSA_ZMQ] pubsub_zmqAdmin_matchSubscriber");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_ZMQ_ADMIN_TYPE,
            psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t pubsub_zmqAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, bool *outMatch) {
    pubsub_zmq_admin_t *psa = handle;
    L_DEBUG("[PSA_ZMQ] pubsub_zmqAdmin_matchEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    bool match = pubsub_utils_matchEndpoint(psa->ctx, endpoint, PUBSUB_ZMQ_ADMIN_TYPE, NULL);
    if (outMatch != NULL) {
        *outMatch = match;
    }
    return status;
}

celix_status_t pubsub_zmqAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **outPublisherEndpoint) {
    pubsub_zmq_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Create TopicSender
    //2) Store TopicSender
    //3) Connect existing endpoints
    //4) set outPublisherEndpoint

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);

    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_zmq_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        psa_zmq_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            sender = pubsub_zmqTopicSender_create(psa->ctx, psa->log, scope, topic, serializerSvcId, serEntry->svc, psa->ipAddress,
                                                  psa->basePort, psa->maxPort);
        }
        if (sender != NULL) {
            const char *psaType = PUBSUB_ZMQ_ADMIN_TYPE;
            const char *serType = serEntry->serType;
            newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType,
                                                serType, NULL);
            celix_properties_set(newEndpoint, PUBSUB_ZMQ_URL_KEY, pubsub_zmqTopicSender_url(sender));
            //if available also set container name
            const char *cn = celix_bundleContext_getProperty(psa->ctx, "CELIX_CONTAINER_NAME", NULL);
            if (cn != NULL) {
                celix_properties_set(newEndpoint, "container_name", cn);
            }
            hashMap_put(psa->topicSenders.map, key, sender);
        } else {
            L_ERROR("[PSA ZMQ] Error creating a TopicSender");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_ZMQ] Cannot setup already existing TopicSender for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    if (sender != NULL && newEndpoint != NULL) {
        //TODO connect endpoints to sender, NOTE is this needed for a zmq topic sender?
    }

    if (newEndpoint != NULL && outPublisherEndpoint != NULL) {
        *outPublisherEndpoint = newEndpoint;
    }

    return status;
}

celix_status_t pubsub_zmqAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic) {
    pubsub_zmq_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    //1) Find and remove TopicSender from map
    //2) destroy topic sender

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicSenders.map, key);
    if (entry != NULL) {
        char *mapKey = hashMapEntry_getKey(entry);
        pubsub_zmq_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        //TODO disconnect endpoints to sender. note is this needed for a zmq topic sender?
        pubsub_zmqTopicSender_destroy(sender);
    } else {
        L_ERROR("[PSA ZMQ] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_zmqAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **outSubscriberEndpoint) {
    pubsub_zmq_admin_t *psa = handle;

    celix_properties_t *newEndpoint = NULL;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    pubsub_zmq_topic_receiver_t *receiver = hashMap_get(psa->topicReceivers.map, key);
    if (receiver == NULL) {
        psa_zmq_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serializerSvcId);
        if (serEntry != NULL) {
            receiver = pubsub_zmqTopicReceiver_create(psa->ctx, psa->log, scope, topic, serializerSvcId, serEntry->svc);
        } else {
            L_ERROR("[PSA_ZMQ] Cannot find serializer for TopicSender %s/%s", scope, topic);
        }
        if (receiver != NULL) {
            const char *psaType = PUBSUB_ZMQ_ADMIN_TYPE;
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
            L_ERROR("[PSA ZMQ] Error creating a TopicReceiver.");
            free(key);
        }
    } else {
        free(key);
        L_ERROR("[PSA_ZMQ] Cannot setup already existing TopicReceiver for scope/topic %s/%s!", scope, topic);
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
                pubsub_zmqAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
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

celix_status_t pubsub_zmqAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic) {
    pubsub_zmq_admin_t *psa = handle;

    char *key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    hash_map_entry_t *entry = hashMap_getEntry(psa->topicReceivers.map, key);
    free(key);
    if (entry != NULL) {
        char *receiverKey = hashMapEntry_getKey(entry);
        pubsub_zmq_topic_receiver_t *receiver = hashMapEntry_getValue(entry);
        hashMap_remove(psa->topicReceivers.map, receiverKey);

        free(receiverKey);
        pubsub_zmqTopicReceiver_destroy(receiver);
    }
    celixThreadMutex_lock(&psa->topicReceivers.mutex);

    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

static celix_status_t pubsub_zmqAdmin_connectEndpointToReceiver(pubsub_zmq_admin_t* psa, pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_zmqTopicReceiver_scope(receiver);
    const char *topic = pubsub_zmqTopicReceiver_topic(receiver);

    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *url = celix_properties_get(endpoint, PUBSUB_ZMQ_URL_KEY, NULL);

    if (url == NULL) {
        const char *admin = celix_properties_get(endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);
        L_WARN("[PSA ZMQ] Error got endpoint without a zmq url (admin: %s, type: %s)", admin , type);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0) {
            pubsub_zmqTopicReceiver_connectTo(receiver, url);
        }
    }

    return status;
}

celix_status_t pubsub_zmqAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_zmq_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_zmq_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            pubsub_zmqAdmin_connectEndpointToReceiver(psa, receiver, endpoint);
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


static celix_status_t pubsub_zmqAdmin_disconnectEndpointFromReceiver(pubsub_zmq_admin_t* psa, pubsub_zmq_topic_receiver_t *receiver, const celix_properties_t *endpoint) {
    //note can be called with discoveredEndpoint.mutex lock
    celix_status_t status = CELIX_SUCCESS;

    const char *scope = pubsub_zmqTopicReceiver_scope(receiver);
    const char *topic = pubsub_zmqTopicReceiver_topic(receiver);

    const char *eScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
    const char *eTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    const char *url = celix_properties_get(endpoint, PUBSUB_ZMQ_URL_KEY, NULL);

    if (url == NULL) {
        L_WARN("[PSA ZMQ] Error got endpoint without zmq url");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        if (eScope != NULL && eTopic != NULL &&
            strncmp(eScope, scope, 1024 * 1024) == 0 &&
            strncmp(eTopic, topic, 1024 * 1024) == 0) {
            pubsub_zmqTopicReceiver_disconnectFrom(receiver, url);
        }
    }

    return status;
}

celix_status_t pubsub_zmqAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_zmq_admin_t *psa = handle;

    const char *type = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TYPE, NULL);

    if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
        celixThreadMutex_lock(&psa->topicReceivers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(psa->topicReceivers.map);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_zmq_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
            pubsub_zmqAdmin_disconnectEndpointFromReceiver(psa, receiver, endpoint);
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

celix_status_t pubsub_zmqAdmin_executeCommand(void *handle, char *commandLine __attribute__((unused)), FILE *out, FILE *errStream __attribute__((unused))) {
    pubsub_zmq_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    fprintf(out, "\n");
    fprintf(out, "Topic Senders:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_zmq_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_zmqTopicSender_serializerSvcId(sender);
        psa_zmq_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_zmqTopicSender_scope(sender);
        const char *topic = pubsub_zmqTopicSender_topic(sender);
        const char *url = pubsub_zmqTopicSender_url(sender);
        fprintf(out, "|- Topic Sender %s/%s\n", scope, topic);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- url             = %s\n", url);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    celixThreadMutex_unlock(&psa->serializers.mutex);

    fprintf(out, "\n");
    fprintf(out, "\nTopic Receivers:\n");
    celixThreadMutex_lock(&psa->serializers.mutex);
    celixThreadMutex_lock(&psa->topicReceivers.mutex);
    iter = hashMapIterator_construct(psa->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_zmq_topic_receiver_t *receiver = hashMapIterator_nextValue(&iter);
        long serSvcId = pubsub_zmqTopicReceiver_serializerSvcId(receiver);
        psa_zmq_serializer_entry_t *serEntry = hashMap_get(psa->serializers.map, (void*)serSvcId);
        const char *serType = serEntry == NULL ? "!Error!" : serEntry->serType;
        const char *scope = pubsub_zmqTopicReceiver_scope(receiver);
        const char *topic = pubsub_zmqTopicReceiver_topic(receiver);

        celix_array_list_t *connected = celix_arrayList_create();
        celix_array_list_t *unconnected = celix_arrayList_create();
        pubsub_zmqTopicReceiver_listConnections(receiver, connected, unconnected);

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

#ifndef ANDROID
static celix_status_t zmq_getIpAddress(const char* interface, char** ip) {
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