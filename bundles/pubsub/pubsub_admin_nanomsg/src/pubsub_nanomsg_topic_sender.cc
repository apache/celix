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


#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <pubsub_common.h>
#include "pubsub_nanomsg_topic_sender.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS                 2
#define NANOMSG_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    logHelper_log(logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)


typedef struct psa_nanomsg_bounded_service_entry {
    pubsub::nanomsg::pubsub_nanomsg_topic_sender *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes;
    int getCount;
} psa_nanomsg_bounded_service_entry_t;

//static void* psa_nanomsg_getPublisherService(void *handle, const celix_bundle_t *requestingBundle,
//                                             const celix_properties_t *svcProperties);
//static void psa_nanomsg_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle,
//                                              const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub::nanomsg::pubsub_nanomsg_topic_sender *sender);

static int psa_nanomsg_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *msg);

pubsub::nanomsg::pubsub_nanomsg_topic_sender::pubsub_nanomsg_topic_sender(celix_bundle_context_t *_ctx,
                                                         log_helper_t *_logHelper,
                                                         const char *_scope,
                                                         const char *_topic,
                                                         long _serializerSvcId,
                                                         pubsub_serializer_service_t *_ser,
                                                         const char *_bindIp,
                                                         unsigned int _basePort,
                                                         unsigned int _maxPort) :
        ctx{_ctx},
        logHelper{_logHelper},
        serializerSvcId {_serializerSvcId},
        serializer{_ser}{

    psa_nanomsg_setScopeAndTopicFilter(_scope, _topic, scopeAndTopicFilter);

    //setting up nanomsg socket for nanomsg TopicSender
    {

        int socket = nn_socket(AF_SP, NN_BUS);
        if (socket == -1) {
            perror("Error for nanomsg_socket");
        }

        int rv = -1, retry=0;
        while(rv == -1 && retry < NANOMSG_BIND_MAX_RETRY ) {
            /* Randomized part due to same bundle publishing on different topics */
            unsigned int port = rand_range(_basePort,_maxPort);
            size_t len = (size_t)snprintf(NULL, 0, "tcp://%s:%u", _bindIp, port) + 1;
            char *_url = static_cast<char*>(calloc(len, sizeof(char*)));
            snprintf(_url, len, "tcp://%s:%u", _bindIp, port);

            len = (size_t)snprintf(NULL, 0, "tcp://0.0.0.0:%u", port) + 1;
            char *bindUrl = static_cast<char*>(calloc(len, sizeof(char)));
            snprintf(bindUrl, len, "tcp://0.0.0.0:%u", port);
            rv = nn_bind (socket, bindUrl);
            if (rv == -1) {
                perror("Error for nn_bind");
                free(_url);
            } else {
                this->url = _url;
                nanomsg.socket = socket;
            }
            retry++;
            free(bindUrl);
        }
    }

    if (url != NULL) {
        scope = strndup(_scope, 1024 * 1024);
        topic = strndup(_topic, 1024 * 1024);

        celixThreadMutex_create(&boundedServices.mutex, NULL);
        celixThreadMutex_create(&nanomsg.mutex, NULL);
        boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (url != NULL) {
        publisher.factory.handle = this;
        publisher.factory.getService = [](void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties) {
            return static_cast<pubsub::nanomsg::pubsub_nanomsg_topic_sender*>(handle)->getPublisherService(
                    requestingBundle,
                    svcProperties);
        };
        publisher.factory.ungetService = [](void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties) {
            return static_cast<pubsub::nanomsg::pubsub_nanomsg_topic_sender*>(handle)->ungetPublisherService(
                    requestingBundle,
                    svcProperties);
        };

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, topic);
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, scope);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.factory = &publisher.factory;
        opts.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_PUBLISHER_SERVICE_VERSION;
        opts.properties = props;

        publisher.svcId = celix_bundleContext_registerServiceWithOptions(_ctx, &opts);
    }

}

