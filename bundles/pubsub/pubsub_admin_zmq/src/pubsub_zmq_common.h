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

#include "version.h"
#include "pubsub_common.h"


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
    //header
    unsigned int type;
    unsigned char major;
    unsigned char minor;
};

typedef struct pubsub_zmq_msg_header pubsub_zmq_msg_header_t;


int psa_zmq_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
void psa_zmq_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter);

bool psa_zmq_checkVersion(version_pt msgVersion, const pubsub_zmq_msg_header_t *hdr);


#endif //CELIX_PUBSUB_ZMQ_COMMON_H
