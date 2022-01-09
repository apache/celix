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

#ifndef PUBSUB_PSA_UDP_CONSTANTS_H_
#define PUBSUB_PSA_UDP_CONSTANTS_H_

#define PSA_UDP_BASE_PORT                       "PSA_UDP_BASE_PORT"
#define PSA_UDP_MAX_PORT                        "PSA_UDP_MAX_PORT"

#define PSA_UDP_MAX_MESSAGE_SIZE                "PSA_UDP_MAX_MESSAGE_SIZE"
#define PSA_UDP_RECV_BUFFER_SIZE                "PSA_UDP_RECV_BUFFER_SIZE"
#define PSA_UDP_TIMEOUT                         "PSA_UDP_TIMEOUT"
#define PSA_UDP_SUBSCRIBER_CONNECTION_TIMEOUT   "PSA_UDP_SUBSCRIBER_CONNECTION_TIMEOUT"

#define PSA_UDP_DEFAULT_BASE_PORT               5501
#define PSA_UDP_DEFAULT_MAX_PORT                6000

#define PSA_UDP_DEFAULT_MAX_MESSAGE_SIZE        64 * 1024
#define PSA_UDP_DEFAULT_RECV_BUFFER_SIZE        64 * 1024
#define PSA_UDP_DEFAULT_TIMEOUT                 2000 // 2 seconds
#define PSA_UDP_SUBSCRIBER_CONNECTION_DEFAULT_TIMEOUT 250 // 250 ms

#define PSA_UDP_DEFAULT_QOS_SAMPLE_SCORE        30
#define PSA_UDP_DEFAULT_QOS_CONTROL_SCORE       70
#define PSA_UDP_DEFAULT_SCORE                   30

#define PSA_UDP_QOS_SAMPLE_SCORE_KEY            "PSA_UDP_QOS_SAMPLE_SCORE"
#define PSA_UDP_QOS_CONTROL_SCORE_KEY           "PSA_UDP_QOS_CONTROL_SCORE"
#define PSA_UDP_DEFAULT_SCORE_KEY               "PSA_UDP_DEFAULT_SCORE"

#define PUBSUB_UDP_VERBOSE_KEY                  "PSA_UDP_VERBOSE"
#define PUBSUB_UDP_VERBOSE_DEFAULT              false

#define PUBSUB_UDP_PUBLISHER_RETRY_CNT_KEY      "PUBSUB_UDP_PUBLISHER_RETRY_COUNT"
#define PUBSUB_UDP_PUBLISHER_RETRY_CNT_DEFAULT  5

#define PUBSUB_UDP_SUBSCRIBER_RETRY_CNT_KEY     "PUBSUB_UDP_SUBSCRIBER_RETRY_COUNT"
#define PUBSUB_UDP_SUBSCRIBER_RETRY_CNT_DEFAULT 5


//Time-out settings are only for BLOCKING connections
#define PUBSUB_UDP_PUBLISHER_SNDTIMEO_KEY       "PUBSUB_UDP_PUBLISHER_SEND_TIMEOUT"
#define PUBSUB_UDP_PUBLISHER_SNDTIMEO_DEFAULT   5.0

#define PUBSUB_UDP_SUBSCRIBER_RCVTIMEO_KEY      "PUBSUB_UDP_SUBSCRIBER_RCV_TIMEOUT"
#define PUBSUB_UDP_SUBSCRIBER_RCVTIMEO_DEFAULT  5.0

#define PUBSUB_UDP_PSA_IP_KEY                   "PSA_IP"
#define PUBSUB_UDP_ADMIN_TYPE                   "udp"

/**
 * The UDP url key for the topic sender endpoints
 */
#define PUBSUB_UDP_URL_KEY                      "udp.url"

/**
 * Can be set in the topic properties to fix a static bind url
 */
#define PUBSUB_UDP_STATIC_BIND_URL              "udp.static.bind.url"

/**
 * Name of environment variable with ip/url to bind to
 * e.g. PSA_UDP_STATIC_BIND_FOR_topic_scope="UDP://0.0.0.0:4444"
 */
#define PUBSUB_UDP_STATIC_BIND_URL_FOR          "PSA_UDP_STATIC_BIND_URL_FOR_"

/**
 * Can be set in the topic properties to fix a static url used for discovery
 */
#define PUBSUB_UDP_STATIC_DISCOVER_URL          "udp.static.bind.url"

/**
 * If set true on the endpoint, the UDP TopicSender bind and/or discovery url is statically configured.
 */
#define PUBSUB_UDP_STATIC_CONFIGURED            "udp.static.configured"

/**
 * The static url which a subscriber should try to connect to.
 * The urls are space separated.
 * Can be set in the topic properties
 */
#define PUBSUB_UDP_STATIC_CONNECT_URLS          "udp.static.connect.urls"


/**
 * Defines if the publisher / subscriber is a passive endpoint and shares
 * the connection with publisher / subscriber endpoint with the matching (passive) key
 * e.g. UDP.passive.configured="true" means that a publisher / subscriber is passive,
 * when a publisher / subscriber is found with a matching key (for example UDP.passive.key="localhost").
 * This creates full-duplex connection using a single socket.
 */
#define PUBSUB_UDP_PASSIVE_CONFIGURED            "udp.passive.configured"
#define PUBSUB_UDP_PASSIVE_KEY                   "udp.passive.key"

/**
 * Name of environment variable to indicate that passive endpoint is configured
 * e.g. PSA_UDP_PASSIVE_CONFIGURED_topic_scope="true"
 */
#define PUBSUB_UDP_PASSIVE_ENABLED               "PSA_UDP_PASSIVE_CONFIGURED_"
/**
 * Name of environment variable to configure the passive key (see PUBSUB_UDP_PASSIVE_KEY )
 * e.g. PSA_UDP_PASSIVE_KEY__topic_scope="UDP://localhost:4444"
 */
#define PUBSUB_UDP_PASSIVE_SELECTION_KEY         "PSA_UDP_PASSIVE_KEY_"

/**
 * Name of environment variable with space-separated list of ips/urls to connect to
 * e.g. PSA_UDP_STATIC_CONNECT_FOR_topic_scope="udp://127.0.0.1:4444 udp://127.0.0.2:4444"
 */
#define PUBSUB_UDP_STATIC_CONNECT_URLS_FOR       "PSA_UDP_STATIC_CONNECT_URL_FOR_"

/**
 * Realtime thread prio and scheduling information. This is used to setup the thread prio/sched of the
 * internal UDP threads.
 * Can be set in the topic properties.
 */
#define PUBSUB_UDP_THREAD_REALTIME_PRIO         "thread.realtime.prio"
#define PUBSUB_UDP_THREAD_REALTIME_SCHED        "thread.realtime.sched"

#endif /* PUBSUB_PSA_UDP_CONSTANTS_H_ */
