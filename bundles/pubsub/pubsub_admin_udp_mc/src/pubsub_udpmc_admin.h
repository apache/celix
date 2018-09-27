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

#ifndef CELIX_PUBSUB_UDPMC_ADMIN_H
#define CELIX_PUBSUB_UDPMC_ADMIN_H

#include "celix_api.h"
#include "log_helper.h"

#define PUBSUB_UDPMC_ADMIN_TYPE                     "udp_mc"
#define PUBSUB_PSA_UDPMC_SOCKET_ADDRESS_KEY			"udpmc.socket_address"
#define PUBSUB_PSA_UDPMC_SOCKET_PORT_KEY            "udpmc.socker_port"

#define PUBSUB_UDPMC_IP_KEY 	                    "PSA_IP"
#define PUBSUB_UDPMC_ITF_KEY	                    "PSA_INTERFACE"
#define PUBSUB_UDPMC_MULTICAST_IP_PREFIX_KEY        "PSA_MC_PREFIX"
#define PUBSUB_UDPMC_VERBOSE_KEY                    "PSA_UDPMC_VERBOSE"

#define PUBSUB_UDPMC_MULTICAST_IP_PREFIX_DEFAULT    "224.100"
#define PUBSUB_UDPMC_VERBOSE_DEFAULT                true

typedef struct pubsub_udpmc_admin pubsub_udpmc_admin_t;

pubsub_udpmc_admin_t* pubsub_udpmcAdmin_create(celix_bundle_context_t *ctx, log_helper_t *logHelper);
void pubsub_udpmcAdmin_destroy(pubsub_udpmc_admin_t *psa);

celix_status_t pubsub_udpmcAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter, double *score, long *serializerSvcId);
celix_status_t pubsub_udpmcAdmin_matchSubscriber(void *handle, long svcProviderBndId, const celix_properties_t *svcProperties, double *score, long *serializerSvcId);
celix_status_t pubsub_udpmcAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, bool *match);

celix_status_t pubsub_udpmcAdmin_setupTopicSender(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **publisherEndpoint);
celix_status_t pubsub_udpmcAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_udpmcAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic, long serializerSvcId, celix_properties_t **subscriberEndpoint);
celix_status_t pubsub_udpmcAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic);

void pubsub_udpmcAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_udpmcAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props);

celix_status_t pubsub_udpmcAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_udpmcAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint);

celix_status_t pubsub_udpmcAdmin_executeCommand(void *handle, char *commandLine, FILE *outStream, FILE *errStream);

#endif //CELIX_PUBSUB_UDPMC_ADMIN_H

