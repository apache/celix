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

/**
 * @brief The Event Admin service name
 */
#define CELIX_EVENT_ADMIN_SERVICE_NAME "org.osgi.service.event.EventAdmin"

/**
 * @brief The Event Admin service version
 */
#define CELIX_EVENT_ADMIN_SERVICE_VERSION "1.0.0"
#define CELIX_EVENT_ADMIN_SERVICE_USE_RANGE "[1.0.0,2)"

/**
 * @brief The Event Admin service
 * @see https://osgi.org/specification/osgi.cmpn/7.0.0/service.event.html#org.osgi.service.event.EventAdmin
 */
typedef struct celix_event_admin_service {
    void* handle;
    /**
     * @brief Asynchronous event delivery . This method returns to the caller before delivery of the event is completed.
     * @param[in] handle The handle as provided by the service registration.
     * @param[in] topic The topic of the event.
     * @param[in] properties The properties of the event. It can be NULL.
     * @return Status code indicating failure or success. CELIX_SUCCESS if no errors are encountered. If an error is encountered, it should be a celix errno.
     */
    celix_status_t (*postEvent)(void* handle, const char* topic, const celix_properties_t* properties);
    /**
     * @brief Synchronous event delivery. This method does not return to the caller until delivery of the event is completed.
     * @param[in] handle The handle as provided by the service registration.
     * @param[in] topic The topic of the event.
     * @param[in] properties The properties of the event. It can be NULL.
     * @return Status code indicating failure or success. CELIX_SUCCESS if no errors are encountered. If an error is encountered, it should be return celix errno.
     */
    celix_status_t (*sendEvent)(void* handle, const char* topic, const celix_properties_t* properties);
}celix_event_admin_service_t;

#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_ADMIN_SERVICE_H
