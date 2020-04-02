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

#include <iostream>
#include <mutex>
#include <memory.h>
#include <vector>
#include <string>
#include <sstream>

#include <stdlib.h>
#include <assert.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <pubsub_serializer.h>
#include <pubsub/subscriber.h>
#include <pubsub_constants.h>
#include <pubsub_endpoint.h>
#include <LogHelper.h>

#include "pubsub_nanomsg_topic_receiver.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"
#include "pubsub_topology_manager.h"

//TODO see if block and wakeup (reset) also works
#define PSA_NANOMSG_RECV_TIMEOUT 100 //100 msec timeout

/*
#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)
*/
//#define L_DEBUG printf
//#define L_INFO printf
//#define L_WARN printf
//#define L_ERROR printf


pubsub::nanomsg::topic_receiver::topic_receiver(celix_bundle_context_t *_ctx,
        const std::string &_scope,
        const std::string &_topic,
        long _serializerSvcId,
        pubsub_serializer_service_t *_serializer) : L{_ctx, "NANOMSG_topic_receiver"}, m_serializerSvcId{_serializerSvcId}, m_scope{_scope}, m_topic{_topic} {
    ctx = _ctx;
    serializer = _serializer;

    m_nanoMsgSocket = nn_socket(AF_SP, NN_BUS);
    if (m_nanoMsgSocket < 0) {
        L.ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for scope/topic: ", m_scope.c_str(), "/", m_topic.c_str());
        std::bad_alloc{};
    } else {
        int timeout = PSA_NANOMSG_RECV_TIMEOUT;
        if (nn_setsockopt(m_nanoMsgSocket , NN_SOL_SOCKET, NN_RCVTIMEO, &timeout,
                          sizeof (timeout)) < 0) {
            L.ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for ",m_scope, "/",m_topic, ", set sockopt RECV_TIMEO failed");
            std::bad_alloc{};
        }

        auto subscriberFilter = celix::pubsub::nanomsg::setScopeAndTopicFilter(m_scope, m_topic);

        auto opts = createOptions();

        subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
        recvThread.running = true;
        free ((void*)opts.filter.filter);
        recvThread.thread = std::thread([this]() {this->recvThread_exec();});
    }
}

celix_service_tracking_options_t pubsub::nanomsg::topic_receiver::createOptions() {
    std::stringstream filter_str;

    filter_str << "(" << PUBSUB_SUBSCRIBER_TOPIC << "=" <<  m_topic << ")";
    celix_service_tracking_options_t opts{};
    opts.filter.ignoreServiceLanguage = true;
    opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
    opts.filter.filter = strdup(filter_str.str().c_str()); // TODO : memory leak ??
    opts.callbackHandle = this;
    opts.addWithOwner = [](void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) {
            static_cast<pubsub::nanomsg::topic_receiver*>(handle)->addSubscriber(svc, props, svcOwner);
        };
    opts.removeWithOwner = [](void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) {
            static_cast<pubsub::nanomsg::topic_receiver*>(handle)->removeSubscriber(svc, props, svcOwner);
        };
    return opts;
}

pubsub::nanomsg::topic_receiver::~topic_receiver() {

        {
            std::lock_guard<std::mutex> _lock(recvThread.mutex);
            recvThread.running = false;
        }
        recvThread.thread.join();

        celix_bundleContext_stopTracker(ctx, subscriberTrackerId);

        {
            std::lock_guard<std::mutex> _lock(subscribers.mutex);
            for (auto elem : subscribers.map) {
                serializer->destroySerializerMap(serializer->handle, elem.second.msgTypes);
            }
            subscribers.map.clear();
        }
        nn_close(m_nanoMsgSocket);

}

std::string pubsub::nanomsg::topic_receiver::scope() const {
    return m_scope;
}

std::string pubsub::nanomsg::topic_receiver::topic() const {
    return m_topic;
}

long pubsub::nanomsg::topic_receiver::serializerSvcId() const {
    return m_serializerSvcId;
}

void pubsub::nanomsg::topic_receiver::listConnections(std::vector<std::string> &connectedUrls,
                                                 std::vector<std::string> &unconnectedUrls) {
    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    for (auto entry : requestedConnections.map) {
        if (entry.second.isConnected()) {
            connectedUrls.emplace_back(entry.second.getUrl());
        } else {
            unconnectedUrls.emplace_back(entry.second.getUrl());
        }
    }
}


void pubsub::nanomsg::topic_receiver::connectTo(const char *url) {
    L.DBG("[PSA_NANOMSG] TopicReceiver ", m_scope, "/", m_topic, " connecting to nanomsg url ", url);

    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    auto entry  = requestedConnections.map.find(url);
    if (entry == requestedConnections.map.end()) {
        requestedConnections.map.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(std::string(url)),
                std::forward_as_tuple(url, -1));
        entry = requestedConnections.map.find(url);
    }
    if (!entry->second.isConnected()) {
        int connection_id = nn_connect(m_nanoMsgSocket, url);
        if (connection_id >= 0) {
            entry->second.setConnected(true);
            entry->second.setId(connection_id);
        } else {
            L.WARN("[PSA_NANOMSG] Error connecting to NANOMSG url ", url, " (",strerror(errno), ")");
        }
    }
}

