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

#ifndef CELIX_PUBSUB_UDPMC_COMMON_H
#define CELIX_PUBSUB_UDPMC_COMMON_H

#include <utils.h>

#include "version.h"

typedef struct pubsub_udp_msg_header {
    unsigned int type;
    unsigned char major;
    unsigned char minor;
} pubsub_udp_msg_header_t;


bool psa_udpmc_checkVersion(version_pt msgVersion, pubsub_udp_msg_header_t *hdr);


#endif //CELIX_PUBSUB_UDPMC_COMMON_H
