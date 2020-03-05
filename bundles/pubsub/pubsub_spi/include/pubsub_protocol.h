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

#ifndef PUBSUB_PROTOCOL_SERVICE_H_
#define PUBSUB_PROTOCOL_SERVICE_H_

#include "celix_properties.h"

#define PUBSUB_PROTOCOL_SERVICE_NAME      "pubsub_protocol"
#define PUBSUB_PROTOCOL_SERVICE_VERSION   "1.0.0"
#define PUBSUB_PROTOCOL_SERVICE_RANGE     "[1,2)"

typedef struct pubsub_protocol_header pubsub_protocol_header_t;

struct pubsub_protocol_header {
    unsigned int msgId;
    unsigned short msgMajorVersion;
    unsigned short msgMinorVersion;

    unsigned int payloadSize;
    unsigned int metadataSize;
};

typedef struct pubsub_protocol_payload pubsub_protocol_payload_t;

struct pubsub_protocol_payload {
    void *payload;
    size_t length;
};

typedef struct pubsub_protocol_metadata pubsub_protocol_metadata_t;

struct pubsub_protocol_metadata {
    celix_properties_t *metadata;
};

typedef struct pubsub_protocol_message pubsub_protocol_message_t;

struct pubsub_protocol_message {
    pubsub_protocol_header_t header;
    pubsub_protocol_payload_t payload;
    pubsub_protocol_metadata_t metadata;
};

typedef struct pubsub_protocol_service {
    void* handle;

    /**
     * Returns the header (as byte array) that should be used by the underlying protocol as sync between messages.
     *
     * @param handle handle for service
     * @param sync output param for byte array
     * @return status code indicating failure or success
     */
    celix_status_t (*getSyncHeader)(void *handle, void *sync);

    /**
     * Encodes the header using the supplied message.header.
     *
     * @param handle handle for service
     * @param message message to use header from
     * @param outBuffer byte array containing the encoded header
     * @param outLength length of the byte array
     * @return status code indicating failure or success
     */
    celix_status_t (*encodeHeader)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);

    /**
     * Encodes the payload using the supplied message.header. Note, this decode is for protocol specific tasks, and does not perform
     * the needed message serialization. See the serialization service for that.
     * In most cases this will simply use the known data and length from message.payload.
     *
     * @param handle handle for service
     * @param message message to use header from
     * @param outBuffer byte array containing the encoded payload
     * @param outLength length of the byte array
     * @return status code indicating failure or success
     */
    celix_status_t (*encodePayload)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);

    /**
     * Encodes the metadata using the supplied message.metadata.
     *
     * @param handle handle for service
     * @param message message to use header from
     * @param outBuffer byte array containing the encoded metadata
     * @param outLength length of the byte array
     * @return status code indicating failure or success
     */
    celix_status_t (*encodeMetadata)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);

    /**
     * Decodes the given data into message.header.
     *
     * @param handle handle for service
     * @param data incoming byte array to decode
     * @param length length of the byte array
     * @param message pointer to message to be filled in with decoded header
     * @return status code indicating failure or success
     */
    celix_status_t (*decodeHeader)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);

    /**
     * Decodes the given data into message.payload. Note, this decode is for protocol specific tasks, and does not perform
     * the needed message serialization. See the serialization service for that.
     * In most cases this will simply set the incoming data and length in message.payload.
     *
     * @param handle handle for service
     * @param data incoming byte array to decode
     * @param length length of the byte array
     * @param message pointer to message to be filled in with decoded payload
     * @return status code indicating failure or success
     */
    celix_status_t (*decodePayload)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);

    /**
     * Decodes the given data into message.metadata.
     *
     * @param handle handle for service
     * @param data incoming byte array to decode
     * @param length length of the byte array
     * @param message pointer to message to be filled in with decoded metadata
     * @return status code indicating failure or success
     */
    celix_status_t (*decodeMetadata)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);
} pubsub_protocol_service_t;

#endif /* PUBSUB_PROTOCOL_SERVICE_H_ */
