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

#include "pubsub_wire_protocol_common.h"

#include <string.h>
#if defined(__APPLE__)
    #include <machine/endian.h>
#else
    #include <endian.h>
    #include <arpa/inet.h>
#endif

int readShort(const unsigned char *data, int offset, uint16_t *val) {
    memcpy(val, data + offset, sizeof(uint16_t));
    *val = ntohs(*val);
    return offset + (int)sizeof(uint16_t);
}

int readInt(const unsigned char *data, int offset, uint32_t *val) {
    memcpy(val, data + offset, sizeof(uint32_t));
    *val = ntohl(*val);
    return offset + (int)sizeof(uint32_t);
}

int readLong(const unsigned char *data, int offset, uint64_t *val) {
    memcpy(val, data + offset, sizeof(uint64_t));
#if defined(__APPLE__)
    *val = ntohll(*val);
#else
    *val = be64toh(*val);
#endif
    return offset + (int)sizeof(uint64_t);
}

int writeShort(unsigned char *data, int offset, uint16_t val) {
    uint16_t nVal = htons(val);
    memcpy(data + offset, &nVal, sizeof(uint16_t));
    return offset + (int)sizeof(uint16_t);
}

int writeInt(unsigned char *data, int offset, uint32_t val) {
    uint32_t nVal = htonl(val);
    memcpy(data + offset, &nVal, sizeof(uint32_t));
    return offset + (int)sizeof(uint32_t);
}

int writeLong(unsigned char *data, int offset, uint64_t val) {
#if defined(__APPLE__)
    uint64_t nVal = htonll(val);
#else
    uint64_t nVal = htobe64(val);
#endif
    memcpy(data + offset, &nVal, sizeof(uint64_t));
    return offset + (int)sizeof(uint64_t);
}
