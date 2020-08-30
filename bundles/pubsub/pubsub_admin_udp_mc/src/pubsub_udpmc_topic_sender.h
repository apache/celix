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

#ifndef CELIX_PUBSUB_UDPMC_TOPIC_SENDER_H
#define CELIX_PUBSUB_UDPMC_TOPIC_SENDER_H

#include "celix_bundle_context.h"
#include "pubsub_serializer.h"

typedef struct pubsub_udpmc_topic_sender pubsub_udpmc_topic_sender_t;

pubsub_udpmc_topic_sender_t* pubsub_udpmcTopicSender_create(
        celix_bundle_context_t *ctx,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *serializer,
        int sendSocket,
        const char *bindIP,
        const celix_properties_t *topicProperties);
void pubsub_udpmcTopicSender_destroy(pubsub_udpmc_topic_sender_t *sender);

const char* pubsub_udpmcTopicSender_scope(pubsub_udpmc_topic_sender_t *sender);
const char* pubsub_udpmcTopicSender_topic(pubsub_udpmc_topic_sender_t *sender);
const char* pubsub_udpmcTopicSender_socketAddress(pubsub_udpmc_topic_sender_t *sender);
long pubsub_udpmcTopicSender_socketPort(pubsub_udpmc_topic_sender_t *sender);
bool pubsub_udpmcTopicSender_isStatic(pubsub_udpmc_topic_sender_t *sender);


long pubsub_udpmcTopicSender_serializerSvcId(pubsub_udpmc_topic_sender_t *sender);

//TODO connections etc

void pubsub_udpmcTopicSender_connectTo(pubsub_udpmc_topic_sender_t *sender, const celix_properties_t *endpoint);
void pubsub_udpmcTopicSender_disconnectFrom(pubsub_udpmc_topic_sender_t *sender, const celix_properties_t *endpoint);

#endif //CELIX_PUBSUB_UDPMC_TOPIC_SENDER_H
