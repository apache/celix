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
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <pubsub_utils.h>
#include "pubsub_udpmc_topic_sender.h"
#include "pubsub_psa_udpmc_constants.h"
#include "large_udp.h"
#include "pubsub_udpmc_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS     2

//TODO make configurable
#define UDP_BASE_PORT                   49152
#define UDP_MAX_PORT                    65000


struct pubsub_udpmc_topic_sender {
    celix_bundle_context_t *ctx;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;
    char *socketAddress;
    long socketPort;
    bool staticallyConfigured;

    int sendSocket;
    struct sockaddr_in destAddr;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_udpmc_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_udpmc_bounded_service_entry {
    pubsub_udpmc_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes;
    hash_map_t *msgTypeIds;
    int getCount;
    largeUdp_t *largeUdpHandle;
} psa_udpmc_bounded_service_entry_t;

typedef struct pubsub_msg {
    pubsub_udp_msg_header_t *header;
    unsigned int payloadSize;
    char *payload;
} pubsub_udp_msg_t;

static int psa_udpmc_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
static void* psa_udpmc_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_udpmc_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static int psa_udpmc_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata);
static bool psa_udpmc_sendMsg(psa_udpmc_bounded_service_entry_t *entry, pubsub_udp_msg_t* msg);
static unsigned int rand_range(unsigned int min, unsigned int max);

pubsub_udpmc_topic_sender_t* pubsub_udpmcTopicSender_create(
        celix_bundle_context_t *ctx,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *serializer,
        int sendSocket,
        const char *bindIP,
        const celix_properties_t *topicProperties) {
    pubsub_udpmc_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = serializer;
    sender->scope = scope == NULL ? NULL : strndup(scope, 1024 * 1024);
    sender->topic = strndup(topic, 1024 * 1024);

    celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
    sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);

    unsigned int port = rand_range(UDP_BASE_PORT, UDP_MAX_PORT);

    const char *portEnvVar = pubsub_getEnvironmentVariableWithScopeTopic(ctx, PUBSUB_UDPMC_STATIC_BIND_PORT_URL_FOR, topic, scope);
    long configuredPort = -1;
    if(portEnvVar != NULL) {
        char *endptr = NULL;
        long p = strtol(portEnvVar, &endptr, 10);
        if(endptr != portEnvVar && errno != ERANGE && p >= 0 && p <= 65535) {
            configuredPort = p;
        }
    }

    if(configuredPort == -1) {
        configuredPort = celix_properties_getAsLong(topicProperties, PUBSUB_UDPMC_STATIC_BIND_PORT, -1L);
    }

    if (configuredPort > 0) {
        port = (unsigned int)configuredPort;
        sender->staticallyConfigured = true;
    } else {
        sender->staticallyConfigured = false;
    }

    //setting up socket for UDPMC TopicSender
    {
        sender->sendSocket = sendSocket;
        sender->destAddr.sin_family = AF_INET;
        sender->destAddr.sin_addr.s_addr = inet_addr(bindIP);
        sender->destAddr.sin_port = htons((uint16_t)port);

        sender->socketAddress = strndup(bindIP, 1024);
        sender->socketPort = port;
    }

    //register publisher services using a service factory
    {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_udpmc_getPublisherService;
        sender->publisher.factory.ungetService = psa_udpmc_ungetPublisherService;


        //register publisher factory service
        {
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
    }

    return sender;
}

void pubsub_udpmcTopicSender_destroy(pubsub_udpmc_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        celixThreadMutex_destroy(&sender->boundedServices.mutex);

        //TODO loop and cleanup?
        hashMap_destroy(sender->boundedServices.map, false, true);

        if (sender->scope != NULL) {
            free(sender->scope);
        }
        free(sender->topic);
        free(sender->socketAddress);
        free(sender);
    }
}

const char* pubsub_udpmcTopicSender_psaType(pubsub_udpmc_topic_sender_t *sender __attribute__((unused))) {
    return PSA_UDPMC_PUBSUB_ADMIN_TYPE;
}


