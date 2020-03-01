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

#include <stdint.h>

#ifndef CELIX_PUBSUB_WIRE_PROTOCOL_COMMON_H
#define CELIX_PUBSUB_WIRE_PROTOCOL_COMMON_H

static const unsigned int PROTOCOL_WIRE_SYNC = 0xABBABAAB;
static const unsigned int PROTOCOL_WIRE_ENVELOPE_VERSION = 1;

int readShort(const unsigned char *data, int offset, uint16_t *val);
int readInt(const unsigned char *data, int offset, uint32_t *val);
int readLong(const unsigned char *data, int offset, uint64_t *val);

int writeShort(unsigned char *data, int offset, uint16_t val);
int writeInt(unsigned char *data, int offset, uint32_t val);
int writeLong(unsigned char *data, int offset, uint64_t val);

#endif //CELIX_PUBSUB_WIRE_PROTOCOL_COMMON_H
