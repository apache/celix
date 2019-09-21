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

#ifndef CELIX_PUBSUB_WEBSOCKET_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_WEBSOCKET_TOPIC_RECEIVER_H

#include <pubsub_admin_metrics.h>
#include "celix_bundle_context.h"

typedef struct pubsub_websocket_topic_receiver pubsub_websocket_topic_receiver_t;

pubsub_websocket_topic_receiver_t* pubsub_websocketTopicReceiver_create(celix_bundle_context_t *ctx,
        log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        const celix_properties_t *topicProperties,
        long serializerSvcId,
        pubsub_serializer_service_t *serializer);
void pubsub_websocketTopicReceiver_destroy(pubsub_websocket_topic_receiver_t *receiver);

const char* pubsub_websocketTopicReceiver_scope(pubsub_websocket_topic_receiver_t *receiver);
const char* pubsub_websocketTopicReceiver_topic(pubsub_websocket_topic_receiver_t *receiver);

long pubsub_websocketTopicReceiver_serializerSvcId(pubsub_websocket_topic_receiver_t *receiver);
void pubsub_websocketTopicReceiver_listConnections(pubsub_websocket_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls);

void pubsub_websocketTopicReceiver_connectTo(pubsub_websocket_topic_receiver_t *receiver, const char *socketAddress, long socketPort, const char *uri);
void pubsub_websocketTopicReceiver_disconnectFrom(pubsub_websocket_topic_receiver_t *receiver, const char *uri);


pubsub_admin_receiver_metrics_t* pubsub_websocketTopicReceiver_metrics(pubsub_websocket_topic_receiver_t *receiver);


#endif //CELIX_PUBSUB_WEBSOCKET_TOPIC_RECEIVER_H
