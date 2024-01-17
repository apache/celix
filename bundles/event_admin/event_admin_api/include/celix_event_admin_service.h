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

#ifndef CELIX_EVENT_ADMIN_SERVICE_H
#define CELIX_EVENT_ADMIN_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_properties.h"
#include "celix_errno.h"

#define CELIX_EVENT_ADMIN_SERVICE_NAME "org.osgi.service.event.EventAdmin"

#define CELIX_EVENT_ADMIN_SERVICE_VERSION "1.0.0"
#define CELIX_EVENT_ADMIN_SERVICE_USE_RANGE "[1.0.0,2)"

/**
 * @brief The Event Admin service
 * @see https://osgi.org/specification/osgi.cmpn/7.0.0/service.event.html
 */
typedef struct celix_event_admin_service {
    void* handle;
    celix_status_t (*postEvent)(void* handle, const char* topic, const celix_properties_t* properties);
    celix_status_t (*sendEvent)(void* handle, const char* topic, const celix_properties_t* properties);
}celix_event_admin_service_t;

#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_ADMIN_SERVICE_H
