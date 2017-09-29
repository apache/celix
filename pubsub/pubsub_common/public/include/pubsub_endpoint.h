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

#include "publisher.h"
#include "subscriber.h"

struct pubsub_endpoint {
    char *frameworkUUID;
    char *scope;
    char *topic;
    long serviceID;
    char* endpoint;
    bool is_secure;
    properties_pt topic_props;
};

typedef struct pubsub_endpoint *pubsub_endpoint_pt;

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props,pubsub_endpoint_pt* psEp);
celix_status_t pubsubEndpoint_createFromServiceReference(service_reference_pt reference,pubsub_endpoint_pt* psEp, bool isPublisher);
celix_status_t pubsubEndpoint_createFromListenerHookInfo(listener_hook_info_pt info,pubsub_endpoint_pt* psEp, bool isPublisher);
celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out);
celix_status_t pubsubEndpoint_destroy(pubsub_endpoint_pt psEp);
bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2);

char *createScopeTopicKey(const char* scope, const char* topic);

#endif /* PUBSUB_ENDPOINT_H_ */
