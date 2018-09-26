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
#ifndef CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H

#include "celix_bundle_context.h"

typedef struct pubsub_updmc_topic_receiver pubsub_updmc_topic_receiver_t;

pubsub_updmc_topic_receiver_t* pubsub_udpmcTopicReceiver_create(celix_bundle_context_t *ctx, 
        const char *scope, 
        const char *topic, 
        const char *ifIP, 
        long serializerSvcId);
void pubsub_udpmcTopicReceiver_destroy(pubsub_updmc_topic_receiver_t *receiver);

const char* pubsub_udpmcTopicReceiver_psaType(pubsub_updmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_serializerType(pubsub_updmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_scope(pubsub_updmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_topic(pubsub_updmc_topic_receiver_t *receiver);
const char* pubsub_udpmcTopicReceiver_socketAddress(pubsub_updmc_topic_receiver_t *receiver);

void pubsub_udpmcTopicReceiver_connectTo(
        pubsub_updmc_topic_receiver_t *receiver,
        const char *socketAddress,
        long socketPort);
void pubsub_udpmcTopicReceiver_disconnectFrom(pubsub_updmc_topic_receiver_t *receiver, const char *socketAddress, long socketPort);

#endif //CELIX_PUBSUB_UDPMC_TOPIC_RECEIVER_H
