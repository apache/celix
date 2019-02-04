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



#ifndef PUBSUB_PSA_ZMQ_CONSTANTS_H_
#define PUBSUB_PSA_ZMQ_CONSTANTS_H_


#define PSA_ZMQ_BASE_PORT                       "PSA_ZMQ_BASE_PORT"
#define PSA_ZMQ_MAX_PORT                        "PSA_ZMQ_MAX_PORT"

#define PSA_ZMQ_DEFAULT_BASE_PORT               5501
#define PSA_ZMQ_DEFAULT_MAX_PORT                6000

#define PSA_ZMQ_DEFAULT_QOS_SAMPLE_SCORE 	    30
#define PSA_ZMQ_DEFAULT_QOS_CONTROL_SCORE 	    70
#define PSA_ZMQ_DEFAULT_SCORE 				    30

#define PSA_ZMQ_QOS_SAMPLE_SCORE_KEY 		    "PSA_ZMQ_QOS_SAMPLE_SCORE"
#define PSA_ZMQ_QOS_CONTROL_SCORE_KEY 		    "PSA_ZMQ_QOS_CONTROL_SCORE"
#define PSA_ZMQ_DEFAULT_SCORE_KEY 			    "PSA_ZMQ_DEFAULT_SCORE"


#define PSA_ZMQ_METRICS_ENABLED "PSA_ZMQ_METRICS_ENABLED"
#define PSA_ZMQ_DEFAULT_METRICS_ENABLED true

#define PSA_ZMQ_ZEROCOPY_ENABLED "PSA_ZMQ_ZEROCOPY_ENABLED"
#define PSA_ZMQ_DEFAULT_ZEROCOPY_ENABLED false


#define PUBSUB_ZMQ_VERBOSE_KEY      "PSA_ZMQ_VERBOSE"
#define PUBSUB_ZMQ_VERBOSE_DEFAULT  true

#define PUBSUB_ZMQ_PSA_IP_KEY       "PSA_IP"
#define PUBSUB_ZMQ_PSA_ITF_KEY		"PSA_INTERFACE"
#define PUBSUB_ZMQ_NR_THREADS_KEY   "PSA_ZMQ_NR_THREADS"

#define PUBSUB_ZMQ_DEFAULT_IP       "127.0.0.1"

#define PUBSUB_ZMQ_ADMIN_TYPE       "zmq"

/**
 * The ZMQ url key for the topic sender endpoints
 */
#define PUBSUB_ZMQ_URL_KEY          "zmq.url"



/**
 * Can be set in the topic properties to fix a static bind url
 */
#define PUBSUB_ZMQ_STATIC_BIND_URL       "zmq.static.bind.url"

/**
 * Can be set in the topic properties to fix a static url used for discovery
 */
#define PUBSUB_ZMQ_STATIC_DISCOVER_URL       "zmq.static.bind.url"

/**
 * If set true on the endpoint, the zmq TopicSender bind and/or discovery url is statically configured.
 */
#define PUBSUB_ZMQ_STATIC_CONFIGURED       "zmq.static.configured"

/**
 * The static url which a subscriber should try to connect to.
 * The urls are space separated
 */
#define PUBSUB_ZMQ_STATIC_CONNECT_URLS    "zmq.static.connect.urls"

#endif /* PUBSUB_PSA_ZMQ_CONSTANTS_H_ */
