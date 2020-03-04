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

    celix_status_t (*getSyncHeader)(void *handle, void *sync);

    celix_status_t (*encodeHeader)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);
    celix_status_t (*encodePayload)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);
    celix_status_t (*encodeMetadata)(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);

    celix_status_t (*decodeHeader)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);
    celix_status_t (*decodePayload)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);
    celix_status_t (*decodeMetadata)(void* handle, void *data, size_t length, pubsub_protocol_message_t *message);
} pubsub_protocol_service_t;

#endif /* PUBSUB_PROTOCOL_SERVICE_H_ */
