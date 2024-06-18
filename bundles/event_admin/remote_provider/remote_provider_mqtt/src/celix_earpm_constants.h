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

#ifndef CELIX_EARPM_CONSTANTS_H
#define CELIX_EARPM_CONSTANTS_H
#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_EARPM_SESSION_EXPIRY_INTERVAL_DEFAULT (10*60) //seconds

#define CELIX_EARPM_SYNC_EVENT_TIMEOUT_DEFAULT (5*60) //seconds

#define CELIX_EARPM_CONTINUOUS_ACK_TIMEOUT_COUNT_MAX 3

#define CELIX_EARPM_MSG_QUEUE_SIZE_MAX 512
#define CELIX_EARPM_PARALLEL_MSG_MAX 20

#define CELIX_EARPM_TOPIC_PREFIX "celix/EventAdminMqtt/"
#define CELIX_EARPM_TOPIC_PATTERN CELIX_EARPM_TOPIC_PREFIX"*"
#define CELIX_EARPM_HANDLER_DESCRIPTION_TOPIC_PREFIX CELIX_EARPM_TOPIC_PREFIX"HandlerDescription/"
#define CELIX_EARPM_HANDLER_DESCRIPTION_QUERY_TOPIC CELIX_EARPM_HANDLER_DESCRIPTION_TOPIC_PREFIX"query"
#define CELIX_EARPM_HANDLER_DESCRIPTION_UPDATE_TOPIC  CELIX_EARPM_HANDLER_DESCRIPTION_TOPIC_PREFIX"update"
#define CELIX_EARPM_HANDLER_DESCRIPTION_ADD_TOPIC  CELIX_EARPM_HANDLER_DESCRIPTION_TOPIC_PREFIX"add"
#define CELIX_EARPM_HANDLER_DESCRIPTION_REMOVE_TOPIC  CELIX_EARPM_HANDLER_DESCRIPTION_TOPIC_PREFIX"remove"

#define CELIX_EARPM_SYNC_EVENT_RESPONSE_TOPIC_PREFIX CELIX_EARPM_TOPIC_PREFIX"SyncEvent/response/" // topic = topic prefix + requester framework UUID

#define CELIX_EARPM_SESSION_EXPIRY_TOPIC CELIX_EARPM_TOPIC_PREFIX"session/expiry"

#define CELIX_EARPM_BROKER_PROFILE "CELIX_EARPM_BROKER_PROFILE"
#define CELIX_EARPM_BROKER_PROFILE_DEFAULT "/etc/mosquitto.conf"

#define CELIX_EARPM_EVENT_QOS "CELIX_EARPM_EVENT_QOS"
#define CELIX_EARPM_EVENT_QOS_DEFAULT CELIX_EARPM_QOS_AT_MOST_ONCE

typedef enum celix_earpm_qos {
    CELIX_EARPM_QOS_UNKNOWN = -1,
    CELIX_EARPM_QOS_AT_MOST_ONCE = 0,
    CELIX_EARPM_QOS_AT_LEAST_ONCE = 1,
    CELIX_EARPM_QOS_EXACTLY_ONCE = 2,
    CELIX_EARPM_QOS_MAX,
}celix_earpm_qos_e;

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_CONSTANTS_H