void pubsub::nanomsg::topic_receiver::disconnectFrom(const char *url) {
    L.DBG("[PSA NANOMSG] TopicReceiver ", m_scope, "/", m_topic, " disconnect from nanomsg url ", url);

    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    auto entry = requestedConnections.map.find(url);
    if (entry != requestedConnections.map.end()) {
        if (entry->second.isConnected()) {
            if (nn_shutdown(m_nanoMsgSocket, entry->second.getId()) == 0) {
                entry->second.setConnected(false);
            } else {
                L.WARN("[PSA_NANOMSG] Error disconnecting from nanomsg url ", url, ", id: ", entry->second.getId(), " (",strerror(errno),")");
            }
        }
        requestedConnections.map.erase(url);
        std::cerr << "REMOVING connection " << url << std::endl;
    } else {
        std::cerr << "Disconnecting from unknown URL " << url << std::endl;
    }
}

void pubsub::nanomsg::topic_receiver::addSubscriber(void *svc, const celix_properties_t *props,
                                                      const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);
    std::string subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, PUBSUB_DEFAULT_ENDPOINT_SCOPE);
    if (subScope != m_scope) {
        //not the same scope. ignore
        return;
    }

    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    auto entry = subscribers.map.find(bndId);
    if (entry != subscribers.map.end()) {
        entry->second.usageCount += 1;
    } else {
        //new create entry
        subscribers.map.emplace(std::piecewise_construct,
                std::forward_as_tuple(bndId),
                std::forward_as_tuple(static_cast<pubsub_subscriber_t*>(svc), 1));
        entry = subscribers.map.find(bndId);

        int rc = serializer->createSerializerMap(serializer->handle, (celix_bundle_t*)bnd, &entry->second.msgTypes);
        if (rc != 0) {
            L.ERROR("[PSA_NANOMSG] Cannot create msg serializer map for TopicReceiver ", m_scope.c_str(), "/", m_topic.c_str());
            subscribers.map.erase(bndId);
        }
    }
}

void pubsub::nanomsg::topic_receiver::removeSubscriber(void */*svc*/,
                                                         const celix_properties_t */*props*/, const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);

    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    auto entry = subscribers.map.find(bndId);
    if (entry != subscribers.map.end()) {
        entry->second.usageCount -= 1;
        if (entry->second.usageCount <= 0) {
            //remove entry
            int rc = serializer->destroySerializerMap(serializer->handle, entry->second.msgTypes);
            if (rc != 0) {
                L.ERROR("[PSA_NANOMSG] Cannot destroy msg serializers map for TopicReceiver ", m_scope.c_str(), "/",m_topic.c_str(),"\n");
            }
            subscribers.map.erase(bndId);
        }
    }
}

void pubsub::nanomsg::topic_receiver::processMsgForSubscriberEntry(psa_nanomsg_subscriber_entry* entry, const celix::pubsub::nanomsg::msg_header *hdr, const char* payload, size_t payloadSize) {
    pubsub_msg_serializer_t* msgSer = static_cast<pubsub_msg_serializer_t*>(hashMap_get(entry->msgTypes, (void*)(uintptr_t)(hdr->type)));
    pubsub_subscriber_t *svc = entry->svc;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = celix::pubsub::nanomsg::checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            celix_status_t status = msgSer->deserialize(msgSer, payload, payloadSize, &deserializedMsg);
            if (status == CELIX_SUCCESS) {
                bool release = false;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, &release);
                if (release) {
                    msgSer->freeMsg(msgSer->handle, deserializedMsg);
                }
            } else {
                L.WARN("[PSA_NANOMSG_TR] Cannot deserialize msg type ", msgSer->msgName , "for scope/topic ", scope(), "/", topic());
            }
        }
    } else {
        L.WARN("[PSA_NANOMSG_TR] Cannot find serializer for type id ", hdr->type);
    }
}

void pubsub::nanomsg::topic_receiver::processMsg(const celix::pubsub::nanomsg::msg_header *hdr, const char *payload, size_t payloadSize) {
    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    for (auto entry : subscribers.map) {
        processMsgForSubscriberEntry(&entry.second, hdr, payload, payloadSize);
    }
}

struct Message {
    celix::pubsub::nanomsg::msg_header header;
    char payload[];
};

void pubsub::nanomsg::topic_receiver::recvThread_exec() {
    while (recvThread.running) {
        Message *msg = nullptr;
        nn_iovec iov[2];
        iov[0].iov_base = &msg;
        iov[0].iov_len = NN_MSG;

        nn_msghdr msgHdr;
        memset(&msgHdr, 0, sizeof(msgHdr));

        msgHdr.msg_iov = iov;
        msgHdr.msg_iovlen = 1;

        msgHdr.msg_control = nullptr;
        msgHdr.msg_controllen = 0;

        errno = 0;
        int recvBytes = nn_recvmsg(m_nanoMsgSocket, &msgHdr, 0);
        if (msg && static_cast<unsigned long>(recvBytes) >= sizeof(celix::pubsub::nanomsg::msg_header)) {
            processMsg(&msg->header, msg->payload, recvBytes-sizeof(msg->header));
            nn_freemsg(msg);
        } else if (recvBytes >= 0) {
            L.ERROR("[PSA_NANOMSG_TR] Error receiving nanomsg msg, size (", recvBytes,") smaller than header\n");
        } else if (errno == EAGAIN || errno == ETIMEDOUT) {
            // no data: go to next cycle
        } else if (errno == EINTR) {
            L.DBG("[PSA_NANOMSG_TR] nn_recvmsg interrupted");
        } else {
            L.WARN("[PSA_NANOMSG_TR] Error receiving nanomessage: errno ", errno, " : ",  strerror(errno), "\n");
        }
    } // while

}
