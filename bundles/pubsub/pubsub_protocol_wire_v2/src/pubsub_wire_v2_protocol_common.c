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

#include "pubsub_wire_v2_protocol_common.h"

#include <string.h>
#include "celix_byteswap.h"

int pubsubProtocol_wire_v2_readChar(const unsigned char *data, int offset, uint8_t *val) {
    memcpy(val, data + offset, sizeof(uint8_t));
    *val = *val;
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_wire_v2_readShort(const unsigned char *data, int offset, unsigned int convert, uint16_t *val) {
    memcpy(val, data + offset, sizeof(uint16_t));
    if (convert) *val = bswap_16(*val);
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_wire_v2_readInt(const unsigned char *data, int offset, unsigned int convert, uint32_t *val) {
    memcpy(val, data + offset, sizeof(uint32_t));
    if(convert) *val = bswap_32(*val);
    return offset + sizeof(uint32_t);
}

int pubsubProtocol_wire_v2_readLong(const unsigned char *data, int offset, unsigned int convert, uint64_t *val) {
    memcpy(val, data + offset, sizeof(uint64_t));
    if (convert) *val = bswap_64(*val);
    return offset + sizeof(uint64_t);
}

int pubsubProtocol_wire_v2_writeChar(unsigned char *data, int offset, uint8_t val) {
    memcpy(data + offset, &val, sizeof(uint8_t));
    return offset + sizeof(uint8_t);
}

int pubsubProtocol_wire_v2_writeShort(unsigned char *data, int offset, unsigned int  convert, uint16_t val) {
    uint16_t nVal = (convert) ? bswap_16(val) : val;
    memcpy(data + offset, &nVal, sizeof(uint16_t));
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_wire_v2_writeInt(unsigned char *data, int offset, unsigned int  convert, uint32_t val) {
    uint32_t nVal = (convert) ? bswap_32(val)  : val;
    memcpy(data + offset, &nVal, sizeof(uint32_t));
    return offset + sizeof(uint32_t);
}

int pubsubProtocol_wire_v2_writeLong(unsigned char *data, int offset, unsigned int convert, uint64_t val) {
    uint64_t nVal = (convert) ? bswap_64(val) : val;
    memcpy(data + offset, &nVal, sizeof(uint64_t));
    return offset + sizeof(uint64_t);
}
