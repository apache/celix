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
 * topic_subscription.h
 *
 *  \date       Sep 22, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TOPIC_SUBSCRIPTION_H_
#define TOPIC_SUBSCRIPTION_H_

#include "celix_threads.h"
#include "array_list.h"
#include "celixbool.h"
#include "service_tracker.h"

#include "pubsub_endpoint.h"
#include "pubsub_common.h"
#include "pubsub_serializer.h"

typedef struct topic_subscription* topic_subscription_pt;

celix_status_t pubsub_topicSubscriptionCreate(bundle_context_pt bundle_context, char* ifIp,char* scope, char* topic ,pubsub_serializer_service_t *best_serializer, topic_subscription_pt* out);
celix_status_t pubsub_topicSubscriptionDestroy(topic_subscription_pt ts);
celix_status_t pubsub_topicSubscriptionStart(topic_subscription_pt ts);
celix_status_t pubsub_topicSubscriptionStop(topic_subscription_pt ts);

celix_status_t pubsub_topicSubscriptionAddConnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL);
celix_status_t pubsub_topicSubscriptionAddDisconnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL);

celix_status_t pubsub_topicSubscriptionConnectPublisher(topic_subscription_pt ts, char* pubURL);
celix_status_t pubsub_topicSubscriptionDisconnectPublisher(topic_subscription_pt ts, char* pubURL);

celix_status_t pubsub_topicSubscriptionAddSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP);
celix_status_t pubsub_topicSubscriptionRemoveSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP);

array_list_pt pubsub_topicSubscriptionGetSubscribersList(topic_subscription_pt sub);
celix_status_t pubsub_topicIncreaseNrSubscribers(topic_subscription_pt subscription);
celix_status_t pubsub_topicDecreaseNrSubscribers(topic_subscription_pt subscription);
unsigned int pubsub_topicGetNrSubscribers(topic_subscription_pt subscription);

#endif /*TOPIC_SUBSCRIPTION_H_ */
