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


typedef struct celix_earpm_client celix_earpm_client_t;

typedef void (*celix_earpmc_receive_msg_fp)(void* handle, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property *props);
typedef void (*celix_earpmc_connected_fp)(void* handle);

typedef struct celix_earpmc_create_options {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* sessionExpiryTopic;
    mosquitto_property* sessionExpiryProps;//ownership will be transferred to the callee
    void* callbackHandle;
    celix_earpmc_receive_msg_fp receiveMsgCallback;
    celix_earpmc_connected_fp connectedCallback;
}celix_earpmc_create_options_t;

celix_earpm_client_t* celix_earpmc_create(celix_earpmc_create_options_t* options);

void celix_earpmc_destroy(celix_earpm_client_t* client);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_client_t, celix_earpmc_destroy);

celix_status_t celix_earpmc_addBrokerInfoService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties);
celix_status_t celix_earpmc_removeBrokerInfoService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties);

celix_status_t celix_earpmc_subscribe(celix_earpm_client_t* client, const char* topic, int qos);
celix_status_t celix_earpmc_unsubscribe(celix_earpm_client_t* client, const char* topic);

celix_status_t celix_earpmc_publishSync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, int qos, const mosquitto_property* props, bool *noMatchingSubscribers, const struct timespec* absTime);

celix_status_t celix_earpmc_publishAsync(celix_earpm_client_t* client, const char* topic, const char* payload, size_t payloadSize, int qos, const mosquitto_property* props, bool errorIfDiscarded, bool replacedIfInQueue);




#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_CLIENT_H
