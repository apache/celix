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

#ifndef PUBSUB_SERIALIZER_SERVICE_H_
#define PUBSUB_SERIALIZER_SERVICE_H_

#include "service_reference.h"
#include "hash_map.h"

#include "pubsub_common.h"

/**
 * There should be a pubsub_serializer_t
 * per msg type (msg id) per bundle
 *
 * The pubsub_serializer_service can create
 * a serializer_map per bundle. Potentially using
 * the extender pattern.
 */

typedef struct pubsub_msg_serializer {
	void* handle;
	unsigned int msgId;
	const char* msgName;
	version_pt msgVersion;

	celix_status_t (*serialize)(void* handle, const void* input, void** out, size_t* outLen);
	celix_status_t (*deserialize)(void* handle, const void* input, size_t inputLen, void** out); //note inputLen can be 0 if predefined size is not needed
	void (*freeMsg)(void* handle, void* msg);

} pubsub_msg_serializer_t;

typedef struct pubsub_serializer_service {
	void* handle;

	celix_status_t (*createSerializerMap)(void* handle, bundle_pt bundle, hash_map_pt* serializerMap);
	celix_status_t (*destroySerializerMap)(void* handle, hash_map_pt serializerMap);

} pubsub_serializer_service_t;

#endif /* PUBSUB_SERIALIZER_SERVICE_H_ */
