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

#ifndef CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H

#include "celix_bundle_context.h"
#include "pubsub_serializer.h"
#include "celix_log_helper.h"

typedef struct pubsub_udpmc_topic_receiver pubsub_udpmc_topic_receiver_t;

pubsub_udpmc_topic_receiver_t* pubsub_udpmcTopicReceiver_create(celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const char *scope, 
        const char *topic, 
        const char *ifIP,
        const celix_properties_t *topicProperties,
        long serializerSvcId,
        pubsub_serializer_service_t *serializer);
void pubsub_udpmcTopicReceiver_destroy(pubsub_udpmc_topic_receiver_t *receiver);

const char* pubsub_udpmcTopicReceiver_scope(pubsub_udpmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_topic(pubsub_udpmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_socketAddress(pubsub_udpmc_topic_receiver_t *receiver);
void pubsub_udpmcTopicReceiver_listConnections(pubsub_udpmc_topic_receiver_t *receiver, celix_array_list_t *connections);

long pubsub_udpmcTopicReceiver_serializerSvcId(pubsub_udpmc_topic_receiver_t *receiver);

void pubsub_udpmcTopicReceiver_connectTo(pubsub_udpmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort);
void pubsub_udpmcTopicReceiver_disconnectFrom(pubsub_udpmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort);

#endif //CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H
