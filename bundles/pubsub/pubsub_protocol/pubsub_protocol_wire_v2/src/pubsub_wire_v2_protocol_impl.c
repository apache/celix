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

#include <stdlib.h>
#include <string.h>

#include "celix_properties.h"

#include "pubsub_wire_v2_protocol_impl.h"
#include "pubsub_wire_protocol_common.h"

struct pubsub_protocol_wire_v2 {
};
celix_status_t pubsubProtocol_wire_v2_create(pubsub_protocol_wire_v2_t **protocol) {
    celix_status_t status = CELIX_SUCCESS;

    *protocol = calloc(1, sizeof(**protocol));

    if (!*protocol) {
        status = CELIX_ENOMEM;
    }
    else {}
    return status;
}

celix_status_t pubsubProtocol_wire_v2_destroy(pubsub_protocol_wire_v2_t* protocol) {
    celix_status_t status = CELIX_SUCCESS;
    free(protocol);
    return status;
}

celix_status_t pubsubProtocol_wire_v2_getHeaderSize(void* handle, size_t *length) {
    *length = sizeof(int) * 9 + sizeof(short) * 2; // header + sync + version = 36
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_wire_v2_getHeaderBufferSize(void* handle, size_t *length) {
    return pubsubProtocol_wire_v2_getHeaderSize(handle, length);
}

celix_status_t pubsubProtocol_wire_v2_getSyncHeaderSize(void* handle,  size_t *length) {
    *length = sizeof(int);
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_wire_v2_getSyncHeader(void* handle, void *syncHeader) {
    pubsubProtocol_writeInt(syncHeader, 0, false, PROTOCOL_WIRE_V2_SYNC_HEADER);
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_wire_v2_getFooterSize(void* handle,  size_t *length) {
    *length = sizeof(int);
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_wire_v2_isMessageSegmentationSupported(void* handle, bool *isSupported) {
    *isSupported = true;
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_wire_v2_encodeHeader(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    celix_status_t status = CELIX_SUCCESS;
    // Get HeaderSize
    size_t headerSize = 0;
    pubsubProtocol_wire_v2_getHeaderSize(handle, &headerSize);

    if (*outBuffer == NULL) {
        *outBuffer = malloc(headerSize);
        *outLength = headerSize;
    }
    if (*outBuffer == NULL) {
        status = CELIX_ENOMEM;
    } else {
        int idx = 0;
        unsigned int convert = message->header.convertEndianess;
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, PROTOCOL_WIRE_V2_SYNC_HEADER);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, PROTOCOL_WIRE_V2_ENVELOPE_VERSION);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.msgId);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.seqNr);
        idx = pubsubProtocol_writeShort(*outBuffer, idx, convert, message->header.msgMajorVersion);
        idx = pubsubProtocol_writeShort(*outBuffer, idx, convert, message->header.msgMinorVersion);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.payloadSize);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.metadataSize);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.payloadPartSize);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.payloadOffset);
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, message->header.isLastSegment);
        *outLength = idx;
    }

    return status;
}

celix_status_t pubsubProtocol_wire_v2_encodeFooter(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    celix_status_t status = CELIX_SUCCESS;
    // Get HeaderSize
    size_t footerSize = 0;
    pubsubProtocol_wire_v2_getFooterSize(handle, &footerSize);

    if (*outBuffer == NULL) {
        *outBuffer = malloc(footerSize);
        *outLength = footerSize;
    }
    if (*outBuffer == NULL) {
        status = CELIX_ENOMEM;
    } else {
        int idx = 0;
        unsigned int convert = message->header.convertEndianess;
        idx = pubsubProtocol_writeInt(*outBuffer, idx, convert, PROTOCOL_WIRE_V2_SYNC_FOOTER);
        *outLength = idx;
    }

    return status;
}

celix_status_t pubsubProtocol_wire_v2_decodeHeader(void* handle, void *data, size_t length, pubsub_protocol_message_t *message) {
    celix_status_t status = CELIX_SUCCESS;

    int idx = 0;
    size_t headerSize = 0;
    pubsubProtocol_wire_v2_getHeaderSize(handle, &headerSize);
    if (length == headerSize) {
        unsigned int sync = 0;
        unsigned int sync_endianess = 0;
        idx = pubsubProtocol_readInt(data, idx, false, &sync);
        pubsubProtocol_readInt(data, 0, true, &sync_endianess);
        message->header.convertEndianess = (sync_endianess == PROTOCOL_WIRE_V2_SYNC_HEADER) ? true : false;
        if ((sync != PROTOCOL_WIRE_V2_SYNC_HEADER) && (sync_endianess != PROTOCOL_WIRE_V2_SYNC_HEADER)) {
            status = CELIX_ILLEGAL_ARGUMENT;
        } else {
            unsigned int envelopeVersion;
            unsigned int convert = message->header.convertEndianess;
            idx = pubsubProtocol_readInt(data, idx, convert, &envelopeVersion);
            if (envelopeVersion != PROTOCOL_WIRE_V2_ENVELOPE_VERSION) {
                fprintf(stderr, "found sync %x and converted sync %x\n", sync, sync_endianess);
                fprintf(stderr, "wrong envelop version\n");
                fprintf(stderr, "Got %i, need %i\n", envelopeVersion, PROTOCOL_WIRE_V2_ENVELOPE_VERSION);
                status = CELIX_ILLEGAL_ARGUMENT;
            } else {
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.msgId);
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.seqNr);
                idx = pubsubProtocol_readShort(data, idx, convert, &message->header.msgMajorVersion);
                idx = pubsubProtocol_readShort(data, idx, convert, &message->header.msgMinorVersion);
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.payloadSize);
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.metadataSize);
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.payloadPartSize);
                idx = pubsubProtocol_readInt(data, idx, convert, &message->header.payloadOffset);
                pubsubProtocol_readInt(data, idx, convert, &message->header.isLastSegment);
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    return status;
}

celix_status_t pubsubProtocol_wire_v2_decodeFooter(void* handle, void *data, size_t length, pubsub_protocol_message_t *message) {
    celix_status_t status = CELIX_SUCCESS;

    int idx = 0;
    size_t footerSize = 0;
    pubsubProtocol_wire_v2_getFooterSize(handle, &footerSize);
    if (length == footerSize) {
        unsigned int footerSync;
        unsigned int convert = message->header.convertEndianess;
        idx = pubsubProtocol_readInt(data, idx, convert, &footerSync);
        if (footerSync != PROTOCOL_WIRE_V2_SYNC_FOOTER) {
            status = CELIX_ILLEGAL_ARGUMENT;
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    return status;
}

celix_status_t pubsubProtocol_wire_v2_encodePayload(void* handle __attribute__((unused)), pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    return pubsubProtocol_encodePayload(message, outBuffer, outLength);
}

celix_status_t pubsubProtocol_wire_v2_encodeMetadata(void* handle __attribute__((unused)), pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    return pubsubProtocol_encodeMetadata(message, outBuffer, outLength);
}

celix_status_t pubsubProtocol_wire_v2_decodePayload(void* handle __attribute__((unused)), void *data, size_t length, pubsub_protocol_message_t *message) {
    return pubsubProtocol_decodePayload(data, length, message);
}

celix_status_t pubsubProtocol_wire_v2_decodeMetadata(void* handle __attribute__((unused)), void *data, size_t length, pubsub_protocol_message_t *message) {
    return pubsubProtocol_decodeMetadata(data, length, message);
}
