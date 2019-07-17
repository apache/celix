/**
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

#include <memory.h>
#include <assert.h>
#include "pubsub_zmq_common.h"

int psa_zmq_localMsgTypeIdForMsgType(void* handle __attribute__((unused)), const char* msgType, unsigned int* msgTypeId) {
    *msgTypeId = utils_stringHash(msgType);
    return 0;
}

bool psa_zmq_checkVersion(version_pt msgVersion, const pubsub_zmq_msg_header_t *hdr) {
    bool check=false;
    int major=0,minor=0;

    if (hdr->major == 0 && hdr->minor == 0) {
        //no check
        return true;
    }

    if (msgVersion!=NULL) {
        version_getMajor(msgVersion,&major);
        version_getMinor(msgVersion,&minor);
        if (hdr->major==((unsigned char)major)) { /* Different major means incompatible */
            check = (hdr->minor>=((unsigned char)minor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
        }
    }

    return check;
}

void psa_zmq_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter) {
    for (int i = 0; i < 5; ++i) {
        filter[i] = '\0';
    }
    if (scope != NULL && strnlen(scope, 3) >= 2)  {
        filter[0] = scope[0];
        filter[1] = scope[1];
    }
    if (topic != NULL && strnlen(topic, 3) >= 2)  {
        filter[2] = topic[0];
        filter[3] = topic[1];
    }
}

static int readInt(const unsigned char *data, int offset, uint32_t *val) {
    *val = ((data[offset+0] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | (data[offset+3] << 0));
    return offset + 4;
}

static int readLong(const unsigned char *data, int offset, uint64_t *val) {
    *val = (
            ((int64_t)data[offset+0] << 56) |
            ((int64_t)data[offset+1] << 48) |
            ((int64_t)data[offset+2] << 40) |
            ((int64_t)data[offset+3] << 32) |
            ((int64_t)data[offset+4] << 24) |
            ((int64_t)data[offset+5] << 16) |
            ((int64_t)data[offset+6] << 8 ) |
            ((int64_t)data[offset+7] << 0 )
    );
    return offset + 8;
}

celix_status_t psa_zmq_decodeHeader(const unsigned char *data, size_t dataLen, pubsub_zmq_msg_header_t *header) {
    int status = CELIX_ILLEGAL_ARGUMENT;
    if (dataLen == sizeof(pubsub_zmq_msg_header_t)) {
        int index = 0;
        index = readInt(data, index, &header->type);
        header->major = (unsigned char) data[index++];
        header->minor = (unsigned char) data[index++];

        index = readInt(data, index, &header->seqNr);
        for (int i = 0; i < 16; ++i) {
            header->originUUID[i] = data[index+i];
        }
        index += 16;
        index = readLong(data, index, &header->sendtimeSeconds);
        readLong(data, index, &header->sendTimeNanoseconds);

        status = CELIX_SUCCESS;
    }
    return status;
}


static int writeInt(unsigned char *data, int offset, int32_t val) {
    data[offset+0] = (unsigned char)((val >> 24) & 0xFF);
    data[offset+1] = (unsigned char)((val >> 16) & 0xFF);
    data[offset+2] = (unsigned char)((val >> 8 ) & 0xFF);
    data[offset+3] = (unsigned char)((val >> 0 ) & 0xFF);
    return offset + 4;
}

static int writeLong(unsigned char *data, int offset, int64_t val) {
    data[offset+0] = (unsigned char)((val >> 56) & 0xFF);
    data[offset+1] = (unsigned char)((val >> 48) & 0xFF);
    data[offset+2] = (unsigned char)((val >> 40) & 0xFF);
    data[offset+3] = (unsigned char)((val >> 32) & 0xFF);
    data[offset+4] = (unsigned char)((val >> 24) & 0xFF);
    data[offset+5] = (unsigned char)((val >> 16) & 0xFF);
    data[offset+6] = (unsigned char)((val >> 8 ) & 0xFF);
    data[offset+7] = (unsigned char)((val >> 0 ) & 0xFF);
    return offset + 8;
}

void psa_zmq_encodeHeader(const pubsub_zmq_msg_header_t *msgHeader, unsigned char *data, size_t dataLen) {
    assert(dataLen == sizeof(*msgHeader));
    int index = 0;
    index = writeInt(data, index, msgHeader->type);
    data[index++] = (unsigned char)msgHeader->major;
    data[index++] = (unsigned char)msgHeader->minor;
    index = writeInt(data, index, msgHeader->seqNr);
    for (int i = 0; i < 16; ++i) {
        data[index+i] = msgHeader->originUUID[i];
    }
    index += 16;
    index = writeLong(data, index, msgHeader->sendtimeSeconds);
    writeLong(data, index, msgHeader->sendTimeNanoseconds);
}
