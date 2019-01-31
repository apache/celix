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

#ifndef CELIX_PUBSUB_ZMQ_COMMON_H
#define CELIX_PUBSUB_ZMQ_COMMON_H

#include <utils.h>
#include <stdint.h>

#include "version.h"


/*
 * NOTE zmq is used by first sending three frames:
 * 1) A subscription filter.
 * This is a 5 char string of the first two chars of scope and topic combined and terminated with a '\0'.
 *
 * 2) The pubsub_zmq_msg_header_t is send containg the type id and major/minor version
 *
 * 3) The actual payload
 */


struct pubsub_zmq_msg_header {
    int32_t type; //msg type id (hash of fqn)
    int8_t major;
    int8_t minor;
    int32_t seqNr;
    unsigned char originUUID[16];
    int64_t sendtimeSeconds; //seconds since epoch
    int64_t sendTimeNanoseconds; //ns since epoch
};

typedef struct pubsub_zmq_msg_header pubsub_zmq_msg_header_t;


int psa_zmq_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
void psa_zmq_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter);

bool psa_zmq_checkVersion(version_pt msgVersion, const pubsub_zmq_msg_header_t *hdr);

celix_status_t psa_zmq_decodeHeader(const unsigned char *data, size_t dataLen, pubsub_zmq_msg_header_t *header);
void psa_zmq_encodeHeader(const pubsub_zmq_msg_header_t *msgHeader, unsigned char *data, size_t dataLen);
#endif //CELIX_PUBSUB_ZMQ_COMMON_H
