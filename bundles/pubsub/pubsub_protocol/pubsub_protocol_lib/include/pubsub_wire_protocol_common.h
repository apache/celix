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

#ifndef CELIX_PUBSUB_PROTOCOL_COMMON_H
#define CELIX_PUBSUB_PROTOCOL_COMMON_H

#include <stdint.h>

#include "celix_errno.h"
#include "pubsub_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NETSTRING_ERROR_TOO_LONG     -1
#define NETSTRING_ERROR_NO_COLON     -2
#define NETSTRING_ERROR_TOO_SHORT    -3
#define NETSTRING_ERROR_NO_COMMA     -4
#define NETSTRING_ERROR_LEADING_ZERO -5
#define NETSTRING_ERROR_NO_LENGTH    -6

static const unsigned int PROTOCOL_WIRE_V1_SYNC_HEADER = 0xABBABAAB;
static const unsigned int PROTOCOL_WIRE_V1_ENVELOPE_VERSION = 1;

static const unsigned int PROTOCOL_WIRE_V2_SYNC_HEADER = 0xABBADEAF;
static const unsigned int PROTOCOL_WIRE_V2_SYNC_FOOTER = 0xDEAFABBA;
static const unsigned int PROTOCOL_WIRE_V2_ENVELOPE_VERSION = 2;

int pubsubProtocol_readChar(const unsigned char *data, int offset, uint8_t *val);
int pubsubProtocol_readShort(const unsigned char *data, int offset, uint32_t convert, uint16_t *val);
int pubsubProtocol_readInt(const unsigned char *data, int offset, uint32_t convert, uint32_t *val);
int pubsubProtocol_readLong(const unsigned char *data, int offset, uint32_t convert, uint64_t *val);

int pubsubProtocol_writeChar(unsigned char *data, int offset, uint8_t val);
int pubsubProtocol_writeShort(unsigned char *data, int offset, uint32_t convert, uint16_t val);
int pubsubProtocol_writeInt(unsigned char *data, int offset, uint32_t convert, uint32_t val);
int pubsubProtocol_writeLong(unsigned char *data, int offset, uint32_t convert, uint64_t val);

celix_status_t pubsubProtocol_encodePayload(pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength);
celix_status_t pubsubProtocol_decodePayload(void *data, size_t length, pubsub_protocol_message_t *message);
celix_status_t pubsubProtocol_decodeMetadata(void *data, size_t length, pubsub_protocol_message_t *message);

/**
 * @brief Encode metadata to bufferInOut.
 *
 * The metadata is encoded as a bytebuffer where the first 4 bytes is used to specify how many metadata (key,value)
 * entries there are and then every metadata entry is encoded as a netstring key followed by a netstring value.
 *
 * If *bufferInOut is NULL, a new buffer will be allocated. If *bufferInOut is not NULL, the buffer is reused and the
 * provided *bufferLengthInOut must indicate the length of the provided buffer.
 * If a provided *bufferInOut is not large enough to fit the encoded metadata, the buffer will be reallocated.
 *
 * If this calls return with an error, the caller is still owner of a possible returned output buffer.
 *
 * @param message The message containing the metadata to encode
 * @param bufferInOut Input/output argument for the buffer, if call is successful will contain the metadata header
 * @param bufferLengthInOut Input/output arguments for the length of the bufferInOut argument.
 * @param bufferContentLengthOut Output argument for the actual content size of the bufferInOut. Note that the
 *                               bufferContentLengthOut can be smaller than the buffer length.
 * @return CELIX_SUCCESS if encoding was successful.
 */
celix_status_t pubsubProtocol_encodeMetadata(pubsub_protocol_message_t* message, char** bufferInOut, size_t* bufferLengthInOut, size_t* bufferContentLengthOut);


#ifdef __cplusplus
}
#endif

#endif //CELIX_PUBSUB_PROTOCOL_COMMON_H
