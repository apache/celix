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
/*
 * pubsub_admin.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_ADMIN_H_
#define PUBSUB_ADMIN_H_

#include "service_reference.h"

#include "pubsub_common.h"
#include "pubsub_endpoint.h"

#define PSA_IP 	"PSA_IP"
#define PSA_ITF	"PSA_INTERFACE"
#define PSA_MULTICAST_IP_PREFIX "PSA_MC_PREFIX"

#define PUBSUB_ADMIN_TYPE_KEY	"pubsub_admin.type"

typedef struct pubsub_admin *pubsub_admin_pt;

struct pubsub_admin_service {
	pubsub_admin_pt admin;

	celix_status_t (*addSubscription)(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
	celix_status_t (*removeSubscription)(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);

	celix_status_t (*addPublication)(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
	celix_status_t (*removePublication)(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);

	celix_status_t (*closeAllPublications)(pubsub_admin_pt admin,char* scope, char* topic);
	celix_status_t (*closeAllSubscriptions)(pubsub_admin_pt admin,char* scope, char* topic);

	/* Match principle:
	 * - A full matching pubsub_admin gives 200 points
	 * - A full matching serializer gives 100 points
	 * - If QoS = sample
	 * 		- fallback pubsub_admin order of selection is: udp_mc, zmq. Points allocation is 100,75.
	 * 		- fallback serializers order of selection is: json, void. Points allocation is 30,20.
	 * - If QoS = control
	 * 		- fallback pubsub_admin order of selection is: zmq,udp_mc. Points allocation is 100,75.
	 * 		- fallback serializers order of selection is: json, void. Points allocation is 30,20.
	 * - If nothing is specified, QoS = sample is assumed, so the same score applies, just divided by two.
	 *
	 */
	celix_status_t (*matchEndpoint)(pubsub_admin_pt admin, pubsub_endpoint_pt endpoint, double* score);
};

typedef struct pubsub_admin_service *pubsub_admin_service_pt;

#endif /* PUBSUB_ADMIN_H_ */
