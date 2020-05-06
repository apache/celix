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

#ifndef CELIX_PUBSUB_ZMQ_TOPIC_SENDER_H
#define CELIX_PUBSUB_ZMQ_TOPIC_SENDER_H

#include "celix_bundle_context.h"
#include "pubsub_admin_metrics.h"
#include "celix_log_helper.h"

typedef struct pubsub_zmq_topic_sender pubsub_zmq_topic_sender_t;

pubsub_zmq_topic_sender_t* pubsub_zmqTopicSender_create(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *ser,
        long protocolSvcId,
        pubsub_protocol_service_t *prot,
        const char *bindIP,
        const char *staticBindUrl,
        unsigned int basePort,
        unsigned int maxPort);
void pubsub_zmqTopicSender_destroy(pubsub_zmq_topic_sender_t *sender);

const char* pubsub_zmqTopicSender_scope(pubsub_zmq_topic_sender_t *sender);
const char* pubsub_zmqTopicSender_topic(pubsub_zmq_topic_sender_t *sender);
const char* pubsub_zmqTopicSender_url(pubsub_zmq_topic_sender_t *sender);
bool pubsub_zmqTopicSender_isStatic(pubsub_zmq_topic_sender_t *sender);

long pubsub_zmqTopicSender_serializerSvcId(pubsub_zmq_topic_sender_t *sender);
long pubsub_zmqTopicSender_protocolSvcId(pubsub_zmq_topic_sender_t *sender);

void pubsub_zmqTopicSender_connectTo(pubsub_zmq_topic_sender_t *sender, const celix_properties_t *endpoint);
void pubsub_zmqTopicSender_disconnectFrom(pubsub_zmq_topic_sender_t *sender, const celix_properties_t *endpoint);

/**
 * Returns a array of pubsub_admin_sender_msg_type_metrics_t entries for every msg_type/bundle send with the topic sender.
 */
pubsub_admin_sender_metrics_t* pubsub_zmqTopicSender_metrics(pubsub_zmq_topic_sender_t *sender);

#endif //CELIX_PUBSUB_ZMQ_TOPIC_SENDER_H
