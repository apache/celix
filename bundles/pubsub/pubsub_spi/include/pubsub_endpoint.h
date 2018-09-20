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
#define PUBSUB_ENDPOINT_ADMIN_TYPE       PUBSUB_ADMIN_TYPE_KEY
#define PUBSUB_ENDPOINT_SERIALIZER       PUBSUB_SERIALIZER_TYPE_KEY


#define PUBSUB_PUBLISHER_ENDPOINT_TYPE  "pubsub.publisher"
#define PUBSUB_SUBSCRIBER_ENDPOINT_TYPE "pubsub.subscriber"


struct pubsub_endpoint {
    const char *topicName;
    const char *topicScope;

    const char *uuid;
    const char *frameworkUUid;
    const char *type;
    const char *adminType;
    const char *serializerType;

    celix_properties_t *properties;
};

typedef struct pubsub_endpoint *pubsub_endpoint_pt;
typedef struct pubsub_endpoint pubsub_endpoint_t;

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, const char* pubsubType, const char* adminType, const char *serType, celix_properties_t *topic_props, pubsub_endpoint_pt* psEp);
celix_status_t pubsubEndpoint_createFromProperties(const celix_properties_t *props, pubsub_endpoint_t **out);
celix_status_t pubsubEndpoint_createFromSvc(bundle_context_t* ctx, const celix_bundle_t *bnd, const celix_properties_t *svcProps, bool isPublisher, pubsub_endpoint_pt* out);
celix_status_t pubsubEndpoint_createFromListenerHookInfo(bundle_context_t *ctx, const celix_service_tracker_info_t *info, bool isPublisher, pubsub_endpoint_pt* out);
celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out);

void pubsubEndpoint_destroy(pubsub_endpoint_pt psEp);
bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2);
bool pubsubEndpoint_equalsWithProperties(pubsub_endpoint_pt psEp1, const celix_properties_t *props);

void pubsubEndpoint_setField(pubsub_endpoint_t *ep, const char *key, const char *val);

//check if the required properties are available for the endpoint
bool pubsubEndpoint_isValid(const celix_properties_t *endpointProperties, bool requireAdminType, bool requireSerializerType);


char * pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic);

#endif /* PUBSUB_ENDPOINT_H_ */
