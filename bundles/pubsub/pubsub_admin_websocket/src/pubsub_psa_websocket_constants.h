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

#ifndef PUBSUB_PSA_WEBSOCKET_CONSTANTS_H_
#define PUBSUB_PSA_WEBSOCKET_CONSTANTS_H_

#define PSA_WEBSOCKET_DEFAULT_QOS_SAMPLE_SCORE        30
#define PSA_WEBSOCKET_DEFAULT_QOS_CONTROL_SCORE       70
#define PSA_WEBSOCKET_DEFAULT_SCORE                   30

#define PSA_WEBSOCKET_QOS_SAMPLE_SCORE_KEY            "PSA_WEBSOCKET_QOS_SAMPLE_SCORE"
#define PSA_WEBSOCKET_QOS_CONTROL_SCORE_KEY           "PSA_WEBSOCKET_QOS_CONTROL_SCORE"
#define PSA_WEBSOCKET_DEFAULT_SCORE_KEY               "PSA_WEBSOCKET_DEFAULT_SCORE"


#define PSA_WEBSOCKET_METRICS_ENABLED                 "PSA_WEBSOCKET_METRICS_ENABLED"
#define PSA_WEBSOCKET_DEFAULT_METRICS_ENABLED         true

#define PUBSUB_WEBSOCKET_VERBOSE_KEY                  "PSA_WEBSOCKET_VERBOSE"
#define PUBSUB_WEBSOCKET_VERBOSE_DEFAULT              true

#define PUBSUB_WEBSOCKET_ADMIN_TYPE                   "websocket"
#define PUBSUB_WEBSOCKET_ADDRESS_KEY                  "websocket.socket_address"
#define PUBSUB_WEBSOCKET_PORT_KEY                     "websocket.socket_port"

/**
 * The static url which a subscriber should try to connect to.
 * The urls are space separated
 */
#define PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES    "websocket.static.connect.socket_addresses"

/**
 * Name of environment variable with space-separated list of ips/urls to connect to
 * e.g. PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR_topic_scope="tcp://127.0.0.1:4444 tcp://127.0.0.2:4444"
 */
#define PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR "PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR_"

#endif /* PUBSUB_PSA_WEBSOCKET_CONSTANTS_H_ */
