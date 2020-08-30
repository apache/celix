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

#ifndef PUBSUB_PSA_UDPMC_CONSTANTS_H_
#define PUBSUB_PSA_UDPMC_CONSTANTS_H_


#define PSA_UDPMC_PUBSUB_ADMIN_TYPE                 "udp_mc"

#define PSA_UDPMC_DEFAULT_QOS_SAMPLE_SCORE          70
#define PSA_UDPMC_DEFAULT_QOS_CONTROL_SCORE         30
#define PSA_UDPMC_DEFAULT_SCORE                     50

#define PSA_UDPMC_QOS_SAMPLE_SCORE_KEY              "PSA_UDPMC_QOS_SAMPLE_SCORE"
#define PSA_UDPMC_QOS_CONTROL_SCORE_KEY             "PSA_UDPMC_QOS_CONTROL_SCORE"
#define PSA_UDPMC_DEFAULT_SCORE_KEY                 "PSA_UDPMC_DEFAULT_SCORE"


#define PUBSUB_UDPMC_ADMIN_TYPE                     "udp_mc"
#define PUBSUB_UDPMC_SOCKET_ADDRESS_KEY             "udpmc.socket_address"
#define PUBSUB_UDPMC_SOCKET_PORT_KEY                "udpmc.socket_port"

#define PUBSUB_UDPMC_IP_KEY                         "PSA_IP"
#define PUBSUB_UDPMC_ITF_KEY                        "PSA_INTERFACE"
#define PUBSUB_UDPMC_MULTICAST_IP_PREFIX_KEY        "PSA_MC_PREFIX"
#define PUBSUB_UDPMC_VERBOSE_KEY                    "PSA_UDPMC_VERBOSE"

#define PUBSUB_UDPMC_MULTICAST_IP_PREFIX_DEFAULT    "224.100"
#define PUBSUB_UDPMC_MULTICAST_IP_DEFAULT           "224.100.1.1"
#define PUBSUB_UDPMC_VERBOSE_DEFAULT                true

/**
 * If set true on the endpoint, the udp mc TopicSender bind and/or discovery url is statically configured.
 */
#define PUBSUB_UDPMC_STATIC_CONFIGURED                  "updmc.static.configured"

/**
 * Name of environment variable with ip/url to bind to
 * e.g. PUBSUB_UDPMC_STATIC_BIND_PORT_URL_FOR_topic_scope="4444"
 */
#define PUBSUB_UDPMC_STATIC_BIND_PORT_URL_FOR "PSA_UDPMC_STATIC_BIND_PORT_FOR_"

/**
 * Can be set in the topic properties to fix a static mc port for topic senders
 */
#define PUBSUB_UDPMC_STATIC_BIND_PORT                   "udpmc.static.bind.port"

/**
 * The static url which a subscriber should try to connect to.
 * The urls are space separated
 */
#define PUBSUB_UDPMC_STATIC_CONNECT_SOCKET_ADDRESSES    "udpmc.static.connect.socket_addresses"

/**
 * Name of environment variable with space-separated list of ips/urls to connect to
 * e.g. PSA_UDPMC_STATIC_CONNECT_URLS_FOR_topic_scope="127.0.0.1:4444 127.0.0.2:4444"
 */
#define PUBSUB_UDPMC_STATIC_CONNECT_URLS_FOR "PSA_UDPMC_STATIC_CONNECT_URLS_FOR_"

#endif /* PUBSUB_PSA_UDPMC_CONSTANTS_H_ */
