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

#ifndef CELIX_PUBSUB_TCP_COMMON_H
#define CELIX_PUBSUB_TCP_COMMON_H

#include <utils.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <log_helper.h>
#include "version.h"
#include "pubsub_tcp_msg_header.h"


typedef struct pubsub_tcp_endPointStore{
  celix_thread_mutex_t mutex;
  hash_map_t *map;
} pubsub_tcp_endPointStore_t;

/*
 * NOTE tcp is used by first sending three frames:
 * 1) A subscription filter.
 * This is a 5 char string of the first two chars of scope and topic combined and terminated with a '\0'.
 *
 * 2) The pubsub_tcp_msg_header_t is send containg the type id and major/minor version
 *
 * 3) The actual payload
 */


int psa_tcp_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
void psa_tcp_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter);
bool psa_tcp_checkVersion(version_pt msgVersion, const pubsub_tcp_msg_header_t *hdr);
void psa_tcp_setupTcpContext(log_helper_t *logHelper, celix_thread_t *thread, const celix_properties_t *topicProperties);

#endif //CELIX_PUBSUB_TCP_COMMON_H
