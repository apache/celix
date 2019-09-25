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

#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include "pubsub_serializer.h"
#include "LogHelper.h"
#include "celix_bundle_context.h"
#include "pubsub_nanomsg_common.h"
#include "pubsub/subscriber.h"

struct psa_nanomsg_subscriber_entry {
    psa_nanomsg_subscriber_entry(pubsub_subscriber_t *_svc, int _usageCount) :
        svc{_svc}, usageCount{_usageCount} {
    }
    pubsub_subscriber_t *svc{};
    int usageCount;
    hash_map_t *msgTypes{nullptr}; //map from serializer svc
};

typedef struct psa_nanomsg_requested_connection_entry {
public:
    psa_nanomsg_requested_connection_entry(std::string _url, int _id, bool _connected=false):
    url{_url}, id{_id}, connected{_connected} {
    }
    bool isConnected() const {
        return connected;
    }

    int getId() const {
        return id;
    }

    void setId(int _id) {
        id = _id;
    }
    void setConnected(bool c) {
        connected = c;
    }

    const std::string &getUrl() const {
        return url;
    }
private:
    std::string url;
    int id;
    bool connected;
} psa_nanomsg_requested_connection_entry_t;

namespace pubsub {
    namespace nanomsg {
        class topic_receiver {
        public:
            topic_receiver(celix_bundle_context_t
                           *ctx,
                           const std::string &scope,
                           const std::string &topic,
                           long serializerSvcId, pubsub_serializer_service_t
                           *serializer);
            topic_receiver(const topic_receiver &) = delete;
            topic_receiver & operator=(const topic_receiver &) = delete;
            ~topic_receiver();

            std::string scope() const;
            std::string topic() const;
            long serializerSvcId() const;
            void listConnections(std::vector<std::string> &connectedUrls, std::vector<std::string> &unconnectedUrls);
            void connectTo(const char *url);
            void disconnectFrom(const char *url);
            void recvThread_exec();
            void processMsg(const celix::pubsub::nanomsg::msg_header *hdr, const char *payload, size_t payloadSize);
            void processMsgForSubscriberEntry(psa_nanomsg_subscriber_entry* entry, const celix::pubsub::nanomsg::msg_header *hdr, const char* payload, size_t payloadSize);
            void addSubscriber(void *svc, const celix_properties_t *props, const celix_bundle_t *bnd);
            void removeSubscriber(void */*svc*/, const celix_properties_t */*props*/, const celix_bundle_t *bnd);
            celix_service_tracking_options_t createOptions();

        private:
            celix_bundle_context_t *ctx{nullptr};
            celix::pubsub::nanomsg::LogHelper L;
            long m_serializerSvcId{0};
            pubsub_serializer_service_t *serializer{nullptr};
            const std::string m_scope{};
            const std::string m_topic{};

            int m_nanoMsgSocket{0};

            struct {
                std::thread thread;
                std::mutex mutex;
                bool running;
            } recvThread{};

            struct {
                std::mutex mutex;
                std::map<std::string, psa_nanomsg_requested_connection_entry_t> map;
            } requestedConnections{};

            long subscriberTrackerId{0};
            struct {
                std::mutex mutex;
                std::map<long, psa_nanomsg_subscriber_entry> map;
            } subscribers{};
        };
    }
}

