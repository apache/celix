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
 * pubsub_endpoint.h
 *
 *  \date       Sep 21, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_ENDPOINT_H_
#define PUBSUB_ENDPOINT_H_

#include "service_reference.h"
#include "listener_hook_service.h"
#include "properties.h"

#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

#include "pubsub_constants.h"

//required for valid endpoint
#define PUBSUB_ENDPOINT_UUID            "pubsub.endpoint.uuid" //required
#define PUBSUB_ENDPOINT_FRAMEWORK_UUID  "pubsub.framework.uuid" //required
#define PUBSUB_ENDPOINT_TYPE            "pubsub.endpoint.type" //PUBSUB_PUBLISHER_ENDPOINT_TYPE or PUBSUB_SUBSCRIBER_ENDPOINT_TYPE
#define PUBSUB_ENDPOINT_ADMIN_TYPE       PUBSUB_ADMIN_TYPE_KEY
#define PUBSUB_ENDPOINT_SERIALIZER       PUBSUB_SERIALIZER_TYPE_KEY
#define PUBSUB_ENDPOINT_TOPIC_NAME      "pubsub.topic.name"
#define PUBSUB_ENDPOINT_TOPIC_SCOPE     "pubsub.topic.scope"

//optional
#define PUBSUB_ENDPOINT_SERVICE_ID      "pubsub.service.id"
#define PUBSUB_ENDPOINT_BUNDLE_ID       "pubsub.bundle.id"
#define PUBSUB_ENDPOINT_URL             "pubsub.url"


#define PUBSUB_PUBLISHER_ENDPOINT_TYPE  "pubsub.publisher"
#define PUBSUB_SUBSCRIBER_ENDPOINT_TYPE "pubsub.subscriber"


struct pubsub_endpoint {
    properties_pt endpoint_props;
    properties_pt topic_props;
};

typedef struct pubsub_endpoint *pubsub_endpoint_pt;

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long bundleId, long serviceId, const char* endpoint, const char* pubsubType, properties_pt topic_props, pubsub_endpoint_pt* psEp);
celix_status_t pubsubEndpoint_createFromServiceReference(bundle_context_t* ctx, service_reference_pt reference, bool isPublisher, pubsub_endpoint_pt* out);
celix_status_t pubsubEndpoint_createFromListenerHookInfo(bundle_context_t* ctx, listener_hook_info_pt info, bool isPublisher, pubsub_endpoint_pt* out);
celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out);
void pubsubEndpoint_destroy(pubsub_endpoint_pt psEp);
bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2);
celix_status_t pubsubEndpoint_setField(pubsub_endpoint_pt ep, const char* key, const char* value);

/**
 * Creates a pubsub_endpoint based on discovered properties.
 * Will take ownership over the discovredProperties
 */
celix_status_t pubsubEndpoint_createFromDiscoveredProperties(properties_t *discoveredProperties, pubsub_endpoint_pt* out);

char * pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic);

#endif /* PUBSUB_ENDPOINT_H_ */
