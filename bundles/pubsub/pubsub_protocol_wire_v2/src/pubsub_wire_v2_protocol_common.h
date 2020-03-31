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

#ifndef CELIX_PUBSUB_WIRE_V2_PROTOCOL_COMMON_H
#define CELIX_PUBSUB_WIRE_V2_PROTOCOL_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static const unsigned int PROTOCOL_WIRE_V2_SYNC = 0xABBADEAF;
static const unsigned int PROTOCOL_WIRE_V2_ENVELOPE_VERSION = 1;

int pubsubProtocol_wire_v2_readChar(const unsigned char *data, int offset, uint8_t *val);
int pubsubProtocol_wire_v2_readShort(const unsigned char *data, int offset, unsigned int convert, uint16_t *val);
int pubsubProtocol_wire_v2_readInt(const unsigned char *data, int offset, unsigned int convert, uint32_t *val);
int pubsubProtocol_wire_v2_readLong(const unsigned char *data, int offset, unsigned int convert, uint64_t *val);

int pubsubProtocol_wire_v2_writeChar(unsigned char *data, int offset, uint8_t val);
int pubsubProtocol_wire_v2_writeShort(unsigned char *data, int offset, unsigned int convert, uint16_t val);
int pubsubProtocol_wire_v2_writeInt(unsigned char *data, int offset, unsigned int convert, uint32_t val);
int pubsubProtocol_wire_v2_writeLong(unsigned char *data, int offset, unsigned int convert, uint64_t val);

#ifdef __cplusplus
}
#endif

#endif //CELIX_PUBSUB_WIRE_V2_PROTOCOL_COMMON_H
