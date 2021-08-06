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

#ifdef __cplusplus
extern "C" {
#endif

#include "hash_map.h"
#include "version.h"
#include "celix_bundle.h"
#include "sys/uio.h"

#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME      "pubsub_message_serialization_service"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION   "1.0.0"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_RANGE     "[1,2)"

#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY     "serialization.type"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY                "msg.fqn"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY            "msg.version"
#define PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY                 "msg.id"

/**
 * @brief A message serialization service for a serialization type (e.g. json) and
 * for a specific msg type (based on the fully qualified name) and version.
 *
 * The properties serialization.type, msg,fqn, msg.version and msg.id are mandatory
 */
typedef struct pubsub_message_serialization_service {
    void* handle;

    /**
     * @brief Serialize a message into iovec structs (set of structures with buffer pointer and length)
     *
     * The correct message serialization services will be selected based on the provided msgId.
     *
     * @param handle        The pubsub message serialization service handle.
     * @param msgId         The msg id for the message to be serialized.
     * @param input         A pointer to the message object
     * @param output        An output pointer to a array of iovec structs.
     * @param outputIovLen  The number of iovec struct created
     * @return              CELIX_SUCCESS on success, CELIX_ILLEGAL_ARGUMENT if the msg id is not known or serialization failed.
     */
    celix_status_t (*serialize)(void* handle, const void* input, struct iovec** output, size_t* outputIovLen);

    /**
     * @brief Free the memory of for the serialized msg.
     */
    void (*freeSerializedMsg)(void* handle, struct iovec* input, size_t inputIovLen);

    /**
     * @brief Deserialize a message using the provided iovec buffers.
     *
     * The deserialize function will also check if the target major/minor version of the message is valid with the version
     * of the serialized data.
     *
     * For some serialization types (e.g. JSON) newer versions of the serialized data can be deserialized.
     * E.g. JSON serialized data with version 1.2.0 can be deserialized to a target message with version 1.0.0
     * But JSON serialized data with a version 2.0.0 will not be deserialized to a target message with version 1.0.0
     * This assume correct use of semantic versioning.
     *
     * @param handle                    The pubsub message serialization service handle.
     * @param msgId                     The msg id for the message to be deserialized.
     * @param serializedMajorVersion    The major version of the serialized data
     * @param serializedMinorVersion    The minor version of the serialized data.
     * @param input                     Pointer to the first element in a array of iovecs.
     * @param inputIovLen               Then number of iovecs.
     * @param out                       The newly allocated and deserialized message object
     * @return                          CELIX_SUCCESS on success. CELIX_ILLEGAL_ARGUMENT if the msg id is not known,
     *                                  or if the version do no match or deserialization failed.
     */
    celix_status_t (*deserialize)(void* handle, const struct iovec* input, size_t inputIovLen, void** out); //note inputLen can be 0 if predefined size is not needed

    /**
     * @brief Free the memory for the  deserialized message.
     */
    void (*freeDeserializedMsg)(void* handle, void* msg);

} pubsub_message_serialization_service_t;

#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_MESSAGE_SERIALIZATION_SERVICE_H_ */
