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
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <utils.h>
#include <arpa/inet.h>
#include <zconf.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>


#include <pubsub_constants.h>
#include <pubsub_common.h>
#include "pubsub_nanomsg_topic_sender.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS                 2
#define NANOMSG_BIND_MAX_RETRY                      10

template <typename T>
std::stringstream LOG_STREAM(T first) {
    std::stringstream ss;
    ss << first;
    return ss;
}

template <typename T, typename... Args>
std::stringstream LOG_STREAM(T first, Args... args) {
    std::stringstream ss;
    ss << first << LOG_STREAM(args...).str();
    return ss;
}

template <typename... Args>
void L_DEBUG(log_helper_t *logHelper, Args... args) {
    std::stringstream ss = LOG_STREAM(args...);
    logHelper_log(logHelper, OSGI_LOGSERVICE_DEBUG, ss.str().c_str());
}

template <typename... Args>
void L_INFO(log_helper_t *logHelper, Args... args) {
    auto ss = LOG_STREAM(args...);
    logHelper_log(logHelper, OSGI_LOGSERVICE_INFO, ss.str().c_str());
}

template <typename... Args>
void L_WARN(log_helper_t *logHelper, Args... args) {
    auto ss = LOG_STREAM(args...);
    logHelper_log(logHelper, OSGI_LOGSERVICE_WARNING, ss.str().c_str());
}

template <typename... Args>
void L_ERROR(log_helper_t *logHelper, Args... args) {
    auto ss = LOG_STREAM(args...);
    logHelper_log((log_helper_pt)logHelper, OSGI_LOGSERVICE_ERROR, ss.str().c_str());
}

static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(log_helper_t* logHelper);

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
        serializer{_ser},
        scope{_scope},
        topic{_topic}{

    scopeAndTopicFilter = psa_nanomsg_setScopeAndTopicFilter(_scope, _topic);

    //setting up nanomsg socket for nanomsg TopicSender
    int nnSock = nn_socket(AF_SP, NN_BUS);
    if (nnSock == -1) {
        perror("Error for nanomsg_socket");
    }

    int rv = -1, retry=0;
    while(rv == -1 && retry < NANOMSG_BIND_MAX_RETRY ) {
        /* Randomized part due to same bundle publishing on different topics */
        unsigned int port = rand_range(_basePort,_maxPort);
        std::stringstream _url;
        _url << "tcp://" << _bindIp << ":" << port;

        std::stringstream bindUrl;
        bindUrl << "tcp://0.0.0.0:" << port;

        rv = nn_bind (nnSock, bindUrl.str().c_str());
        if (rv == -1) {
            perror("Error for nn_bind");
        } else {
            this->url = _url.str();
            nanomsg.socket = nnSock;
        }
        retry++;
    }

    if (!url.empty()) {

        //register publisher services using a service factory
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
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, topic.c_str());
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, scope.c_str());

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
    std::lock_guard<std::mutex> lock(boundedServices.mutex);
    for  (auto &it: boundedServices.map) {
            serializer->destroySerializerMap(serializer->handle, it.second.msgTypes);
    }
    boundedServices.map.clear();

}

long pubsub::nanomsg::pubsub_nanomsg_topic_sender::getSerializerSvcId() const {
    return serializerSvcId;
}

const std::string &pubsub::nanomsg::pubsub_nanomsg_topic_sender::getScope() const {
    return scope;
}

const std::string &pubsub::nanomsg::pubsub_nanomsg_topic_sender::getTopic() const {
    return topic;
}

const std::string &pubsub::nanomsg::pubsub_nanomsg_topic_sender::getUrl() const  {
    return url;
}


void* pubsub::nanomsg::pubsub_nanomsg_topic_sender::getPublisherService(const celix_bundle_t *requestingBundle,
                                             const celix_properties_t *svcProperties __attribute__((unused))) {
    long bndId = celix_bundle_getId(requestingBundle);
    void *service{nullptr};
    std::lock_guard<std::mutex> lock(boundedServices.mutex);
    auto existingEntry = boundedServices.map.find(bndId);
    if (existingEntry != boundedServices.map.end()) {
        existingEntry->second.getCount += 1;
        service = &existingEntry->second.service;
    } else {
        auto entry = boundedServices.map.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(bndId),
                                    std::forward_as_tuple(scope, topic, bndId, nanomsg.socket, logHelper));
        int rc = serializer->createSerializerMap(serializer->handle, (celix_bundle_t*)requestingBundle, &entry.first->second.msgTypes);

        if (rc == 0) {
            entry.first->second.service.handle = &entry.first->second;
            entry.first->second.service.localMsgTypeIdForMsgType = psa_nanoMsg_localMsgTypeIdForMsgType;
            entry.first->second.service.send = [](void *handle, unsigned int msgTypeId, const void *msg) {
                return static_cast<pubsub::nanomsg::bounded_service_entry*>(handle)->topicPublicationSend(msgTypeId, msg);
            };
            entry.first->second.service.sendMultipart = nullptr; //not supported TODO remove
            service = &entry.first->second.service;
        } else {
            boundedServices.map.erase(bndId);
            auto x =  LOG_STREAM(12, "hallo");
            logHelper_log(logHelper, OSGI_LOGSERVICE_DEBUG, x.str().c_str());
            log_helper_pt lh = logHelper;
            L_ERROR(lh, "Error creating serializer map for NanoMsg TopicSender ", scope, topic);
        }
    }

    return service;
}

