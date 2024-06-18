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

#ifndef CELIX_EARPM_IMPL_H
#define CELIX_EARPM_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_errno.h"
#include "celix_compiler.h"
#include "celix_bundle_context.h"
#include "celix_properties.h"
#include "celix_log_helper.h"
#include "celix_event_admin_service.h"

typedef struct celix_event_admin_remote_provider_mqtt celix_event_admin_remote_provider_mqtt_t;

celix_event_admin_remote_provider_mqtt_t* celix_earpm_create(celix_bundle_context_t* ctx);
void celix_earpm_destroy(celix_event_admin_remote_provider_mqtt_t* earpm);

celix_status_t celix_earpm_addBrokerInfoService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties);
celix_status_t celix_earpm_removeBrokerInfoService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties);

celix_status_t celix_earpm_addEventHandlerService(void* handle , void* service, const celix_properties_t* properties);
celix_status_t celix_earpm_removeEventHandlerService(void* handle , void* service, const celix_properties_t* properties);

celix_status_t celix_earpm_setEventAdminSvc(void* handle, void* eventAdminSvc);

celix_status_t celix_earpm_postEvent(void* handle , const char* topic, const celix_properties_t* properties);
celix_status_t celix_earpm_sendEvent(void* handle , const char* topic, const celix_properties_t* properties);

size_t celix_earpm_currentRemoteFrameworkCnt(celix_event_admin_remote_provider_mqtt_t* earpm);

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_IMPL_H
