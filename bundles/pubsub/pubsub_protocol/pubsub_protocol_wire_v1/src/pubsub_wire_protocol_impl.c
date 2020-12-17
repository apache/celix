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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h"
#include "celix_properties.h"

#include "pubsub_wire_protocol_impl.h"
#include "pubsub_wire_protocol_common.h"

#define BYTESWAP_SYNC true

struct pubsub_protocol_wire_v1 {
};

celix_status_t pubsubProtocol_create(pubsub_protocol_wire_v1_t **protocol) {
    celix_status_t status = CELIX_SUCCESS;

    *protocol = calloc(1, sizeof(**protocol));

    if (!*protocol) {
        status = CELIX_ENOMEM;
    }
    else {
        //
    }

    return status;
}

celix_status_t pubsubProtocol_destroy(pubsub_protocol_wire_v1_t* protocol) {
    celix_status_t status = CELIX_SUCCESS;
    free(protocol);
    return status;
}

celix_status_t pubsubProtocol_getHeaderSize(void* handle __attribute__((unused)), size_t *length) {
    *length = sizeof(int) * 5 + sizeof(short) * 2; // header + sync + version = 24
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getHeaderBufferSize(void* handle, size_t *length) {
    return pubsubProtocol_getHeaderSize(handle, length);
}

celix_status_t pubsubProtocol_getSyncHeaderSize(void* handle __attribute__((unused)),  size_t *length) {
    *length = sizeof(int);
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getSyncHeader(void* handle __attribute__((unused)), void *syncHeader) {
    pubsubProtocol_writeInt(syncHeader, 0, true, PROTOCOL_WIRE_V1_SYNC_HEADER);
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getFooterSize(void* handle __attribute__((unused)),  size_t *length) {
    *length = 0;
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_isMessageSegmentationSupported(void* handle __attribute__((unused)), bool *isSupported) {
    *isSupported = false;
    return CELIX_SUCCESS;
}
celix_status_t pubsubProtocol_encodeHeader(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    celix_status_t status = CELIX_SUCCESS;
    // Get HeaderSize
    size_t headerSize = 0;
    pubsubProtocol_getHeaderSize(handle, &headerSize);

    if (*outBuffer == NULL) {
        *outBuffer = calloc(1, headerSize);
        *outLength = headerSize;
    }
    if (*outBuffer == NULL) {
        status = CELIX_ENOMEM;
    } else {
        int idx = 0;
        idx = pubsubProtocol_writeInt(*outBuffer, idx,  BYTESWAP_SYNC, PROTOCOL_WIRE_V1_SYNC_HEADER);
        idx = pubsubProtocol_writeInt(*outBuffer, idx,  true, PROTOCOL_WIRE_V1_ENVELOPE_VERSION);
        idx = pubsubProtocol_writeInt(*outBuffer, idx,  true, message->header.msgId);
        idx = pubsubProtocol_writeShort(*outBuffer, idx, true, message->header.msgMajorVersion);
        idx = pubsubProtocol_writeShort(*outBuffer, idx, true, message->header.msgMinorVersion);
        idx = pubsubProtocol_writeInt(*outBuffer, idx,  true, message->header.payloadSize);
        idx = pubsubProtocol_writeInt(*outBuffer, idx,  true, message->header.metadataSize);

        *outLength = idx;
    }

    return status;
}

celix_status_t pubsubProtocol_v1_encodePayload(void *handle __attribute__((unused)), pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    return pubsubProtocol_encodePayload(message, outBuffer, outLength);
}

celix_status_t pubsubProtocol_v1_encodeMetadata(void *handle __attribute__((unused)), pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    return pubsubProtocol_encodeMetadata(message, outBuffer, outLength);
}

celix_status_t pubsubProtocol_encodeFooter(void *handle __attribute__((unused)), pubsub_protocol_message_t *message __attribute__((unused)), void **outBuffer, size_t *outLength) {
    *outBuffer = NULL;
    return pubsubProtocol_getFooterSize(handle,  outLength);
}

celix_status_t pubsubProtocol_v1_decodePayload(void* handle __attribute__((unused)), void *data, size_t length, pubsub_protocol_message_t *message){
    return pubsubProtocol_decodePayload(data, length, message);
}

celix_status_t pubsubProtocol_v1_decodeMetadata(void* handle __attribute__((unused)), void *data, size_t length, pubsub_protocol_message_t *message) {
    return pubsubProtocol_decodeMetadata(data, length, message);
}

celix_status_t pubsubProtocol_decodeFooter(void* handle __attribute__((unused)), void *data __attribute__((unused)), size_t length __attribute__((unused)), pubsub_protocol_message_t *message __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t pubsubProtocol_decodeHeader(void *handle, void *data, size_t length, pubsub_protocol_message_t *message) {
    celix_status_t status = CELIX_SUCCESS;

    int idx = 0;
    size_t headerSize = 0;
    pubsubProtocol_getHeaderSize(handle, &headerSize);
    if (length == headerSize) {
        unsigned int sync;
        idx = pubsubProtocol_readInt(data, idx,  BYTESWAP_SYNC, &sync);
        if (sync != PROTOCOL_WIRE_V1_SYNC_HEADER) {
            status = CELIX_ILLEGAL_ARGUMENT;
        } else {
            unsigned int envelopeVersion;
            idx = pubsubProtocol_readInt(data, idx,  true, &envelopeVersion);
            if (envelopeVersion != PROTOCOL_WIRE_V1_ENVELOPE_VERSION) {
                status = CELIX_ILLEGAL_ARGUMENT;
            } else {
                idx = pubsubProtocol_readInt(data, idx,  true, &message->header.msgId);
                idx = pubsubProtocol_readShort(data, idx, true, &message->header.msgMajorVersion);
                idx = pubsubProtocol_readShort(data, idx, true, &message->header.msgMinorVersion);
                idx = pubsubProtocol_readInt(data, idx,  true, &message->header.payloadSize);
                pubsubProtocol_readInt(data, idx,  true, &message->header.metadataSize);
                // Set message segmentation parameters to defaults
                message->header.seqNr           = 0;
                message->header.payloadPartSize = message->header.payloadSize;
                message->header.payloadOffset   = 0;
                message->header.isLastSegment   = 0x1;
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    return status;
}