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

#include <stdlib.h>
#include <utils.h>
#include <arpa/inet.h>
#include <zconf.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>


#include <pubsub_serializer.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <pubsub_common.h>
#include <log_helper.h>
#include "pubsub_nanomsg_topic_sender.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS                 2
#define NANOMSG_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_nanomsg_topic_sender {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;

    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *url;

    struct {
        celix_thread_mutex_t mutex;
        int socket;
    } nanomsg;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_nanomsg_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_nanomsg_bounded_service_entry {
    pubsub_nanomsg_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes;
    int getCount;
} psa_nanomsg_bounded_service_entry_t;

static void* psa_nanomsg_getPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                             const celix_properties_t *svcProperties);
static void psa_nanomsg_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                              const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub_nanomsg_topic_sender_t *sender);

static int psa_nanomsg_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *msg);

pubsub_nanomsg_topic_sender_t* pubsub_nanoMsgTopicSender_create(celix_bundle_context_t *ctx, log_helper_t *logHelper,
                                                                const char *scope, const char *topic,
                                                                long serializerSvcId, pubsub_serializer_service_t *ser,
                                                                const char *bindIP, unsigned int basePort,
                                                                unsigned int maxPort) {
    pubsub_nanomsg_topic_sender_t *sender = static_cast<pubsub_nanomsg_topic_sender_t*>(calloc(1, sizeof(*sender)));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;
    psa_nanomsg_setScopeAndTopicFilter(scope, topic, sender->scopeAndTopicFilter);

    //setting up nanomsg socket for nanomsg TopicSender
    {

        int socket = nn_socket(AF_SP, NN_BUS);
        if (socket == -1) {
            perror("Error for nanomsg_socket");
        }

        int rv = -1, retry=0;
        while(rv == -1 && retry < NANOMSG_BIND_MAX_RETRY ) {
            /* Randomized part due to same bundle publishing on different topics */
            unsigned int port = rand_range(basePort,maxPort);
            size_t len = (size_t)snprintf(NULL, 0, "tcp://%s:%u", bindIP, port) + 1;
            char *url = static_cast<char*>(calloc(len, sizeof(char*)));
            snprintf(url, len, "tcp://%s:%u", bindIP, port);

            len = (size_t)snprintf(NULL, 0, "tcp://0.0.0.0:%u", port) + 1;
            char *bindUrl = static_cast<char*>(calloc(len, sizeof(char)));
            snprintf(bindUrl, len, "tcp://0.0.0.0:%u", port);
            rv = nn_bind (socket, bindUrl);
            if (rv == -1) {
                perror("Error for nn_bind");
                free(url);
            } else {
                sender->url = url;
                sender->nanomsg.socket = socket;
            }
            retry++;
            free(bindUrl);
        }
    }

    if (sender->url != NULL) {
        sender->scope = strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        celixThreadMutex_create(&sender->nanomsg.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (sender->url != NULL) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_nanomsg_getPublisherService;
        sender->publisher.factory.ungetService = psa_nanomsg_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.factory = &sender->publisher.factory;
        opts.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_PUBLISHER_SERVICE_VERSION;
        opts.properties = props;

        sender->publisher.svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    if (sender->url == NULL) {
        free(sender);
        sender = NULL;
    }

    return sender;
}

void pubsub_nanoMsgTopicSender_destroy(pubsub_nanomsg_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        nn_close(sender->nanomsg.socket);

        celixThreadMutex_lock(&sender->boundedServices.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMapIterator_nextValue(&iter));
            if (entry != NULL) {
                sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
                free(entry);
            }
        }
        hashMap_destroy(sender->boundedServices.map, false, false);
        celixThreadMutex_unlock(&sender->boundedServices.mutex);

        celixThreadMutex_destroy(&sender->boundedServices.mutex);
        celixThreadMutex_destroy(&sender->nanomsg.mutex);

        free(sender->scope);
        free(sender->topic);
        free(sender->url);
        free(sender);
    }
}

