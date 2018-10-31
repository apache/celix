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

#ifndef PUBSUB_ENDPOINT_H_
#define PUBSUB_ENDPOINT_H_

#include "celix_bundle_context.h"
#include "celix_properties.h"

#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

#include "pubsub_constants.h"

//required for valid endpoint
#define PUBSUB_ENDPOINT_TOPIC_NAME      "pubsub.topic.name"
#define PUBSUB_ENDPOINT_TOPIC_SCOPE     "pubsub.topic.scope"

#define PUBSUB_ENDPOINT_UUID            "pubsub.endpoint.uuid" //required
#define PUBSUB_ENDPOINT_FRAMEWORK_UUID  "pubsub.framework.uuid" //required
#define PUBSUB_ENDPOINT_TYPE            "pubsub.endpoint.type" //PUBSUB_PUBLISHER_ENDPOINT_TYPE or PUBSUB_SUBSCRIBER_ENDPOINT_TYPE
#define PUBSUB_ENDPOINT_VISIBILITY      "pubsub.endpoint.visiblity" //local, host or system. e.g. for IPC host
#define PUBSUB_ENDPOINT_ADMIN_TYPE       PUBSUB_ADMIN_TYPE_KEY
#define PUBSUB_ENDPOINT_SERIALIZER       PUBSUB_SERIALIZER_TYPE_KEY


#define PUBSUB_PUBLISHER_ENDPOINT_TYPE      "pubsub.publisher"
#define PUBSUB_SUBSCRIBER_ENDPOINT_TYPE     "pubsub.subscriber"
#define PUBSUB_ENDPOINT_VISIBILITY_DEFAULT  PUBSUB_ENDPOINT_SYSTEM_VISIBLITY


celix_properties_t* pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, const char* pubsubType, const char* adminType, const char *serType, celix_properties_t *topic_props);
celix_properties_t* pubsubEndpoint_createFromSubscriberSvc(bundle_context_t* ctx, long svcBndId, const celix_properties_t *svcProps);
celix_properties_t* pubsubEndpoint_createFromPublisherTrackerInfo(bundle_context_t *ctx, long bundleId, const char *filter);

bool pubsubEndpoint_equals(const celix_properties_t *psEp1, const celix_properties_t *psEp2);

//check if the required properties are available for the endpoint
bool pubsubEndpoint_isValid(const celix_properties_t *endpointProperties, bool requireAdminType, bool requireSerializerType);


char * pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic);

#endif /* PUBSUB_ENDPOINT_H_ */
