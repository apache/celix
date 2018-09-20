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



#define PSA_ZMQ_PUBSUB_ADMIN_TYPE	            "zmq"

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

#define PSA_ZMQ_DEFAULT_VERBOSE 			    false
#define PSA_ZMQ_VERBOSE_KEY		 			    "PSA_ZMQ_VERBOSE"


#define PUBSUB_PSA_ZMQ_ENDPOINT_URL_KEY			"pubsub.zmq.url"


#endif /* PUBSUB_PSA_ZMQ_CONSTANTS_H_ */
