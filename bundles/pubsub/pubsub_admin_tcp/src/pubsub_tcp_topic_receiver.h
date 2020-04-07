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

#ifndef CELIX_PUBSUB_TCP_TOPIC_RECEIVER_H
#define CELIX_PUBSUB_TCP_TOPIC_RECEIVER_H

#include <pubsub_admin_metrics.h>
#include "celix_bundle_context.h"
#include <pubsub_protocol.h>
#include "pubsub_tcp_common.h"

typedef struct pubsub_tcp_topic_receiver pubsub_tcp_topic_receiver_t;

pubsub_tcp_topic_receiver_t *pubsub_tcpTopicReceiver_create(celix_bundle_context_t *ctx,
                                                            log_helper_t *logHelper,
                                                            const char *scope,
                                                            const char *topic,
                                                            const celix_properties_t *topicProperties,
                                                            pubsub_tcp_endPointStore_t *endPointStore,
                                                            long serializerSvcId,
                                                            pubsub_serializer_service_t *serializer,
                                                            long protocolSvcId,
                                                            pubsub_protocol_service_t *protocol);
void pubsub_tcpTopicReceiver_destroy(pubsub_tcp_topic_receiver_t *receiver);

const char *pubsub_tcpTopicReceiver_scope(pubsub_tcp_topic_receiver_t *receiver);
const char *pubsub_tcpTopicReceiver_topic(pubsub_tcp_topic_receiver_t *receiver);

long pubsub_tcpTopicReceiver_serializerSvcId(pubsub_tcp_topic_receiver_t *receiver);
long pubsub_tcpTopicReceiver_protocolSvcId(pubsub_tcp_topic_receiver_t *receiver);
void pubsub_tcpTopicReceiver_listConnections(pubsub_tcp_topic_receiver_t *receiver,
                                             celix_array_list_t *connectedUrls,
                                             celix_array_list_t *unconnectedUrls);

void pubsub_tcpTopicReceiver_connectTo(pubsub_tcp_topic_receiver_t *receiver, const char *url);
void pubsub_tcpTopicReceiver_disconnectFrom(pubsub_tcp_topic_receiver_t *receiver, const char *url);

pubsub_admin_receiver_metrics_t *pubsub_tcpTopicReceiver_metrics(pubsub_tcp_topic_receiver_t *receiver);

#endif //CELIX_PUBSUB_TCP_TOPIC_RECEIVER_H
