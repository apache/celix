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

#ifndef CELIX_EARPM_EVENT_DELIVERER_H
#define CELIX_EARPM_EVENT_DELIVERER_H
#ifdef __cplusplus
extern "C" {
#endif

#include  <stdbool.h>

#include "celix_errno.h"
#include "celix_cleanup.h"
#include "celix_bundle_context.h"
#include "celix_log_helper.h"
#include "celix_event_admin_service.h"

typedef struct celix_earpm_event_deliverer celix_earpm_event_deliverer_t;

celix_earpm_event_deliverer_t* celix_earpmd_create(celix_bundle_context_t* ctx, celix_log_helper_t* logHelper);
void celix_earpmd_destroy(celix_earpm_event_deliverer_t* earpmd);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_event_deliverer_t, celix_earpmd_destroy);

celix_status_t celix_earpmd_setEventAdminSvc(celix_earpm_event_deliverer_t* earpmd, celix_event_admin_service_t *eventAdminSvc);

typedef void (*celix_earpmd_deliver_done_callback)(void* data, const char* topic, celix_status_t status);

celix_status_t celix_earpmd_postEvent(celix_earpm_event_deliverer_t* earpmd, const char* topic, celix_properties_t* properties);

celix_status_t celix_earpmd_sendEvent(celix_earpm_event_deliverer_t* earpmd, const char* topic, celix_properties_t* properties, celix_earpmd_deliver_done_callback done, void* callbackData);

#ifdef __cplusplus
}
#endif

#endif //CELIX_EARPM_EVENT_DELIVERER_H
