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

#ifndef CELIX_PUBSUB_UDPMC_ADMIN_H
#define CELIX_PUBSUB_UDPMC_ADMIN_H

#include "celix_api.h"
#include "celix_log_helper.h"
#include "pubsub_psa_udpmc_constants.h"

typedef struct pubsub_udpmc_admin pubsub_udpmc_admin_t;

pubsub_udpmc_admin_t* pubsub_udpmcAdmin_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper);
void pubsub_udpmcAdmin_destroy(pubsub_udpmc_admin_t *psa);

celix_status_t pubsub_udpmcAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, celix_properties_t **topicProperties, double *score, long *serializerSvcId, long *protocolSvcId);
celix_status_t pubsub_udpmcAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, celix_properties_t **topicProperties, double *score, long *serializerSvcId, long *protocolSvcId);
celix_status_t pubsub_udpmcAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, bool *match);

celix_status_t pubsub_udpmcAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **publisherEndpoint);
celix_status_t pubsub_udpmcAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_udpmcAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId, celix_properties_t **subscriberEndpoint);
celix_status_t pubsub_udpmcAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic);

void pubsub_udpmcAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_udpmcAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props);

celix_status_t pubsub_udpmcAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_udpmcAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint);

bool pubsub_udpmcAdmin_executeCommand(void *handle, const char *commandLine, FILE *outStream, FILE *errStream);

#endif //CELIX_PUBSUB_UDPMC_ADMIN_H