const char* pubsub_udpmcTopicSender_scope(pubsub_udpmc_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_udpmcTopicSender_topic(pubsub_udpmc_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_udpmcTopicSender_socketAddress(pubsub_udpmc_topic_sender_t *sender) {
    return sender->socketAddress;
}

long pubsub_udpmcTopicSender_socketPort(pubsub_udpmc_topic_sender_t *sender) {
    return sender->socketPort;
}

void pubsub_udpmcTopicSender_connectTo(pubsub_udpmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO subscriber count -> topic info
}

void pubsub_udpmcTopicSender_disconnectFrom(pubsub_udpmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

static int psa_udpmc_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId) {
    psa_udpmc_bounded_service_entry_t *entry = (psa_udpmc_bounded_service_entry_t *) handle;
    *msgTypeId = (unsigned int)(uintptr_t) hashMap_get(entry->msgTypeIds, msgType);
    return 0;
}

static void* psa_udpmc_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_udpmc_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    pubsub_publisher_t *svc = NULL;

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_udpmc_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;
        entry->largeUdpHandle = largeUdp_create(1);
        entry->msgTypeIds = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                pubsub_msg_serializer_t *msgSer  = hashMapIterator_nextValue(&iter);
                hashMap_put(entry->msgTypeIds, strndup(msgSer->msgName, 1024), (void *)(uintptr_t) msgSer->msgId);
            }

            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_udpmc_localMsgTypeIdForMsgType;
            entry->service.send = psa_udpmc_topicPublicationSend;
            hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
            svc = &entry->service;
        } else {
            fprintf(stderr, "Error creating publisher service, serializer not available / cannot get msg serializer map\n");
            free(entry);
        }
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return svc;
}

static void psa_udpmc_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_udpmc_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_udpmc_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(sender->boundedServices.map, (void*)bndId);

        int rc = sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            fprintf(stderr, "Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }

        hashMap_destroy(entry->msgTypeIds, true, false);
        largeUdp_destroy(entry->largeUdpHandle);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static int psa_udpmc_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    psa_udpmc_bounded_service_entry_t *entry = handle;
    int status = 0;

    pubsub_msg_serializer_t* msgSer = NULL;
    if (entry->msgTypes != NULL) {
        msgSer = hashMap_get(entry->msgTypes, (void*)(intptr_t)(msgTypeId));
    }

    if (msgSer != NULL) {
        size_t serializedOutputLen = 0;
        struct iovec* serializedOutput = NULL;
        if (msgSer->serialize(msgSer->handle, inMsg, &serializedOutput, &serializedOutputLen) == CELIX_SUCCESS) {
            pubsub_udp_msg_header_t *msg_hdr = calloc(1, sizeof(*msg_hdr));
            msg_hdr->type = msgTypeId;

            if (msgSer->msgVersion != NULL) {
                int major = 0, minor = 0;
                version_getMajor(msgSer->msgVersion, &major);
                version_getMinor(msgSer->msgVersion, &minor);
                msg_hdr->major = (unsigned char) major;
                msg_hdr->minor = (unsigned char) minor;
            }

            pubsub_udp_msg_t *msg = calloc(1, sizeof(*msg));
            msg->header = msg_hdr;
            msg->payload = (char *) serializedOutput->iov_base;
            msg->payloadSize = (unsigned int)  serializedOutput->iov_len;

            if (psa_udpmc_sendMsg(entry, msg) == false) {
                status = -1;
            }
            free(msg);
            free(msg_hdr);
            msgSer->freeSerializeMsg(msgSer->handle, serializedOutput, serializedOutputLen);
        } else {
            printf("[PSA_UDPMC/TopicSender] Serialization of msg type id %d failed\n", msgTypeId);
            status = -1;
        }

    } else {
        printf("[PSA_UDPMC/TopicSender] No msg serializer available for msg type id %d\n", msgTypeId);
        status = -1;
    }
    celix_properties_destroy(metadata);
    return status;
}

static void delay_first_send_for_late_joiners() {

    static bool firstSend = true;

    if (firstSend) {
        printf("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static bool psa_udpmc_sendMsg(psa_udpmc_bounded_service_entry_t *entry, pubsub_udp_msg_t* msg) {
    const int iovec_len = 3; // header + size + payload
    bool ret = true;

    struct iovec msg_iovec[iovec_len];
    msg_iovec[0].iov_base = msg->header;
    msg_iovec[0].iov_len = sizeof(*msg->header);
    msg_iovec[1].iov_base = &msg->payloadSize;
    msg_iovec[1].iov_len = sizeof(msg->payloadSize);
    msg_iovec[2].iov_base = msg->payload;
    msg_iovec[2].iov_len = msg->payloadSize;

    delay_first_send_for_late_joiners();

    if (largeUdp_sendmsg(entry->largeUdpHandle, entry->parent->sendSocket, msg_iovec, iovec_len, 0, &entry->parent->destAddr, sizeof(entry->parent->destAddr)) == -1) {
        perror("send_pubsub_msg:sendSocket");
        ret = false;
    }

    return ret;
}

static unsigned int rand_range(unsigned int min, unsigned int max) {
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}

long pubsub_udpmcTopicSender_serializerSvcId(pubsub_udpmc_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

bool pubsub_udpmcTopicSender_isStatic(pubsub_udpmc_topic_sender_t *sender) {
    return sender->staticallyConfigured;
}