void pubsub::nanomsg::pubsub_nanomsg_topic_sender::ungetPublisherService(const celix_bundle_t *requestingBundle,
                                                                         const celix_properties_t */*svcProperties*/) {
    long bndId = celix_bundle_getId(requestingBundle);

    std::lock_guard<std::mutex> lock(boundedServices.mutex);
    auto entry = boundedServices.map.find(bndId);
    if (entry != boundedServices.map.end()) {
        entry->second.getCount -= 1;
        if (entry->second.getCount == 0) {
            int rc = serializer->destroySerializerMap(serializer->handle, entry->second.msgTypes);
            if (rc != 0) {
                L_ERROR(logHelper, "Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
            }
            boundedServices.map.erase(bndId);
        }
    }
}

int pubsub::nanomsg::bounded_service_entry::topicPublicationSend(unsigned int msgTypeId, const void *inMsg) {
    int status;
    auto msgSer = static_cast<pubsub_msg_serializer_t*>(hashMap_get(msgTypes, (void*)(uintptr_t)msgTypeId));

    if (msgSer != nullptr) {
        delay_first_send_for_late_joiners(logHelper);

        int major = 0, minor = 0;

        pubsub_nanmosg_msg_header_t msg_hdr;// = calloc(1, sizeof(*msg_hdr));
        msg_hdr.type = msgTypeId;

        if (msgSer->msgVersion != nullptr) {
            version_getMajor(msgSer->msgVersion, &major);
            version_getMinor(msgSer->msgVersion, &minor);
            msg_hdr.major = (unsigned char) major;
            msg_hdr.minor = (unsigned char) minor;
        }

        void *serializedOutput = nullptr;
        size_t serializedOutputLen = 0;
        status = msgSer->serialize(msgSer, inMsg, &serializedOutput, &serializedOutputLen);
        if (status == CELIX_SUCCESS) {
            nn_iovec data[2];

            nn_msghdr msg{};
            msg.msg_iov = data;
            msg.msg_iovlen = 2;
            msg.msg_iov[0].iov_base = static_cast<void*>(&msg_hdr);
            msg.msg_iov[0].iov_len = sizeof(msg_hdr);
            msg.msg_iov[1].iov_base = serializedOutput;
            msg.msg_iov[1].iov_len = serializedOutputLen;
            msg.msg_control = nullptr;
            msg.msg_controllen = 0;
            errno = 0;
            int rc = nn_sendmsg(nanoMsgSocket, &msg, 0 );
            free(serializedOutput);
            if (rc < 0) {
                L_WARN(logHelper, "[PSA_ZMQ_TS] Error sending zmsg, rc: ", rc, ", error: ",  strerror(errno));
            } else {
                L_INFO(logHelper, "[PSA_ZMQ_TS] Send message with size ",  rc, "\n");
                L_INFO(logHelper, "[PSA_ZMQ_TS] Send message ID ", msg_hdr.type,
                        " major: ", (int)msg_hdr.major,
                        " minor: ",  (int)msg_hdr.minor,"\n");
            }
        } else {
            L_WARN(logHelper, "[PSA_ZMQ_TS] Error serialize message of type ", msgSer->msgName,
                    " for scope/topic ", scope.c_str(), "/", topic.c_str(),"\n");
        }
    } else {
        status = CELIX_SERVICE_EXCEPTION;
        L_WARN(logHelper, "[PSA_ZMQ_TS] Error cannot serialize message with msg type id ", msgTypeId,
                " for scope/topic ", scope.c_str(), "/", topic.c_str(),"\n");
    }
    return status;
}

static void delay_first_send_for_late_joiners(log_helper_t* logHelper) {

    static bool firstSend = true;

    if(firstSend){
        L_INFO(logHelper, "PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max){
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
