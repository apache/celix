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

typedef struct topic_publication *topic_publication_pt;

celix_status_t pubsub_topicPublicationCreate(bundle_context_pt bundle_context,pubsub_endpoint_pt pubEP, pubsub_serializer_service_t *best_serializer, const char *serType, char* bindIP, unsigned int basePort, unsigned int maxPort, topic_publication_pt *out);
celix_status_t pubsub_topicPublicationDestroy(topic_publication_pt pub);

celix_status_t pubsub_topicPublicationAddPublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep);
celix_status_t pubsub_topicPublicationRemovePublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep);

celix_status_t pubsub_topicPublicationStart(bundle_context_pt bundle_context,topic_publication_pt pub,service_factory_pt* svcFactory);
celix_status_t pubsub_topicPublicationStop(topic_publication_pt pub);

array_list_pt pubsub_topicPublicationGetPublisherList(topic_publication_pt pub);

#endif /* TOPIC_PUBLICATION_H_ */
