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

#include <pubsub_serializer.h>
#include <stdlib.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <pubsub_common.h>
#include <zconf.h>
#include <arpa/inet.h>
#include "pubsub_udpmc_topic_sender.h"
#include "pubsub_psa_udpmc_constants.h"
#include "large_udp.h"
#include "pubsub_udpmc_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS             2

//TODO make configurable
#define UDP_BASE_PORT	                        49152
#define UDP_MAX_PORT	                        65000


struct pubsub_updmc_topic_sender {
    celix_bundle_context_t *ctx;
    char *scope;
    char *topic;
    char *socketAddress;
    long socketPort;

    int sendSocket;
    struct sockaddr_in destAddr;

    long serTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        pubsub_serializer_service_t *svc;
        const celix_properties_t *props;
    } serializer;

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
    pubsub_updmc_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes;
    int getCount;
    largeUdp_pt largeUdpHandle;
} psa_udpmc_bounded_service_entry_t;

typedef struct pubsub_msg{
    pubsub_msg_header_t *header;
    char* payload;
    unsigned int payloadSize;
} pubsub_msg_t;

static void pubsub_udpmcTopicSender_setSerializer(void *handle, void *svc, const celix_properties_t *props);
static void* psa_udpmc_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_udpmc_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static int psa_udpmc_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg);
static bool psa_udpmc_sendMsg(psa_udpmc_bounded_service_entry_t *entry, pubsub_msg_t* msg, bool last, pubsub_release_callback_t *releaseCallback);
static unsigned int rand_range(unsigned int min, unsigned int max);

pubsub_updmc_topic_sender_t* pubsub_udpmcTopicSender_create(
        celix_bundle_context_t *ctx,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        int sendSocket,
        const char *bindIP) {
    pubsub_updmc_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->scope = strndup(scope, 1024 * 1024);
    sender->topic = strndup(topic, 1024 * 1024);

    celixThreadMutex_create(&sender->serializer.mutex, NULL);
    celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
    sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);

    //setting up socket for UDPMC TopicSender
    {
        unsigned int port = rand_range(UDP_BASE_PORT, UDP_MAX_PORT);
        sender->sendSocket = sendSocket;
        sender->destAddr.sin_family = AF_INET;
        sender->destAddr.sin_addr.s_addr = inet_addr(bindIP);
        sender->destAddr.sin_port = htons((uint16_t)port);

        sender->socketAddress = strndup(bindIP, 1024);
        sender->socketPort = port;
    }

    //track serializer svc based on the provided serializerSvcId
    {
        char filter[64];
        snprintf(filter, 64, "(service.id=%li)", serializerSvcId);

        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
        opts.filter.filter = filter;
        opts.filter.ignoreServiceLanguage = true;
        opts.callbackHandle = sender;
        opts.setWithProperties = pubsub_udpmcTopicSender_setSerializer;
        sender->serTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register publisher services using a service factory
    {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_udpmc_getPublisherService;
        sender->publisher.factory.ungetService = psa_udpmc_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);


        sender->publisher.svcId = celix_bundleContext_registerServiceFactory(ctx, &sender->publisher.factory, PUBSUB_PUBLISHER_SERVICE_NAME, props);
    }

    return sender;
}

void pubsub_udpmcTopicSender_destroy(pubsub_updmc_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_stopTracker(sender->ctx, sender->serTrackerId);
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        celixThreadMutex_destroy(&sender->serializer.mutex);
        celixThreadMutex_destroy(&sender->boundedServices.mutex);

        //TODO loop and cleanup?
        hashMap_destroy(sender->boundedServices.map, false, true);

        free(sender->scope);
        free(sender->topic);
        free(sender->socketAddress);
        free(sender);
    }
}

const char* pubsub_udpmcTopicSender_psaType(pubsub_updmc_topic_sender_t *sender __attribute__((unused))) {
    return PSA_UDPMC_PUBSUB_ADMIN_TYPE;
}

