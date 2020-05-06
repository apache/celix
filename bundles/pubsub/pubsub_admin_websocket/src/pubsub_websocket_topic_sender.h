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

#ifndef CELIX_PUBSUB_WEBSOCKET_TOPIC_SENDER_H
#define CELIX_PUBSUB_WEBSOCKET_TOPIC_SENDER_H

#include "celix_bundle_context.h"
#include "pubsub_admin_metrics.h"

typedef struct pubsub_websocket_topic_sender pubsub_websocket_topic_sender_t;

pubsub_websocket_topic_sender_t* pubsub_websocketTopicSender_create(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *ser);
void pubsub_websocketTopicSender_destroy(pubsub_websocket_topic_sender_t *sender);

const char* pubsub_websocketTopicSender_scope(pubsub_websocket_topic_sender_t *sender);
const char* pubsub_websocketTopicSender_topic(pubsub_websocket_topic_sender_t *sender);
const char* pubsub_websocketTopicSender_url(pubsub_websocket_topic_sender_t *sender);

long pubsub_websocketTopicSender_serializerSvcId(pubsub_websocket_topic_sender_t *sender);

#endif //CELIX_PUBSUB_WEBSOCKET_TOPIC_SENDER_H
