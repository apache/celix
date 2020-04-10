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

#ifndef PUBSUB_MESSAGE_SERIALIZATION_SERVICE_H_
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_H_

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

#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME      "pubsub_message_serialization_service"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION   "1.0.0"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_RANGE     "[1,2)"

#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY     "serialization.type"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY                "msg.fqn"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY            "msg.version"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY                 "msg.id"

/**
 * A message serialization service for a serialization type (e.g. json) and
 * for a specific msg type (based on the fully qualified name) and version.
 *
 * The properties serialization.type, msg,fqn, msg.version and msg.id are mandatory
 */
typedef struct pubsub_message_serialization_service {
    void* handle;
    
    celix_status_t (*serialize)(void* handle, const void* input, struct iovec** output, size_t* outputIovLen);
    void (*freeSerializedMsg)(void* handle, struct iovec* input, size_t inputIovLen);
    celix_status_t (*deserialize)(void* handle, const struct iovec* input, size_t inputIovLen, void** out); //note inputLen can be 0 if predefined size is not needed
    void (*freeDeserializedMsg)(void* handle, void* msg);

} pubsub_message_serialization_service_t;

#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME      "pubsub_serialization_marker"
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_VERSION   "1.0.0"
#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_RANGE     "[1,2)"

#define PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY     "serialization.type"

/**
 * Marker which should be registered to mark the presence of this type of
 * serialization service for the different message types.
 *
 * Mandatory property is PUBSUB_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY (e.g. json)
 */
typedef struct pubsub_message_serialization_marker {
    void *handle;
    //TODO maybe a match function with qos as input?
} pubsub_message_serialization_marker_t;

#endif /* PUBSUB_MESSAGE_SERIALIZATION_SERVICE_H_ */
