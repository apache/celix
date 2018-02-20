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
 * topic_publication.h
 *
 *  \date       Sep 24, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TOPIC_PUBLICATION_H_
#define TOPIC_PUBLICATION_H_

#include "pubsub/publisher.h"
#include "pubsub_endpoint.h"
#include "pubsub_common.h"

#include "pubsub_serializer.h"

#define UDP_BASE_PORT	49152
#define UDP_MAX_PORT	65000

typedef struct pubsub_udp_msg {
    struct pubsub_msg_header header;
    unsigned int payloadSize;
    char payload[];
} pubsub_udp_msg_t;

typedef struct topic_publication *topic_publication_pt;
celix_status_t pubsub_topicPublicationCreate(int sendSocket, pubsub_endpoint_pt pubEP, pubsub_serializer_service_t *best_serializer, const char* best_serializer_type, char* bindIP, topic_publication_pt *out);
celix_status_t pubsub_topicPublicationDestroy(topic_publication_pt pub);

celix_status_t pubsub_topicPublicationAddPublisherEP(topic_publication_pt pub, pubsub_endpoint_pt ep);
celix_status_t pubsub_topicPublicationRemovePublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep);

celix_status_t pubsub_topicPublicationStart(bundle_context_pt bundle_context,topic_publication_pt pub,service_factory_pt* svcFactory);
celix_status_t pubsub_topicPublicationStop(topic_publication_pt pub);

array_list_pt pubsub_topicPublicationGetPublisherList(topic_publication_pt pub);

#endif /* TOPIC_PUBLICATION_H_ */
