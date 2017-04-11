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

typedef struct pubsub_serializer {
	bundle_context_pt bundle_context;
	log_helper_pt loghelper;
} pubsub_serializer_t;

typedef struct pubsub_msg_serializer_impl {
    pubsub_msg_serializer_t msgSerializer;
    dyn_message_type* dynMsg;
} pubsub_msg_serializer_impl_t;

celix_status_t pubsubSerializer_create(bundle_context_pt context, pubsub_serializer_t* *serializer);
celix_status_t pubsubSerializer_destroy(pubsub_serializer_t* serializer);

celix_status_t pubsubSerializer_createSerializerMap(pubsub_serializer_t* serializer, bundle_pt bundle, pubsub_msg_serializer_map_t** out);
celix_status_t pubsubSerializer_destroySerializerMap(pubsub_serializer_t*, pubsub_msg_serializer_map_t* map);

/* Start of serializer specific functions */
celix_status_t pubsubMsgSerializer_serialize(pubsub_msg_serializer_impl_t* impl, const void* msg, char** out, size_t *outLen);
celix_status_t pubsubMsgSerializer_deserialize(pubsub_msg_serializer_impl_t* impl, const char* input, size_t inputLen, void **out);
void pubsubMsgSerializer_freeMsg(pubsub_msg_serializer_impl_t* impl, void *msg);

#endif /* PUBSUB_SERIALIZER_IMPL_H_ */
