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

#ifndef CELIX_PUBSUB_SERIALIZER_HANDLER_H
#define CELIX_PUBSUB_SERIALIZER_HANDLER_H

#include <stdint.h>
#include <sys/uio.h>

#include "celix_log_helper.h"
#include "celix_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pubsub_serializer_handler pubsub_serializer_handler_t; //opaque type


/**
 * @brief Creates a pubsub serializer handler which tracks pubsub_custom_msg_serialization_service services using the provided serialization type.
 *
 * If there are multiple pubsub_message_serialization_service services for the same msg fqn
 * (targeted.msg.fqn property) the highest ranking service will be used.
 *
 * The message handler assumes (and checks) that all provided serialization services do not clash in message ids (so every msgId should have its own msgFqn)
 * and that only one version for a message serialization is registered.
 * This means that all bundles in a single celix container (a single process) should all use the same version of messages.
 *
 * If backwards compatibility is supported, when serialized message with a higher minor version when available in the serializer handler are used to
 * deserialize. This could be supported for serialization like json.
 * So when a json message of version 1.1.x with content {"field1":"value1", "field2":"value2"} is deserialized to a version 1.0 (which only has field1),
 * the message can and will be deserialized
 *
 * @param ctx                   The bundle contest.
 * @param serializerType        type of serialization services to handle (e.g. json, avrobin, etc)
 * @param backwardCompatible    Whether backwards compatible serialization is supported.
 * @return A newly created pubsub serializer handler.
 */
pubsub_serializer_handler_t* pubsub_serializerHandler_create(celix_bundle_context_t* ctx, const char* serializerType, bool backwardCompatible);

/**
 * @brief Creates a pubsub serializer handler which tracks pubsub_custom_msg_serialization_service services using the serialization type of the provided
 * marker service.id
 *
 * If there are multiple pubsub_message_serialization_service services for the same msg fqn
 * (targeted.msg.fqn property) the highest ranking service will be used.
 *
 * The message handler assumes (and checks) that all provided serialization services do not clash in message ids (so every msgId should have its own msgFqn)
 * and that only one version for a message serialization is registered.
 * This means that all bundles in a single celix container (a single process) should all use the same version of messages.
 *
 * If backwards compatibility is supported, when serialized message with a higher minor version when available in the serializer handler are used to
 * deserialize. This could be supported for serialization like json.
 * So when a json message of version 1.1.x with content {"field1":"value1", "field2":"value2"} is deserialized to a version 1.0 (which only has field1),
 * the message can and will be deserialized
 *
 * @param ctx                               The bundle contest.
 * @param pubsubSerializerMarkerSvcId       The service.id of the pubsub_serialization_marker to use for deferring serializationType and backwardsCompatible.
 * @param logHelper                         Optional log helper. If provided will be used to log issues whit creating a serializer handler for the provided marker svc id.
 * @return A newly created pubsub serializer handler.
 */
pubsub_serializer_handler_t* pubsub_serializerHandler_createForMarkerService(celix_bundle_context_t* ctx, long pubsubSerializerMarkerSvcId, celix_log_helper_t* logHelper);

/**
 * @brief destroy the pubsub_serializer_handler and free the used memory.
 */
void pubsub_serializerHandler_destroy(pubsub_serializer_handler_t* handler);

/**
 * @brief Serialize a message into iovec structs (set of structures with buffer pointer and length)
 *
 * The correct message serialization services will be selected based on the provided msgId.
 *
 * @param handler       The pubsub serialization handler.
 * @param msgId         The msg id for the message to be serialized.
 * @param input         A pointer to the message object
 * @param output        An output pointer to a array of iovec structs.
 * @param outputIovLen  The number of iovec struct created
 * @return              CELIX_SUCCESS on success, CELIX_ILLEGAL_ARGUMENT if the msg id is not known or serialization failed.
 */
celix_status_t pubsub_serializerHandler_serialize(pubsub_serializer_handler_t* handler, uint32_t msgId, const void* input, struct iovec** output, size_t* outputIovLen);

/**
 * @brief Free the memory of for the serialized msg.
 */
