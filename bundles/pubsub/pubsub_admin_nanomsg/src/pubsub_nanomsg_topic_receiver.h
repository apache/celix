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
#ifndef CELIX_PUBSUB_NANOMSG_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_NANOMSG_TOPIC_RECEIVER_H
#include <string>
#include <vector>
#include "pubsub_serializer.h"
#include "log_helper.h"
#include "celix_bundle_context.h"

//typedef struct pubsub_nanomsg_topic_receiver pubsub_nanomsg_topic_receiver_t;
namespace pubsub {
    namespace nanomsg {
        class topic_receiver {
        public:
            topic_receiver(celix_bundle_context_t
                           *ctx,
                           log_helper_t *logHelper,
                           const char *scope,
                           const char *topic,
                           long serializerSvcId, pubsub_serializer_service_t
                           *serializer);
            topic_receiver(const topic_receiver &) = delete;
            topic_receiver & operator=(const topic_receiver &) = delete;
            ~topic_receiver();

            const char* scope() const;
            const char* topic() const;
            long serializerSvcId() const;
            void listConnections(std::vector<std::string> &connectedUrls, std::vector<std::string> &unconnectedUrls);
            void connectTo(const char *url);
            void disconnectFrom(const char *url);
        private:
            celix_bundle_context_t *ctx{nullptr};
            log_helper_t *logHelper{nullptr};
            long m_serializerSvcId{0};
            pubsub_serializer_service_t *serializer{nullptr};
            const char *m_scope{nullptr};
            const char *m_topic{nullptr};
            char m_scopeAndTopicFilter[5];

            int m_nanoMsgSocket{0};

            struct {
                celix_thread_t thread;
                std::mutex mutex;
                bool running;
            } recvThread{};

            struct {
                std::mutex mutex;
                hash_map_t *map; //key = zmq url, value = psa_zmq_requested_connection_entry_t*
            } requestedConnections{};

            long subscriberTrackerId{0};
            struct {
                std::mutex mutex;
                hash_map_t *map; //key = bnd id, value = psa_zmq_subscriber_entry_t
            } subscribers{};
        };
    }}
//pubsub_nanomsg_topic_receiver_t* pubsub_nanoMsgTopicReceiver_create(celix_bundle_context_t *ctx,
//                                                                    log_helper_t *logHelper, const char *scope,
//                                                                    const char *topic, long serializerSvcId,
//                                                                    pubsub_serializer_service_t *serializer);
//void pubsub_nanoMsgTopicReceiver_destroy(pubsub_nanomsg_topic_receiver_t *receiver);

//const char* pubsub_nanoMsgTopicReceiver_scope(pubsub_nanomsg_topic_receiver_t *receiver);
//const char* pubsub_nanoMsgTopicReceiver_topic(pubsub_nanomsg_topic_receiver_t *receiver);

//long pubsub_nanoMsgTopicReceiver_serializerSvcId(pubsub_nanomsg_topic_receiver_t *receiver);
//void pubsub_nanoMsgTopicReceiver_listConnections(pubsub_nanomsg_topic_receiver_t *receiver,
//                                                 std::vector<std::string> &connectedUrls,
//                                                 std::vector<std::string> &unconnectedUrls);

//void pubsub_nanoMsgTopicReceiver_connectTo(pubsub_nanomsg_topic_receiver_t *receiver, const char *url);
//void pubsub_nanoMsgTopicReceiver_disconnectFrom(pubsub_nanomsg_topic_receiver_t *receiver, const char *url);

#endif //CELIX_PUBSUB_NANOMSG_TOPIC_RECEIVER_H
