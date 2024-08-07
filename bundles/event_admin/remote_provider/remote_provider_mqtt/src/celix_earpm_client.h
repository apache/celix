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

#ifndef CELIX_EARPM_CLIENT_H
#define CELIX_EARPM_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>
#include <mosquitto.h>

#include "celix_errno.h"
#include "celix_cleanup.h"
#include "celix_properties.h"
#include "celix_bundle_context.h"
#include "celix_log_helper.h"
#include "endpoint_description.h"
#include "celix_earpm_constants.h"


typedef struct celix_earpm_client celix_earpm_client_t;

typedef void (*celix_earpmc_receive_msg_fp)(void* handle, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property *props);
typedef void (*celix_earpmc_connected_fp)(void* handle);

typedef struct celix_earpmc_create_options {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* sessionEndTopic;
    mosquitto_property* sessionEndProps;//ownership will be transferred to the callee
    void* callbackHandle;
    celix_earpmc_receive_msg_fp receiveMsgCallback;
    celix_earpmc_connected_fp connectedCallback;
}celix_earpmc_create_options_t;

typedef enum celix_earpmc_message_priority {
    //The priority message will be sent after all middle-priority messages,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %70
    CELIX_EARPMC_MSG_PRI_LOW = 0,
    //The priority message will be sent after all high-priority messages,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %85
    CELIX_EARPMC_MSG_PRI_MIDDLE = 1,
    //The priority message will be sent as soon as possible,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %100
    CELIX_EARPMC_MSG_PRI_HIGH = 2,
} celix_earpmc_message_priority_e;

celix_earpm_client_t* celix_earpmc_create(celix_earpmc_create_options_t* options);

void celix_earpmc_destroy(celix_earpm_client_t* client);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_t, celix_earpmc_destroy);

celix_status_t celix_earpmc_endpointAdded(void* handle, endpoint_description_t* endpoint, char* matchedFilter);

celix_status_t celix_earpmc_endpointRemoved(void* handle, endpoint_description_t* endpoint, char* matchedFilter);

celix_status_t celix_earpmc_subscribe(celix_earpm_client_t* client, const char* topic, celix_earpm_qos_e qos);
celix_status_t celix_earpmc_unsubscribe(celix_earpm_client_t* client, const char* topic);

celix_status_t celix_earpmc_publishAsync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, celix_earpm_qos_e qos, const mosquitto_property* mqttProps, celix_earpmc_message_priority_e pri);

celix_status_t celix_earpmc_publishSync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, celix_earpm_qos_e qos, const mosquitto_property* mqttProps, const struct timespec* absTime);




#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_CLIENT_H