pubsub::nanomsg::pubsub_nanomsg_topic_sender::~pubsub_nanomsg_topic_sender() {
    celix_bundleContext_unregisterService(ctx, publisher.svcId);

    nn_close(nanomsg.socket);

    celixThreadMutex_lock(&boundedServices.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(boundedServices.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMapIterator_nextValue(&iter));
        if (entry != NULL) {
            serializer->destroySerializerMap(serializer->handle, entry->msgTypes);
            free(entry);
        }
    }
    hashMap_destroy(boundedServices.map, false, false);
    celixThreadMutex_unlock(&boundedServices.mutex);

    celixThreadMutex_destroy(&boundedServices.mutex);
    celixThreadMutex_destroy(&nanomsg.mutex);

    free(scope);
    free(topic);
    free(url);
}

long pubsub::nanomsg::pubsub_nanomsg_topic_sender::getSerializerSvcId() const {
    return serializerSvcId;
}

const char* pubsub::nanomsg::pubsub_nanomsg_topic_sender::getScope() const {
    return scope;
}

const char* pubsub::nanomsg::pubsub_nanomsg_topic_sender::getTopic() const {
    return topic;
}

const char* pubsub::nanomsg::pubsub_nanomsg_topic_sender::getUrl() const  {
    return url;
}


void* pubsub::nanomsg::pubsub_nanomsg_topic_sender::getPublisherService(const celix_bundle_t *requestingBundle,
                                             const celix_properties_t *svcProperties __attribute__((unused))) {
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&boundedServices.mutex);
    psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMap_get(boundedServices.map, (void*)bndId));
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(calloc(1, sizeof(*entry)));
        entry->getCount = 1;
        entry->parent = this;
        entry->bndId = bndId;

        int rc = serializer->createSerializerMap(serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_nanoMsg_localMsgTypeIdForMsgType;
            entry->service.send = psa_nanomsg_topicPublicationSend;
            entry->service.sendMultipart = NULL; //not supported TODO remove
            hashMap_put(boundedServices.map, (void*)bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for NanoMsg TopicSender %s/%s", scope, topic);
        }



    }
    celixThreadMutex_unlock(&boundedServices.mutex);

    return &entry->service;
}

void pubsub::nanomsg::pubsub_nanomsg_topic_sender::ungetPublisherService(const celix_bundle_t *requestingBundle,
                                              const celix_properties_t *svcProperties __attribute__((unused))) {
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&boundedServices.mutex);
    psa_nanomsg_bounded_service_entry_t *entry = static_cast<psa_nanomsg_bounded_service_entry_t*>(hashMap_get(boundedServices.map, (void*)bndId));
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(boundedServices.map, (void*)bndId);
        int rc = serializer->destroySerializerMap(serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }

        free(entry);
    }
    celixThreadMutex_unlock(&boundedServices.mutex);
}

static int psa_nanomsg_topicPublicationSend(void *handle, unsigned int msgTypeId, const void *inMsg) {
    int status = CELIX_SUCCESS;
    psa_nanomsg_bounded_service_entry_t *bound = static_cast<psa_nanomsg_bounded_service_entry_t*>(handle);
    pubsub::nanomsg::pubsub_nanomsg_topic_sender *sender = bound->parent;

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
                //TODO L_WARN("[PSA_ZMQ_TS] Error sending zmsg, rc is %i. %s", rc, strerror(errno));
            } else {
                //TODO L_INFO("[PSA_ZMQ_TS] Send message with size %d\n",  rc);
                //TODO L_INFO("[PSA_ZMQ_TS] Send message ID %d, major %d, minor %d\n",  msg_hdr.type, (int)msg_hdr.major, (int)msg_hdr.minor);
            }
        } else {
            //TODO L_WARN("[PSA_ZMQ_TS] Error serialize message of type %s for scope/topic %s/%s", msgSer->msgName, sender->scope, sender->topic);
        }
    } else {
        status = CELIX_SERVICE_EXCEPTION;
        //TODO L_WARN("[PSA_ZMQ_TS] Error cannot serialize message with msg type id %i for scope/topic %s/%s", msgTypeId, sender->scope, sender->topic);
    }
    return status;
}

static void delay_first_send_for_late_joiners(pubsub::nanomsg::pubsub_nanomsg_topic_sender */*sender*/) {

    static bool firstSend = true;

    if(firstSend){
        //TODO L_INFO("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max){
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
