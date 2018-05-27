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

#ifndef PUBLISHER_ENDPOINT_ANNOUNCE_H_
#define PUBLISHER_ENDPOINT_ANNOUNCE_H_

#include "pubsub_endpoint.h"


//TODO refactor to pubsub_endpoint_announce
//can be used to announce and remove publisher and subscriber endpoints

struct publisher_endpoint_announce {
	void *handle;
	celix_status_t (*announcePublisher)(void *handle, pubsub_endpoint_pt pubEP);
	celix_status_t (*removePublisher)(void *handle, pubsub_endpoint_pt pubEP);
	celix_status_t (*interestedInTopic)(void* handle, const char *scope, const char *topic);
	celix_status_t (*uninterestedInTopic)(void* handle, const char *scope, const char *topic);
};

typedef struct publisher_endpoint_announce *publisher_endpoint_announce_pt;


#endif /* PUBLISHER_ENDPOINT_ANNOUNCE_H_ */
