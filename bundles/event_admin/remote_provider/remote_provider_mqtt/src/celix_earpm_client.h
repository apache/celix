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
#include <stdio.h>

#include "celix_errno.h"
#include "celix_cleanup.h"
#include "celix_properties.h"
#include "celix_bundle_context.h"
#include "celix_log_helper.h"
#include "endpoint_description.h"
#include "celix_earpm_constants.h"


typedef struct celix_earpm_client celix_earpm_client_t;
typedef struct celix_earpm_client_request_info celix_earpm_client_request_info_t;

typedef void (*celix_earpm_client_receive_msg_fp)(void* handle, const celix_earpm_client_request_info_t* requestInfo);
typedef void (*celix_earpm_client_connected_fp)(void* handle);

typedef struct celix_earpm_client_create_options {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* sessionEndMsgTopic;
    const char* sessionEndMsgSenderUUID;
    const char* sessionEndMsgVersion;
    void* callbackHandle;
    celix_earpm_client_receive_msg_fp receiveMsgCallback;
    celix_earpm_client_connected_fp connectedCallback;
} celix_earpm_client_create_options_t;

typedef enum celix_earpm_client_message_priority {
    //The priority message will be sent after all middle-priority messages,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %70
    CELIX_EARPM_MSG_PRI_LOW = 0,
    //The priority message will be sent after all high-priority messages,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %85
    CELIX_EARPM_MSG_PRI_MIDDLE = 1,
    //The priority message will be sent as soon as possible,
    // and the new message which has this priority will be discarded if the message queue usage rate exceeds %100
    CELIX_EARPM_MSG_PRI_HIGH = 2,
} celix_earpm_client_message_priority_e;

struct celix_earpm_client_request_info {
    const char* topic;
    const char* payload;
    size_t payloadSize;
    celix_earpm_qos_e qos;
    celix_earpm_client_message_priority_e pri;
    double expiryInterval;//seconds
    const char* responseTopic;
    const void* correlationData;
    size_t correlationDataSize;
    const char* senderUUID;
    const char* version;
};

celix_earpm_client_t* celix_earpmClient_create(celix_earpm_client_create_options_t* options);

void celix_earpmClient_destroy(celix_earpm_client_t* client);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_t, celix_earpmClient_destroy);

celix_status_t celix_earpmClient_mqttBrokerEndpointAdded(void* handle, const endpoint_description_t* endpoint, char* matchedFilter);

celix_status_t celix_earpmClient_mqttBrokerEndpointRemoved(void* handle, const endpoint_description_t* endpoint, char* matchedFilter);

celix_status_t celix_earpmClient_subscribe(celix_earpm_client_t* client, const char* topic, celix_earpm_qos_e qos);
celix_status_t celix_earpmClient_unsubscribe(celix_earpm_client_t* client, const char* topic);

celix_status_t celix_earpmClient_publishAsync(celix_earpm_client_t* client, const celix_earpm_client_request_info_t* requestInfo);

celix_status_t celix_earpmClient_publishSync(celix_earpm_client_t* client, const celix_earpm_client_request_info_t* requestInfo);

void celix_earpmClient_info(celix_earpm_client_t* client, FILE* outStream);


#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_CLIENT_H
