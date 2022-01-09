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

#ifndef CELIX_PUBSUB_UDP_TOPIC_SENDER_H
#define CELIX_PUBSUB_UDP_TOPIC_SENDER_H

#include "celix_bundle_context.h"
#include "pubsub_admin_metrics.h"
#include "pubsub_protocol.h"
#include "pubsub_skt_handler.h"
#include "pubsub_serializer_handler.h"

typedef struct pubsub_udp_topic_sender pubsub_udp_topic_sender_t;

pubsub_udp_topic_sender_t *pubsub_udpTopicSender_create(
    celix_bundle_context_t *ctx,
    celix_log_helper_t *logHelper,
    const char *scope,
    const char *topic,
    pubsub_serializer_handler_t* serializerHandler,
    void *admin,
    const celix_properties_t *topicProperties,
    pubsub_sktHandler_endPointStore_t *handlerStore,
    long protocolSvcId,
    pubsub_protocol_service_t *prot);

void pubsub_udpTopicSender_destroy(pubsub_udp_topic_sender_t *sender);
const char *pubsub_udpTopicSender_scope(pubsub_udp_topic_sender_t *sender);
const char *pubsub_udpTopicSender_topic(pubsub_udp_topic_sender_t *sender);
const char *pubsub_udpTopicSender_url(pubsub_udp_topic_sender_t *sender);
const char* pubsub_udpTopicSender_serializerType(pubsub_udp_topic_sender_t *sender);
bool pubsub_udpTopicSender_isStatic(pubsub_udp_topic_sender_t *sender);
bool pubsub_udpTopicSender_isPassive(pubsub_udp_topic_sender_t *sender);
long pubsub_udpTopicSender_protocolSvcId(pubsub_udp_topic_sender_t *sender);
void pubsub_udpTopicSender_connectTo(pubsub_udp_topic_sender_t *receiver, const char *url);
void pubsub_udpTopicSender_disconnectFrom(pubsub_udp_topic_sender_t *receiver, const char *url);
void pubsub_udpTopicSender_listConnections(pubsub_udp_topic_sender_t *sender, celix_array_list_t *urls);

#endif //CELIX_PUBSUB_udp_TOPIC_SENDER_H
