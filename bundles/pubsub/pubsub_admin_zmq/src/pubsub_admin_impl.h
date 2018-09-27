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

#ifndef PUBSUB_ADMIN_ZMQ_IMPL_H_
#define PUBSUB_ADMIN_ZMQ_IMPL_H_

#include <czmq.h>
/* The following undefs prevent the collision between:
 * - sys/syslog.h (which is included within czmq)
 * - celix/dfi/dfi_log_util.h
 */
#undef LOG_DEBUG
#undef LOG_WARNING
#undef LOG_INFO
#undef LOG_WARNING

#include "pubsub_psa_zmq_constants.h"
#include "pubsub_admin.h"
#include "pubsub_utils.h"
#include "log_helper.h"
#include "command.h"

#define PUBSUB_PSA_ZMQ_PSA_TYPE		"zmq"
#define PUBSUB_PSA_ZMQ_IP 			"PSA_IP"
#define PUBSUB_PSA_ZMQ_ITF			"PSA_INTERFACE"


typedef struct pubsub_admin {

	bundle_context_pt bundle_context;
	log_helper_pt loghelper;

	/* List of the available serializers */
	celix_thread_mutex_t serializerListLock;
	array_list_pt serializerList; // List<serializers service references>

	celix_thread_mutex_t localPublicationsLock;
	hash_map_pt localPublications;//<topic(string),service_factory_pt>

	celix_thread_mutex_t externalPublicationsLock;
	hash_map_pt externalPublications;//<topic(string),List<pubsub_ep>>

	celix_thread_mutex_t subscriptionsLock;
	hash_map_pt subscriptions; //<topic(string),topic_subscription>

	celix_thread_mutex_t pendingSubscriptionsLock;
	celix_thread_mutexattr_t pendingSubscriptionsAttr;
	hash_map_pt pendingSubscriptions; //<topic(string),List<pubsub_ep>>

	/* Those are used to keep track of valid subscriptions/publications that still have no valid serializer */
	celix_thread_mutex_t noSerializerPendingsLock;
	celix_thread_mutexattr_t noSerializerPendingsAttr;
	array_list_pt noSerializerSubscriptions; // List<pubsub_ep>
	array_list_pt noSerializerPublications; // List<pubsub_ep>

	celix_thread_mutex_t usedSerializersLock;
	hash_map_pt topicSubscriptionsPerSerializer; // <serializer,List<topicSubscription>>
	hash_map_pt topicPublicationsPerSerializer; // <serializer,List<topicPublications>>

	char* ipAddress;

	zactor_t* zmq_auth;

    unsigned int basePort;
    unsigned int maxPort;

	double qosSampleScore;
	double qosControlScore;
	double defaultScore;

	bool verbose;
} pubsub_admin_t;

celix_status_t pubsubAdmin_create(bundle_context_pt context, pubsub_admin_t **admin);
celix_status_t pubsubAdmin_destroy(pubsub_admin_t *admin);

void pubsubAdmin_addSerializer(void * handle, void *svc, const celix_properties_t *properties);
void pubsubAdmin_removeSerializer(void * handle, void *svc, const celix_properties_t *properties);



//for the pubsub_admin_service

celix_status_t pubsubAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, double *score, long *serializerSvcId);
celix_status_t pubsubAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, double *score, long *serializerSvcId);
celix_status_t pubsubAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, double *score);

//note endpoint is owned by caller
celix_status_t pubsubAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **publisherEndpoint);
celix_status_t pubsubAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

//note endpoint is owned by caller
celix_status_t pubsubAdmin_setupTopicReciever(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **subscriberEndpoint);
celix_status_t pubsubAdmin_teardownTopicReciever(void *handle, const char *scope, const char *topic);

celix_status_t pubsubAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsubAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint);


#endif /* PUBSUB_ADMIN_ZMQ_IMPL_H_ */
