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

#ifndef CELIX_PUBSUB_NANOMSG_ADMIN_H
#define CELIX_PUBSUB_NANOMSG_ADMIN_H

#include <mutex>
#include <map>
#include <pubsub_admin.h>
#include "celix_api.h"
#include "pubsub_nanomsg_topic_receiver.h"
#include <pubsub_serializer.h>
#include "LogHelper.h"
#include "command.h"
#include "pubsub_nanomsg_topic_sender.h"
#include "pubsub_nanomsg_topic_receiver.h"

#define PUBSUB_NANOMSG_ADMIN_TYPE       "nanomsg"
#define PUBSUB_NANOMSG_URL_KEY          "nanomsg.url"

#define PUBSUB_NANOMSG_VERBOSE_KEY      "PSA_NANOMSG_VERBOSE"
#define PUBSUB_NANOMSG_VERBOSE_DEFAULT  true

#define PUBSUB_NANOMSG_PSA_IP_KEY       "PSA_IP"
#define PUBSUB_NANOMSG_PSA_ITF_KEY      "PSA_INTERFACE"

#define PUBSUB_NANOMSG_DEFAULT_IP       "127.0.0.1"

template <typename key, typename value>
struct ProtectedMap {
    std::mutex mutex{};
    std::map<key, value> map{};
};

class pubsub_nanomsg_admin {
public:
    pubsub_nanomsg_admin(celix_bundle_context_t *ctx);
    pubsub_nanomsg_admin(const pubsub_nanomsg_admin&) = delete;
    pubsub_nanomsg_admin& operator=(const pubsub_nanomsg_admin&) = delete;
    ~pubsub_nanomsg_admin();
    void start();
    void stop();

private:
    void addSerializerSvc(void *svc, const celix_properties_t *props);
    void removeSerializerSvc(void */*svc*/, const celix_properties_t *props);
    celix_status_t matchPublisher(
            long svcRequesterBndId,
            const celix_filter_t *svcFilter,
            celix_properties_t **outTopicProperties,
            double *outScore,
            long *outsSerializerSvcId);
    celix_status_t matchSubscriber(
            long svcProviderBndId,
            const celix_properties_t *svcProperties,
            celix_properties_t **outTopicProperties,
            double *outScope,
            long *outSerializerSvcId);
    celix_status_t matchEndpoint(const celix_properties_t *endpoint, bool *match);

    celix_status_t setupTopicSender(
            const char *scope,
            const char *topic,
            const celix_properties_t *topicProperties,
            long serializerSvcId,
            celix_properties_t **outPublisherEndpoint);

    celix_status_t teardownTopicSender(const char *scope, const char *topic);

    celix_status_t setupTopicReceiver(
            const std::string &scope,
            const std::string &topic,
            const celix_properties_t *topicProperties,
            long serializerSvcId,
            celix_properties_t **outSubscriberEndpoint);

    celix_status_t teardownTopicReceiver(const char *scope, const char *topic);

    celix_status_t addEndpoint(const celix_properties_t *endpoint);
    celix_status_t removeEndpoint(const celix_properties_t *endpoint);

    celix_status_t executeCommand(char *commandLine __attribute__((unused)), FILE *out,
                                                        FILE *errStream __attribute__((unused)));

    celix_status_t connectEndpointToReceiver(pubsub::nanomsg::topic_receiver *receiver,
                                                                   const celix_properties_t *endpoint);

    celix_status_t disconnectEndpointFromReceiver(pubsub::nanomsg::topic_receiver *receiver,
                                                                        const celix_properties_t *endpoint);
    celix_bundle_context_t *ctx;
    celix::pubsub::nanomsg::LogHelper L;
    pubsub_admin_service_t adminService{};
    long adminSvcId = -1L;
    long cmdSvcId = -1L;
    command_service_t cmdSvc{};
    long serializersTrackerId = -1L;

    const char *fwUUID{};

    char* ipAddress{};

    unsigned int basePort{};
    unsigned int maxPort{};

    double qosSampleScore{};
    double qosControlScore{};
    double defaultScore{};

    bool verbose{};

    class psa_nanomsg_serializer_entry {
    public:
        psa_nanomsg_serializer_entry(const char*_serType, long _svcId, pubsub_serializer_service_t *_svc) :
            serType{_serType}, svcId{_svcId}, svc{_svc} {

        }

        const char *serType;
        long svcId;
        pubsub_serializer_service_t *svc;
    };
    ProtectedMap<long, psa_nanomsg_serializer_entry> serializers{};
    ProtectedMap<std::string, pubsub::nanomsg::pubsub_nanomsg_topic_sender> topicSenders{};
    ProtectedMap<std::string, pubsub::nanomsg::topic_receiver*> topicReceivers{};
    ProtectedMap<const std::string, celix_properties_t *> discoveredEndpoints{};
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


#endif //CELIX_PUBSUB_NANOMSG_ADMIN_H
