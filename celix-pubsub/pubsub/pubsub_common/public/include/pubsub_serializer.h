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
 *  \date       Dec 7, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_SERIALIZER_H
#define PUBSUB_SERIALIZER_H

#include "bundle.h"
#include "hash_map.h"
#include "celix_errno.h"

typedef struct _pubsub_message_type pubsub_message_type;

celix_status_t pubsubSerializer_serialize(pubsub_message_type *msgType, const void *input, void **output, int *outputLen);
celix_status_t pubsubSerializer_deserialize(pubsub_message_type *msgType, const void *input, void **output);

unsigned int pubsubSerializer_hashCode(const char *string);
version_pt pubsubSerializer_getVersion(pubsub_message_type *msgType);
char* pubsubSerializer_getName(pubsub_message_type *msgType);

void pubsubSerializer_fillMsgTypesMap(hash_map_pt msgTypesMap,bundle_pt bundle);
void pubsubSerializer_emptyMsgTypesMap(hash_map_pt msgTypesMap);

void pubsubSerializer_freeMsg(pubsub_message_type *msgType, void *msg);

#endif
