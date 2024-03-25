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

#ifndef CELIX_EVENT_ADMIN_H
#define CELIX_EVENT_ADMIN_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_bundle_context.h"
#include "celix_errno.h"

typedef struct celix_event_admin celix_event_admin_t;

celix_event_admin_t* celix_eventAdmin_create(celix_bundle_context_t* ctx);

void celix_eventAdmin_destroy(celix_event_admin_t* ea);

int celix_eventAdmin_start(celix_event_admin_t* ea);
int celix_eventAdmin_stop(celix_event_admin_t* ea);

int celix_eventAdmin_addEventHandlerWithProperties(void* handle, void* svc, const celix_properties_t* props);
int celix_eventAdmin_removeEventHandlerWithProperties(void* handle, void* svc, const celix_properties_t* props);

celix_status_t celix_eventAdmin_sendEvent(void* handle, const char* topic, const celix_properties_t* props);
celix_status_t celix_eventAdmin_postEvent(void* handle, const char* topic, const celix_properties_t* props);

#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_ADMIN_H
