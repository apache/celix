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
 * pubsub_admin_impl.h
 *
 *  \date       Dec 5, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_ADMIN_IMPL_H_
#define PUBSUB_ADMIN_IMPL_H_

#include "pubsub_admin.h"
#include "pubsub_serializer.h"
#include "log_helper.h"

struct pubsub_admin {

	pubsub_serializer_service_t* serializerSvc;

	bundle_context_pt bundle_context;
	log_helper_pt loghelper;

	celix_thread_mutex_t localPublicationsLock;
	hash_map_pt localPublications;//<topic(string),service_factory_pt>

	celix_thread_mutex_t externalPublicationsLock;
	hash_map_pt externalPublications;//<topic(string),List<pubsub_ep>>

	celix_thread_mutex_t subscriptionsLock;
	hash_map_pt subscriptions; //<topic(string),topic_subscription>

	celix_thread_mutex_t pendingSubscriptionsLock;
	hash_map_pt pendingSubscriptions; //<topic(string),List<pubsub_ep>>

	char* ifIpAddress; // The local interface which is used for multicast communication
    char* mcIpAddress; // The multicast IP address

	int sendSocket;

};

celix_status_t pubsubAdmin_create(bundle_context_pt context, pubsub_admin_pt *admin);
celix_status_t pubsubAdmin_stop(pubsub_admin_pt admin);
celix_status_t pubsubAdmin_destroy(pubsub_admin_pt admin);

celix_status_t pubsubAdmin_addSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
celix_status_t pubsubAdmin_removeSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);

celix_status_t pubsubAdmin_addPublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP);
celix_status_t pubsubAdmin_removePublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP);

celix_status_t pubsubAdmin_closeAllPublications(pubsub_admin_pt admin,char* scope, char* topic);
celix_status_t pubsubAdmin_closeAllSubscriptions(pubsub_admin_pt admin,char* scope, char* topic);

celix_status_t pubsubAdmin_matchPublisher(pubsub_admin_pt admin, pubsub_endpoint_pt pubEP, double* score);
celix_status_t pubsubAdmin_matchSubscriber(pubsub_admin_pt admin, pubsub_endpoint_pt subEP, double* score);

celix_status_t pubsubAdmin_setSerializer(pubsub_admin_pt admin, pubsub_serializer_service_t* serializerSvc);
celix_status_t pubsubAdmin_removeSerializer(pubsub_admin_pt admin, pubsub_serializer_service_t* serializerSvc);

#endif /* PUBSUB_ADMIN_IMPL_H_ */
