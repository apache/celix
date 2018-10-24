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

#ifndef CELIX_PUBSUB_ZMQ_ADMIN_H
#define CELIX_PUBSUB_ZMQ_ADMIN_H

#include "celix_api.h"
#include "log_helper.h"

#define PUBSUB_NANOMSG_ADMIN_TYPE       "zmq"
#define PUBSUB_NANOMSG_URL_KEY          "zmq.url"

#define PUBSUB_NANOMSG_VERBOSE_KEY      "PSA_ZMQ_VERBOSE"
#define PUBSUB_NANOMSG_VERBOSE_DEFAULT  true

#define PUBSUB_NANOMSG_PSA_IP_KEY       "PSA_IP"
#define PUBSUB_NANOMSG_PSA_ITF_KEY		"PSA_INTERFACE"
#define PUBSUB_NANOMSG_NR_THREADS_KEY   "PSA_ZMQ_NR_THREADS"

#define PUBSUB_NANOMSG_DEFAULT_IP       "127.0.0.1"

typedef struct pubsub_nanomsg_admin pubsub_nanomsg_admin_t;

pubsub_nanomsg_admin_t* pubsub_nanoMsgAdmin_create(celix_bundle_context_t *ctx, log_helper_t *logHelper);
void pubsub_nanoMsgAdmin_destroy(pubsub_nanomsg_admin_t *psa);

#ifdef __cplusplus
extern "C" {
#endif
celix_status_t pubsub_nanoMsgAdmin_matchPublisher(void *handle, long svcRequesterBndId, const celix_filter_t *svcFilter,
                                                  double *score, long *serializerSvcId);
                                                  celix_status_t pubsub_nanoMsgAdmin_matchSubscriber(void *handle, long svcProviderBndId,
                                                   const celix_properties_t *svcProperties, double *score,
                                                   long *serializerSvcId);
celix_status_t pubsub_nanoMsgAdmin_matchEndpoint(void *handle, const celix_properties_t *endpoint, bool *match);

celix_status_t pubsub_nanoMsgAdmin_setupTopicSender(void *handle, const char *scope, const char *topic,
                                                    long serializerSvcId, celix_properties_t **publisherEndpoint);
celix_status_t pubsub_nanoMsgAdmin_teardownTopicSender(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_nanoMsgAdmin_setupTopicReceiver(void *handle, const char *scope, const char *topic,
                                                      long serializerSvcId, celix_properties_t **subscriberEndpoint);
celix_status_t pubsub_nanoMsgAdmin_teardownTopicReceiver(void *handle, const char *scope, const char *topic);

celix_status_t pubsub_nanoMsgAdmin_addEndpoint(void *handle, const celix_properties_t *endpoint);
celix_status_t pubsub_nanoMsgAdmin_removeEndpoint(void *handle, const celix_properties_t *endpoint);

void pubsub_nanoMsgAdmin_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
void pubsub_nanoMsgAdmin_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props);
#ifdef __cplusplus
}
#endif

celix_status_t pubsub_nanoMsgAdmin_executeCommand(void *handle, char *commandLine, FILE *outStream, FILE *errStream);

#endif //CELIX_PUBSUB_ZMQ_ADMIN_H

