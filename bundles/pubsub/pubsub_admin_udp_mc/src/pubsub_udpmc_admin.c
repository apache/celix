
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <pubsub_endpoint.h>

#include "pubsub_utils.h"
#include "pubsub_udpmc_admin.h"
#include "pubsub_psa_udpmc_constants.h"
#include "pubsub_udpmc_topic_sender.h"

#define PUBSUB_UDPMC_MC_IP_DEFAULT                     "224.100.1.1"

#define LOG_DEBUG(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_INFO, __VA_ARGS__);
#define LOG_WARN(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, __VA_ARGS__);
#define LOG_ERROR(...) \
    logHelper_log(psa->log, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)


static celix_status_t udpmc_getIpAddress(const char* interface, char** ip);

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
        LOG_WARN("[PSA_UDPMC] Could not retrieve IP address for interface %s", interface);
    } else if (psa->verbose) {
        LOG_INFO("[PSA_UDPMC] Using IP address %s", if_ip);
    }

    if(if_ip && sscanf(if_ip, "%i.%i.%i.%i", &b0, &b1, &b2, &b3) != 4) {
        logHelper_log(psa->log, OSGI_LOGSERVICE_WARNING, "[PSA_UDPMC] Could not parse IP address %s", if_ip);
        b2 = 1;
        b3 = 1;
    }

    asprintf(&mc_ip, "%s.%d.%d",mc_prefix, b2, b3);

    sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(sendSocket == -1) {
        LOG_ERROR("[PSA_UDPMC] Error creating socket: %s", strerror(errno));
    } else {
        char loop = 1;
        int rc = setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if(rc != 0) {
            LOG_ERROR("[PSA_UDPMC] Error setsockopt(IP_MULTICAST_LOOP): %s", strerror(errno));
        }
        if (rc == 0) {
            struct in_addr multicast_interface;
            inet_aton(if_ip, &multicast_interface);
            rc = setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_IF, &multicast_interface, sizeof(multicast_interface));
            if (rc != 0) {
                LOG_ERROR("[PSA_UDPMC] Error setsockopt(IP_MULTICAST_IF): %s", strerror(errno));
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
        LOG_INFO("[PSA_UDPMC] Using %s as interface for multicast communication", psa->ifIpAddress);
    }


    if (mc_ip != NULL) {
        psa->mcIpAddress = mc_ip;
    } else {
        psa->mcIpAddress = strdup(PUBSUB_UDPMC_MC_IP_DEFAULT);
    }
    if (psa->verbose) {
        LOG_INFO("[PSA_UDPMC] Using %s for service annunciation", psa->mcIpAddress);
    }

    psa->defaultScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_DEFAULT_SCORE_KEY, PSA_UDPMC_DEFAULT_SCORE);
    psa->qosSampleScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_QOS_SAMPLE_SCORE_KEY, PSA_UDPMC_DEFAULT_QOS_SAMPLE_SCORE);
    psa->qosControlScore = celix_bundleContext_getPropertyAsDouble(ctx, PSA_UDPMC_QOS_CONTROL_SCORE_KEY, PSA_UDPMC_DEFAULT_QOS_CONTROL_SCORE);

    celixThreadMutex_create(&psa->topicSenders.mutex, NULL);
    psa->topicSenders.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->topicReceivers.mutex, NULL);
    psa->topicReceivers.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celixThreadMutex_create(&psa->connectedEndpoints.mutex, NULL);
    psa->connectedEndpoints.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    return psa;
}

void pubsub_udpmcAdmin_destroy(pubsub_udpmc_admin_t *psa) {
    if (psa == NULL) {
        return;
    }

    //note assuming al psa register services and service tracker are removed.

    celixThreadMutex_destroy(&psa->topicSenders.mutex);
    hashMap_destroy(psa->topicSenders.map, true, false);

    celixThreadMutex_destroy(&psa->topicReceivers.mutex);
    hashMap_destroy(psa->topicReceivers.map, true, false);

    celixThreadMutex_destroy(&psa->connectedEndpoints.mutex);
    hashMap_destroy(psa->connectedEndpoints.map, true, false);

    free(psa->mcIpAddress);
    free(psa->ifIpAddress);
    free(psa);
}

celix_status_t pubsub_udpmcAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, double *outScore, long *outSerializerSvcId) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchPublisher");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchPublisher(psa->ctx, svcRequesterBndId, svcFilter->filterStr, PUBSUB_UDPMC_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    *outScore = score;

    return status;
}

