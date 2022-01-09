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

#ifndef CELIX_PUBSUB_UDP_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_UDP_TOPIC_RECEIVER_H

#include "pubsub_admin_metrics.h"
#include "celix_bundle_context.h"
#include "pubsub_protocol.h"
#include "pubsub_serializer_handler.h"

typedef struct pubsub_udp_topic_receiver pubsub_udp_topic_receiver_t;

pubsub_udp_topic_receiver_t *pubsub_udpTopicReceiver_create(celix_bundle_context_t *ctx,
                                                            celix_log_helper_t *logHelper,
                                                            const char *scope,
                                                            const char *topic,
                                                            pubsub_serializer_handler_t* serializerHandler,
                                                            void *admin,
                                                            const celix_properties_t *topicProperties,
                                                            pubsub_sktHandler_endPointStore_t *handlerStore,
                                                            long protocolSvcId,
                                                            pubsub_protocol_service_t *protocol);
void pubsub_udpTopicReceiver_destroy(pubsub_udp_topic_receiver_t *receiver);

const char *pubsub_udpTopicReceiver_scope(pubsub_udp_topic_receiver_t *receiver);
const char *pubsub_udpTopicReceiver_topic(pubsub_udp_topic_receiver_t *receiver);
const char *pubsub_udpTopicReceiver_serializerType(pubsub_udp_topic_receiver_t *sender);

long pubsub_udpTopicReceiver_protocolSvcId(pubsub_udp_topic_receiver_t *receiver);
void pubsub_udpTopicReceiver_listConnections(pubsub_udp_topic_receiver_t *receiver,
                                             celix_array_list_t *connectedUrls,
                                             celix_array_list_t *unconnectedUrls);
bool pubsub_udpTopicReceiver_isPassive(pubsub_udp_topic_receiver_t *receiver);
const char *pubsub_udpTopicReceiver_url(pubsub_udp_topic_receiver_t *receiver);
bool pubsub_udpTopicReceiver_isStatic(pubsub_udp_topic_receiver_t *receiver);
void pubsub_udpTopicReceiver_connectTo(pubsub_udp_topic_receiver_t *receiver, const char *url);
void pubsub_udpTopicReceiver_disconnectFrom(pubsub_udp_topic_receiver_t *receiver, const char *url);

#endif //CELIX_PUBSUB_UDP_TOPIC_RECEIVER_H
