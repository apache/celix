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

#ifndef PUBSUB_PSA_TCP_CONSTANTS_H_
#define PUBSUB_PSA_TCP_CONSTANTS_H_

#define PSA_TCP_BASE_PORT                       "PSA_TCP_BASE_PORT"
#define PSA_TCP_MAX_PORT                        "PSA_TCP_MAX_PORT"

#define PSA_TCP_MAX_RECV_SESSIONS               "PSA_TCP_MAX_RECV_SESSIONS"
#define PSA_TCP_RECV_BUFFER_SIZE                "PSA_TCP_RECV_BUFFER_SIZE"
#define PSA_TCP_TIMEOUT                         "PSA_TCP_TIMEOUT"

#define PSA_TCP_DEFAULT_BASE_PORT               5501
#define PSA_TCP_DEFAULT_MAX_PORT                6000

#define PSA_TCP_DEFAULT_MAX_RECV_SESSIONS       1

#define PSA_TCP_DEFAULT_RECV_BUFFER_SIZE        65 * 1024
#define PSA_TCP_DEFAULT_TIMEOUT                 2000

#define PSA_TCP_DEFAULT_QOS_SAMPLE_SCORE        30
#define PSA_TCP_DEFAULT_QOS_CONTROL_SCORE       70
#define PSA_TCP_DEFAULT_SCORE                   30

#define PSA_TCP_QOS_SAMPLE_SCORE_KEY            "PSA_TCP_QOS_SAMPLE_SCORE"
#define PSA_TCP_QOS_CONTROL_SCORE_KEY           "PSA_TCP_QOS_CONTROL_SCORE"
#define PSA_TCP_DEFAULT_SCORE_KEY               "PSA_TCP_DEFAULT_SCORE"


#define PSA_TCP_METRICS_ENABLED                 "PSA_TCP_METRICS_ENABLED"
#define PSA_TCP_DEFAULT_METRICS_ENABLED         true

#define PUBSUB_TCP_VERBOSE_KEY                  "PSA_TCP_VERBOSE"
#define PUBSUB_TCP_VERBOSE_DEFAULT              true

#define PUBSUB_TCP_PUBLISHER_BLOCKING_KEY       "PUBSUB_TCP_PUBLISHER_BLOCKING"
#define PUBSUB_TCP_PUBLISHER_BLOCKING_DEFAULT   true

#define PUBSUB_TCP_SUBSCRIBER_BLOCKING_KEY      "PUBSUB_TCP_SUBSCRIBER_BLOCKING"
#define PUBSUB_TCP_SUBSCRIBER_BLOCKING_DEFAULT   true

#define PUBSUB_TCP_PSA_IP_KEY                   "PSA_IP"
#define PUBSUB_TCP_PSA_ITF_KEY                  "PSA_INTERFACE"

#define PUBSUB_TCP_DEFAULT_IP                   "127.0.0.1"

#define PUBSUB_TCP_ADMIN_TYPE                   "tcp"

/**
 * The TCP url key for the topic sender endpoints
 */
#define PUBSUB_TCP_URL_KEY                      "tcp.url"

/**
 * Can be set in the topic properties to fix a static bind url
 */
#define PUBSUB_TCP_STATIC_BIND_URL              "tcp.static.bind.url"

/**
 * Can be set in the topic properties to fix a static url used for discovery
 */
#define PUBSUB_TCP_STATIC_DISCOVER_URL          "tcp.static.bind.url"

/**
 * If set true on the endpoint, the tcp TopicSender bind and/or discovery url is statically configured.
 */
#define PUBSUB_TCP_STATIC_CONFIGURED            "tcp.static.configured"

/**
 * The static url which a subscriber should try to connect to.
 * The urls are space separated.
 * Can be set in the topic properties.
 */
#define PUBSUB_TCP_STATIC_CONNECT_URLS          "tcp.static.connect.urls"

/**
 * The static endpoint type which a static endpoint should be configured.
 * Can be set in the topic properties.
 */
#define PUBSUB_TCP_STATIC_ENDPOINT_TYPE         "tcp.static.endpoint.type"


#define PUBSUB_TCP_STATIC_ENDPOINT_TYPE_SERVER  "server"
#define PUBSUB_TCP_STATIC_ENDPOINT_TYPE_CLIENT  "client"


/**
 * The TCP admin supports send message without pubsub header.
 * In this case a message_id must be part of the message to be able to distinguish
 * the different messages. The location of the message ID is configured with PUBSUB_TCP_MESSAGE_ID_OFFSET.
 * The size of the message ID in bytes is specified with PUBSUB_TCP_MESSAGE_ID_SIZE
 * The properties can be set in the topic properties.
 */
#define PUBSUB_TCP_BYPASS_HEADER          "tcp.static.bypass.header"
#define PUBSUB_TCP_DEFAULT_BYPASS_HEADER  false
#define PUBSUB_TCP_MESSAGE_ID_OFFSET      "tcp.static.message_id.offset"
#define PUBSUB_TCP_DEFAULT_MESSAGE_ID_OFFSET 0
#define PUBSUB_TCP_MESSAGE_ID_SIZE        "tcp.static.message_id.size"
#define PUBSUB_TCP_DEFAULT_MESSAGE_ID_SIZE   4

/**
 * Realtime thread prio and scheduling information. This is used to setup the thread prio/sched of the
 * internal TCP threads.
 * Can be set in the topic properties.
 */
#define PUBSUB_TCP_THREAD_REALTIME_PRIO         "thread.realtime.prio"
#define PUBSUB_TCP_THREAD_REALTIME_SCHED        "thread.realtime.sched"


#endif /* PUBSUB_PSA_TCP_CONSTANTS_H_ */