celix_status_t pubsub_serializerHandler_freeSerializedMsg(pubsub_serializer_handler_t* handler, uint32_t msgId, struct iovec* input, size_t inputIovLen);

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
 * @param handler                   The pubsub serialization handler.
 * @param msgId                     The msg id for the message to be deserialized.
 * @param serializedMajorVersion    The major version of the serialized data
 * @param serializedMinorVersion    The minor version of the serialized data.
 * @param input                     Pointer to the first element in a array of iovecs.
 * @param inputIovLen               Then number of iovecs.
 * @param out                       The newly allocated and deserialized message object
 * @return                          CELIX_SUCCESS on success. CELIX_ILLEGAL_ARGUMENT if the msg id is not known,
 *                                  or if the version do no match or deserialization failed.
 */
celix_status_t pubsub_serializerHandler_deserialize(pubsub_serializer_handler_t* handler, uint32_t msgId, int serializedMajorVersion, int serializedMinorVersion, const struct iovec* input, size_t inputIovLen, void** out);

/**
 * @brief Free the memory for the  deserialized message.
 */
celix_status_t pubsub_serializerHandler_freeDeserializedMsg(pubsub_serializer_handler_t* handler, uint32_t msgId, void* msg);

/**
 * @brief Whether the msg is support. More specifically:
 *  - msg id is known and
 *  - a serialized msg with the provided major and minor version can be deserialized.
 */
bool pubsub_serializerHandler_isMessageSupported(pubsub_serializer_handler_t* handler, uint32_t msgId, int majorVersion, int minorVersion);

/**
 * @brief Whether the serializer handler has found 1 or more pubsub_message_serialization_service for the provided msg id.
 */
bool pubsub_serializerHandler_isMessageSerializationServiceAvailable(pubsub_serializer_handler_t* handler, uint32_t msgId);

/**
 * @brief Get msg fqn from a msg id.
 * @return msg fqn or NULL if msg id is not known. msg fqn is valid as long as the handler exists.
 */
const char* pubsub_serializerHandler_getMsgFqn(pubsub_serializer_handler_t* handler, uint32_t msgId);

/**
 * @brief Get a msg id from a msgFqn.
 * @return msg id or 0 if msg fqn is not known.
 */
uint32_t pubsub_serializerHandler_getMsgId(pubsub_serializer_handler_t* handler, const char* msgFqn);

/**
 * @brief nr of serialization services found.
 */
size_t pubsub_serializerHandler_messageSerializationServiceCount(pubsub_serializer_handler_t* handler);


/**
 * @brief Get the serializer type for this hanlder.
 *
 * Valid as long as the handler exists.
 */
const char* pubsub_serializerHandler_getSerializationType(pubsub_serializer_handler_t* handler);

/**
 * @brief Returns the major version part of a message version.
 *
 * Returns -1 if message cannot be found.
 */
int pubsub_serializerHandler_getMsgMinorVersion(pubsub_serializer_handler_t* handler, uint32_t msgId);

/**
 * @brief Returns the minor version part of a message version.
 *
 * Returns -1 if message cannot be found.
 */
int pubsub_serializerHandler_getMsgMajorVersion(pubsub_serializer_handler_t* handler, uint32_t msgId);


/**
 * @brief Returns msg info (fqn, major version, minor version) in 1 call.
 *
 * @param handler               The serializer handler
 * @param msgId                 The msg id where to get the info for
 * @param msgFqnOut             If not NULL will be set to the msgFqn (valid as long as the serializer handler is valid)
 * @param msgMajorVersionOut    If not NULL will be set to the msg major version
 * @param msgMinorVersionOut    If not NULL will be set to the msg minor version
 * @return                      CELIX_SUCCESS on success, CELIX_ILLEGAL_ARGUMENT if the message for the provided msg id cannot be found.
 */
celix_status_t pubsub_serializerHandler_getMsgInfo(
        pubsub_serializer_handler_t* handler,
        uint32_t msgId,
        const char** msgFqnOut,
        int* msgMajorVersionOut,
        int* msgMinorVersionOut);

#ifdef __cplusplus
}
#endif

#endif //CELIX_PUBSUB_SERIALIZER_HANDLER_H
