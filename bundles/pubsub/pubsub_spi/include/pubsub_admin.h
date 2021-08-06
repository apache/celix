/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef PUBSUB_ADMIN_H_
#define PUBSUB_ADMIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "celix_properties.h"
#include "celix_bundle.h"
#include "celix_filter.h"

#define PUBSUB_ADMIN_SERVICE_NAME       "pubsub_admin"
#define PUBSUB_ADMIN_SERVICE_VERSION    "3.0.0"
#define PUBSUB_ADMIN_SERVICE_RANGE      "[3,4)"

//expected service properties
#define PUBSUB_ADMIN_SERVICE_TYPE       "psa_type"

#define PUBSUB_ADMIN_FULL_MATCH_SCORE   100.0F
#define PUBSUB_ADMIN_NO_MATCH_SCORE     0.0F

struct pubsub_admin_service {
    void *handle;

    celix_status_t (*matchPublisher)(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, celix_properties_t **outTopicProperties, double *outScore, long *outSerializerSvcId, long *outProtocolSvcId);
    celix_status_t (*matchSubscriber)(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, celix_properties_t **outTopicProperties, double *outScore, long *outSerializerSvcId, long *outProtocolSvcId);
    celix_status_t (*matchDiscoveredEndpoint)(void *handle, const celix_properties_t *endpoint, bool *match);

    //note endpoint is owned by caller
    celix_status_t (*setupTopicSender)(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **publisherEndpoint);
    celix_status_t (*teardownTopicSender)(void *handle, const char *scope, const char *topic);

    //note endpoint is owned by caller
    celix_status_t (*setupTopicReceiver)(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **subscriberEndpoint);
    celix_status_t (*teardownTopicReceiver)(void *handle, const char *scope, const char *topic);

    celix_status_t (*addDiscoveredEndpoint)(void *handle, const celix_properties_t *endpoint);
    celix_status_t (*removeDiscoveredEndpoint)(void *handle, const celix_properties_t *endpoint);
};

typedef struct pubsub_admin_service pubsub_admin_service_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_ADMIN_H_ */


