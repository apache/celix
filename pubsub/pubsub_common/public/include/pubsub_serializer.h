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
 * pubsub_serializer.h
 *
 *  \date       Mar 24, 2017
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_SERIALIZER_H_
#define PUBSUB_SERIALIZER_H_

#include "service_reference.h"

#include "pubsub_common.h"

typedef struct _pubsub_message_type pubsub_message_type;

typedef struct pubsub_serializer *pubsub_serializer_pt;

struct pubsub_serializer_service {

	pubsub_serializer_pt serializer;

	celix_status_t (*serialize)(pubsub_serializer_pt serializer, pubsub_message_type *msgType, const void *input, void **output, int *outputLen);
	celix_status_t (*deserialize)(pubsub_serializer_pt serializer, pubsub_message_type *msgType, const void *input, void **output);

	void (*fillMsgTypesMap)(pubsub_serializer_pt serializer, hash_map_pt msgTypesMap,bundle_pt bundle);
	void (*emptyMsgTypesMap)(pubsub_serializer_pt serializer, hash_map_pt msgTypesMap);

	version_pt (*getVersion)(pubsub_serializer_pt serializer, pubsub_message_type *msgType);
	char* (*getName)(pubsub_serializer_pt serializer, pubsub_message_type *msgType);
	void (*freeMsg)(pubsub_serializer_pt serializer, pubsub_message_type *msgType, void *msg);

};

typedef struct pubsub_serializer_service *pubsub_serializer_service_pt;

#endif /* PUBSUB_SERIALIZER_H_ */