const char* pubsub_udpmcTopicSender_serializerType(pubsub_updmc_topic_sender_t *sender) {
    const char *result = NULL;
    celixThreadMutex_lock(&sender->serializer.mutex);
    if (sender->serializer.props != NULL) {
        result = celix_properties_get(sender->serializer.props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    }
    celixThreadMutex_unlock(&sender->serializer.mutex);
    return result;
}

const char* pubsub_udpmcTopicSender_scope(pubsub_updmc_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_udpmcTopicSender_topic(pubsub_updmc_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_udpmcTopicSender_socketAddress(pubsub_updmc_topic_sender_t *sender) {
    return sender->socketAddress;
}

long pubsub_udpmcTopicSender_socketPort(pubsub_updmc_topic_sender_t *sender) {
    return sender->socketPort;
}

void pubsub_udpmcTopicSender_connectTo(pubsub_updmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO subscriber count -> topic info
}

void pubsub_udpmcTopicSender_disconnectFrom(pubsub_updmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

static void pubsub_udpmcTopicSender_setSerializer(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_updmc_topic_sender_t *sender = handle;
    pubsub_serializer_service_t *ser = svc;

    if (ser == NULL) {
        //TODO -> no serializer -> remove all publishers
    }

    celixThreadMutex_lock(&sender->serializer.mutex);
    sender->serializer.svc = ser;
    sender->serializer.props = props;
    celixThreadMutex_unlock(&sender->serializer.mutex);
}

static void* psa_udpmc_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_updmc_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

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

        celixThreadMutex_lock(&sender->serializer.mutex);
        celix_status_t rc = CELIX_SUCCESS;
        if (sender->serializer.svc != NULL) {
            rc = sender->serializer.svc->createSerializerMap(sender->serializer.svc->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        }
        if (sender->serializer.svc == NULL || rc != CELIX_SUCCESS) {
            //TODO destroy and return NULL?
            fprintf(stderr, "Error creating publisher service, serializer not available / cannot get msg serializer map\n");
        }
        celixThreadMutex_unlock(&sender->serializer.mutex);


        entry->service.handle = entry;
        entry->service.localMsgTypeIdForMsgType = psa_udpmc_localMsgTypeIdForMsgType;
        entry->service.send = psa_udpmc_topicPublicationSend;
        entry->service.sendMultipart = NULL; //note multipart not supported by MCUDP

        hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return &entry->service;
}

static void psa_udpmc_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_updmc_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_udpmc_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(sender->boundedServices.map, (void*)bndId);


        celixThreadMutex_lock(&sender->serializer.mutex);
        celix_status_t rc = CELIX_SUCCESS;
        if (sender->serializer.svc != NULL) {
            rc = sender->serializer.svc->destroySerializerMap(sender->serializer.svc->handle, entry->msgTypes);
        }
        if (sender->serializer.svc == NULL || rc != CELIX_SUCCESS) {
            fprintf(stderr, "Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }
        celixThreadMutex_unlock(&sender->serializer.mutex);


        largeUdp_destroy(entry->largeUdpHandle);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static int psa_udpmc_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg) {
    psa_udpmc_bounded_service_entry_t *entry = handle;
    int status = 0;

    pubsub_msg_serializer_t* msgSer = NULL;
    if (entry->msgTypes != NULL) {
        msgSer = hashMap_get(entry->msgTypes, (void*)(intptr_t)(msgTypeId));
    }

    if (msgSer != NULL) {
        int major=0, minor=0;

        pubsub_msg_header_t *msg_hdr = calloc(1,sizeof(struct pubsub_msg_header));
        strncpy(msg_hdr->topic,entry->parent->topic,MAX_TOPIC_LEN-1);
        msg_hdr->type = msgTypeId;


        if (msgSer->msgVersion != NULL){
            version_getMajor(msgSer->msgVersion, &major);
            version_getMinor(msgSer->msgVersion, &minor);
            msg_hdr->major = major;
            msg_hdr->minor = minor;
        }

        void* serializedOutput = NULL;
        size_t serializedOutputLen = 0;
        msgSer->serialize(msgSer,inMsg,&serializedOutput, &serializedOutputLen);

        pubsub_msg_t *msg = calloc(1,sizeof(pubsub_msg_t));
        msg->header = msg_hdr;
        msg->payload = (char*)serializedOutput;
        msg->payloadSize = serializedOutputLen;


        if(psa_udpmc_sendMsg(entry, msg,true, NULL) == false) {
            status = -1;
        }
        free(msg_hdr);
        free(msg);
        free(serializedOutput);


    } else {
        printf("[PSA_UDPMC/TopicSender] No msg serializer available for msg type id %d\n", msgTypeId);
        status=-1;
    }
    return status;
}

static void delay_first_send_for_late_joiners(){

    static bool firstSend = true;

    if(firstSend){
        printf("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static bool psa_udpmc_sendMsg(psa_udpmc_bounded_service_entry_t *entry, pubsub_msg_t* msg, bool last, pubsub_release_callback_t *releaseCallback) {
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

    if(largeUdp_sendmsg(entry->largeUdpHandle, entry->parent->sendSocket, msg_iovec, iovec_len, 0, &entry->parent->destAddr, sizeof(entry->parent->destAddr)) == -1) {
        perror("send_pubsub_msg:sendSocket");
        ret = false;
    }

    if(releaseCallback) {
        releaseCallback->release(msg->payload, entry);
    }
    return ret;
}

static unsigned int rand_range(unsigned int min, unsigned int max){
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