long pubsub_nanoMsgTopicSender_serializerSvcId(pubsub_nanomsg_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

const char* pubsub_nanoMsgTopicSender_scope(pubsub_nanomsg_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_nanoMsgTopicSender_topic(pubsub_nanomsg_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_nanoMsgTopicSender_url(pubsub_nanomsg_topic_sender_t *sender) {
    return sender->url;
}

void pubsub_nanoMsgTopicSender_connectTo(pubsub_nanomsg_topic_sender_t *, const celix_properties_t *) {
    //TODO subscriber count -> topic info
}

void pubsub_nanoMsgTopicSender_disconnectFrom(pubsub_nanomsg_topic_sender_t *, const celix_properties_t *) {
    //TODO
}

static void* psa_nanomsg_getPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                             const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_nanomsg_topic_sender_t *sender = static_cast<pubsub_nanomsg_topic_sender_t*>(handle);
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMap_get(sender->boundedServices.map, (void*)bndId));
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(calloc(1, sizeof(*entry)));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_nanoMsg_localMsgTypeIdForMsgType;
            entry->service.send = psa_nanomsg_topicPublicationSend;
            entry->service.sendMultipart = NULL; //not supported TODO remove
            hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for NanoMsg TopicSender %s/%s", sender->scope, sender->topic);
        }



    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return &entry->service;
}

static void psa_nanomsg_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle,
                                              const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_nanomsg_topic_sender_t *sender = static_cast<pubsub_nanomsg_topic_sender_t*>(handle);
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMap_get(sender->boundedServices.map, (void*)bndId));
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

        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static int psa_nanomsg_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *inMsg) {
    int status = CELIX_SUCCESS;
    psa_nanomsg_bounded_service_entry_t *bound = static_cast<psa_nanomsg_bounded_service_entry_t*>(handle);
    pubsub_nanomsg_topic_sender_t *sender = bound->parent;

    pubsub_msg_serializer_t* msgSer = static_cast<pubsub_msg_serializer_t*>(hashMap_get(bound->msgTypes, (void*)(uintptr_t)msgTypeId));

    if (msgSer != NULL) {
        delay_first_send_for_late_joiners(sender);

        int major = 0, minor = 0;

        pubsub_nanmosg_msg_header_t msg_hdr;// = calloc(1, sizeof(*msg_hdr));
        msg_hdr.type = msgTypeId;

        if (msgSer->msgVersion != NULL) {
            version_getMajor(msgSer->msgVersion, &major);
            version_getMinor(msgSer->msgVersion, &minor);
            msg_hdr.major = (unsigned char) major;
            msg_hdr.minor = (unsigned char) minor;
        }

        void *serializedOutput = NULL;
        size_t serializedOutputLen = 0;
        status = msgSer->serialize(msgSer, inMsg, &serializedOutput, &serializedOutputLen);
        if (status == CELIX_SUCCESS) {
            nn_iovec data[2];

            nn_msghdr msg;
            msg.msg_iov = data;
            msg.msg_iovlen = 2;
            msg.msg_iov[0].iov_base = static_cast<void*>(&msg_hdr);
            msg.msg_iov[0].iov_len = sizeof(msg_hdr);
            msg.msg_iov[1].iov_base = serializedOutput;
            msg.msg_iov[1].iov_len = serializedOutputLen;
            msg.msg_control = nullptr;
            msg.msg_controllen = 0;
            //zmsg_t *msg = zmsg_new();
            //TODO revert to use zmq_msg_init_data (or something like that) for zero copy for the payload
            //TODO remove socket mutex .. not needed (initialized during creation)
            //zmsg_addstr(msg, sender->scopeAndTopicFilter);
            //zmsg_addmem(msg, &msg_hdr, sizeof(msg_hdr));
            //zmsg_addmem(msg, serializedOutput, );
            errno = 0;
            int rc = nn_sendmsg(sender->nanomsg.socket, &msg, 0 );
            free(serializedOutput);
            if (rc < 0) {
                L_WARN("[PSA_ZMQ_TS] Error sending zmsg, rc is %i. %s", rc, strerror(errno));
            } else {
                L_INFO("[PSA_ZMQ_TS] Send message with size %d\n",  rc);
                L_INFO("[PSA_ZMQ_TS] Send message ID %d, major %d, minor %d\n",  msg_hdr.type, (int)msg_hdr.major, (int)msg_hdr.minor);
            }
        } else {
            L_WARN("[PSA_ZMQ_TS] Error serialize message of type %s for scope/topic %s/%s", msgSer->msgName, sender->scope, sender->topic);
        }
    } else {
        status = CELIX_SERVICE_EXCEPTION;
        L_WARN("[PSA_ZMQ_TS] Error cannot serialize message with msg type id %i for scope/topic %s/%s", msgTypeId, sender->scope, sender->topic);
    }
    return status;
}

static void delay_first_send_for_late_joiners(pubsub_nanomsg_topic_sender_t *sender) {

    static bool firstSend = true;

    if(firstSend){
        L_INFO("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max){
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