celix_status_t pubsub_udpmcAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, double *outScore, long *outSerializerSvcId) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchSubscriber");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchSubscriber(psa->ctx, svcProviderBndId, svcProperties, PUBSUB_UDPMC_ADMIN_TYPE,
            psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, outSerializerSvcId);
    if (outScore != NULL) {
        *outScore = score;
    }
    return status;
}

celix_status_t pubsub_udpmcAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, double *outScore) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_DEBUG("[PSA_UDPMC] pubsub_udpmcAdmin_matchEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    double score = pubsub_utils_matchEndpoint(psa->ctx, endpoint, PUBSUB_UDPMC_ADMIN_TYPE,
                                                psa->qosSampleScore, psa->qosControlScore, psa->defaultScore, NULL);
    if (outScore != NULL) {
        *outScore = score;
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
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    pubsub_updmc_topic_sender_t *sender = hashMap_get(psa->topicSenders.map, key);
    if (sender == NULL) {
        sender = pubsub_udpmcTopicSender_create(psa->ctx, scope, topic, serializerSvcId);
        const char *psaType = pubsub_udpmcTopicSender_psaType(sender);
        const char *serType = pubsub_udpmcTopicSender_serializerType(sender);
        newEndpoint = pubsubEndpoint_create(psa->fwUUID, scope, topic,
                PUBSUB_PUBLISHER_ENDPOINT_TYPE, psaType, serType, NULL);
        bool valid = pubsubEndpoint_isValid(newEndpoint, true, true);
        if (!valid) {
            LOG_ERROR("[PSA UDPMC] Error creating a valid TopicSender. Endpoints are not valid");
            celix_properties_destroy(newEndpoint);
            pubsub_udpmcTopicSender_destroy(sender);
            free(key);
        } else {
            hashMap_put(psa->topicSenders.map, key, sender);
            //TODO connect endpoints to sender
        }
    } else {
        free(key);
        LOG_ERROR("[PSA_UDPMC] Cannot setup already existing TopicSender for scope/topic %s/%s!", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);


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
        pubsub_updmc_topic_sender_t *sender = hashMap_remove(psa->topicSenders.map, key);
        free(mapKey);
        //TODO disconnect endpoints to sender
        pubsub_udpmcTopicSender_destroy(sender);
    } else {
        LOG_ERROR("[PSA UDPMC] Cannot teardown TopicSender with scope/topic %s/%s. Does not exists", scope, topic);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    free(key);

    return status;
}

celix_status_t pubsub_udpmcAdmin_setupTopicReciever(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **subscriberEndpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_INFO("[PSA_UDPMC] pubsub_udpmcAdmin_setupTopicReciever. scope/topic: %s/%s", scope, topic);
    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_udpmcAdmin_teardownTopicReciever(void *handle, const char *scope, const char *topic) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_INFO("[PSA_UDPMC] pubsub_udpmcAdmin_teardownTopicReciever. scope/topic: %s/%s", scope, topic);
    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_udpmcAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_INFO("[PSA_UDPMC] pubsub_udpmcAdmin_addEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_udpmcAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_udpmc_admin_t *psa = handle;
    LOG_INFO("[PSA_UDPMC] pubsub_udpmcAdmin_removeEndpoint");
    celix_status_t  status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsub_udpmcAdmin_executeCommand(void *handle, char *commandLine __attribute__((unused)), FILE *out, FILE *errStream __attribute__((unused))) {
    pubsub_udpmc_admin_t *psa = handle;
    celix_status_t  status = CELIX_SUCCESS;

    fprintf(out, "\nTopic Senders:\n");
    celixThreadMutex_lock(&psa->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(psa->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_updmc_topic_sender_t *sender = hashMapIterator_nextValue(&iter);
        const char *psaType = pubsub_udpmcTopicSender_psaType(sender);
        const char *serType = pubsub_udpmcTopicSender_serializerType(sender);
        const char *scope = pubsub_udpmcTopicSender_scope(sender);
        const char *topic = pubsub_udpmcTopicSender_topic(sender);
        const char *url = pubsub_udpmcTopicSender_socketAddress(sender);
        fprintf(out, "|- Topic Sender %s/%s\n", scope, topic);
        fprintf(out, "   |- psa type        = %s\n", psaType);
        fprintf(out, "   |- serializer type = %s\n", serType);
        fprintf(out, "   |- url             = %s\n", url);
    }
    celixThreadMutex_unlock(&psa->topicSenders.mutex);
    fprintf(out, "\n");

    //TODO topic receivers

    return status;
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