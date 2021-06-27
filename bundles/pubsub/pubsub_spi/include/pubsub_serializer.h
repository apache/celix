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

#ifndef PUBSUB_SERIALIZER_SERVICE_H_
#define PUBSUB_SERIALIZER_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hash_map.h"
#include "version.h"
#include "celix_bundle.h"
#include "sys/uio.h"

/**
 * There should be a pubsub_serializer_t
 * per msg type (msg id) per bundle
 *
 * The pubsub_serializer_service can create
 * a serializer_map per bundle. Potentially using
 * the extender pattern.
 */

#define PUBSUB_SERIALIZER_SERVICE_NAME      "pubsub_serializer"
#define PUBSUB_SERIALIZER_SERVICE_VERSION   "1.0.0"
#define PUBSUB_SERIALIZER_SERVICE_RANGE     "[1,2)"

//NOTE deprecated used the pubsub_message_serialization_service instead
typedef struct pubsub_msg_serializer {
    void* handle;

    unsigned int msgId;
    const char* msgName;
    version_pt msgVersion;

    celix_status_t (*serialize)(void* handle, const void* input, struct iovec** output, size_t* outputIovLen);
    void (*freeSerializeMsg)(void* handle, const struct iovec* input, size_t inputIovLen);
    celix_status_t (*deserialize)(void* handle, const struct iovec* input, size_t inputIovLen, void** out); //note inputLen can be 0 if predefined size is not needed
    void (*freeDeserializeMsg)(void* handle, void* msg);

} pubsub_msg_serializer_t;

typedef struct pubsub_serializer_service {
    void* handle;

    celix_status_t (*createSerializerMap)(void* handle, const celix_bundle_t *bundle, hash_map_pt* serializerMap);
    celix_status_t (*destroySerializerMap)(void* handle, hash_map_pt serializerMap);

} pubsub_serializer_service_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_SERIALIZER_SERVICE_H_ */
