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
 * pubsub_serializer_impl.h
 *
 *  \date       Mar 24, 2017
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_SERIALIZER_IMPL_H_
#define PUBSUB_SERIALIZER_IMPL_H_

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_message.h"
#include "log_helper.h"

#include "pubsub_serializer.h"

struct _pubsub_message_type {	/* _dyn_message_type */
	struct namvals_head header;
	struct namvals_head annotations;
	struct types_head types;
	dyn_type *msgType;
	version_pt msgVersion;
};

struct pubsub_serializer {
	bundle_context_pt bundle_context;
	log_helper_pt loghelper;
};

celix_status_t pubsubSerializer_create(bundle_context_pt context, pubsub_serializer_pt *serializer);
celix_status_t pubsubSerializer_destroy(pubsub_serializer_pt serializer);

/* Start of serializer specific functions */
celix_status_t pubsubSerializer_serialize(pubsub_serializer_pt serializer, pubsub_message_type *msgType, const void *input, void **output, int *outputLen);
celix_status_t pubsubSerializer_deserialize(pubsub_serializer_pt serializer, pubsub_message_type *msgType, const void *input, void **output);

void pubsubSerializer_fillMsgTypesMap(pubsub_serializer_pt serializer, hash_map_pt msgTypesMap,bundle_pt bundle);
void pubsubSerializer_emptyMsgTypesMap(pubsub_serializer_pt serializer, hash_map_pt msgTypesMap);

version_pt pubsubSerializer_getVersion(pubsub_serializer_pt serializer, pubsub_message_type *msgType);
char* pubsubSerializer_getName(pubsub_serializer_pt serializer, pubsub_message_type *msgType);
void pubsubSerializer_freeMsg(pubsub_serializer_pt serializer, pubsub_message_type *msgType, void *msg);


#endif /* PUBSUB_SERIALIZER_IMPL_H_ */
