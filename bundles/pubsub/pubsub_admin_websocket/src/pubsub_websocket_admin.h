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

#ifndef CELIX_PUBSUB_WEBSOCKET_ADMIN_H
#define CELIX_PUBSUB_WEBSOCKET_ADMIN_H

#include <stdint.h>
#include <pubsub_admin_metrics.h>
#include "celix_api.h"
#include "celix_log_helper.h"
#include "pubsub_psa_websocket_constants.h"
#include <pubsub_message_serialization_service.h>

typedef struct pubsub_websocket_admin pubsub_websocket_admin_t;

typedef struct psa_websocket_serializer_entry {
    const char *fqn;
    const char *version;
    pubsub_message_serialization_service_t *svc;
} psa_websocket_serializer_entry_t;

pubsub_websocket_admin_t* pubsub_websocketAdmin_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper);
void pubsub_websocketAdmin_destroy(pubsub_websocket_admin_t *psa);

celix_status_t pubsub_websocketAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, celix_properties_t **topicProperties, double *score, long *serializerSvcId, long *protocolSvcId);
celix_status_t pubsub_websocketAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, celix_properties_t **topicProperties, double *score, long *serializerSvcId, long *protocolSvcId);
celix_status_t pubsub_websocketAdmin_matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *match);

celix_status_t pubsub_websocketAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, const celix_properties_t* topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **publisherEndpoint);
celix_status_t pubsub_websocketAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_websocketAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, const celix_properties_t* topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **subscriberEndpoint);
celix_status_t pubsub_websocketAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_websocketAdmin_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_websocketAdmin_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint);

void pubsub_websocketAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_websocketAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props);

psa_websocket_serializer_entry_t* pubsub_websocketAdmin_acquireSerializerForMessageId(void *handle, const char *serializationType, uint32_t msgId);
void pubsub_websocketAdmin_releaseSerializer(void *handle, psa_websocket_serializer_entry_t* serializer);
int64_t pubsub_websocketAdmin_getMessageIdForMessageFqn(void *handle, const char *serializationType, const char *fqn);

bool pubsub_websocketAdmin_executeCommand(void *handle, const char *commandLine, FILE *outStream, FILE *errStream);

#endif //CELIX_PUBSUB_WEBSOCKET_ADMIN_H

