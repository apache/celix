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

#ifndef CELIX_PUBSUB_TCP_ADMIN_H
#define CELIX_PUBSUB_TCP_ADMIN_H

#include <pubsub_admin_metrics.h>
#include <pubsub_message_serialization_service.h>
#include <stdint.h>
#include "celix_api.h"
#include "celix_log_helper.h"
#include "pubsub_psa_tcp_constants.h"

typedef struct pubsub_tcp_admin pubsub_tcp_admin_t;

typedef struct psa_tcp_serializer_entry {
    const char *fqn;
    const char *version;
    pubsub_message_serialization_service_t *svc;
} psa_tcp_serializer_entry_t;

pubsub_tcp_admin_t *pubsub_tcpAdmin_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper);
void pubsub_tcpAdmin_destroy(pubsub_tcp_admin_t *psa);

celix_status_t pubsub_tcpAdmin_matchPublisher(void *handle,
                                              long svcRequesterBndId,
                                              const celix_filter_t *svcFilter,
                                              celix_properties_t **topicProperties,
                                              double *score,
                                              long *serializerSvcId,
                                              long *protocolSvcId);
celix_status_t pubsub_tcpAdmin_matchSubscriber(void *handle,
                                               long svcProviderBndId,
                                               const celix_properties_t *svcProperties,
                                               celix_properties_t **topicProperties,
                                               double *score,
                                               long *serializerSvcId,
                                               long *protocolSvcId);
celix_status_t pubsub_tcpAdmin_matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *match);

celix_status_t pubsub_tcpAdmin_setupTopicSender(void *handle,
                                                const char *scope,
                                                const char *topic,
                                                const celix_properties_t *topicProperties,
                                                long serializerSvcId,
                                                long protocolSvcId,
                                                celix_properties_t **publisherEndpoint);
celix_status_t pubsub_tcpAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_tcpAdmin_setupTopicReceiver(void *handle,
                                                  const char *scope,
                                                  const char *topic,
                                                  const celix_properties_t *topicProperties,
                                                  long serializerSvcId,
                                                  long protocolSvcId,
                                                  celix_properties_t **subscriberEndpoint);
celix_status_t pubsub_tcpAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_tcpAdmin_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_tcpAdmin_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint);

void pubsub_tcpAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_tcpAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props);

void pubsub_tcpAdmin_addProtocolSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_tcpAdmin_removeProtocolSvc(void *handle, void *svc, const celix_properties_t *props);
bool pubsub_tcpAdmin_executeCommand(void *handle, const char *commandLine, FILE *outStream, FILE *errStream);

pubsub_admin_metrics_t *pubsub_tcpAdmin_metrics(void *handle);

#endif //CELIX_PUBSUB_TCP_ADMIN_H

