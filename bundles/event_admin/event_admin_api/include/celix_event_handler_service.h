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

#ifndef CELIX_EVENT_HANDLER_SERVICE_H
#define CELIX_EVENT_HANDLER_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_properties.h"
#include "celix_errno.h"

/**
 * @brief Name of the event handler service
 */
#define CELIX_EVENT_HANDLER_SERVICE_NAME "celix_event_handler_service"

/**
 * @brief Version of the event handler service
 */
#define CELIX_EVENT_HANDLER_SERVICE_VERSION "1.0.0"
#define CELIX_EVENT_HANDLER_SERVICE_USE_RANGE "[1.0.0,2)"


/**
 * @brief Listener for Events.
 * @see https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.event.html#org.osgi.service.event.EventHandler
 */
typedef struct celix_event_handler_service {
    void* handle;
    /**
     * @brief Handle an event. Called by the event admin to notify the event handler of an event.
     * @details The event handler must be registered with the CELIX_EVENT_TOPIC service property.
     * Event Handlers which have not specified the CELIX_EVENT_TOPIC service property must not receive events.
     * Event handlers can also be registered with a service property named CELIX_EVENT_FILTER. The value of this property is LDAP-style filter string.
     * In addition, event handlers can be registered with a service property named CELIX_EVENT_DELIVERY. The value of this property is a string that indicates the delivery qualities required by the event handler.
     * @param[in] handle The handle as provided by the service registration.
     * @param[in] topic The topic of the event.
     * @param[in] properties The properties of the event.
     * @return Status code indicating failure or success.
     * CELIX_SUCCESS if no errors are encountered. If an error is encountered, it should be return celix errno.
     * @see CELIX_EVENT_TOPIC, CELIX_EVENT_FILTER, CELIX_EVENT_DELIVERY
     */
    celix_status_t (*handleEvent)(void* handle, const char* topic, const celix_properties_t* properties);
}celix_event_handler_service_t;

#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_HANDLER_SERVICE_H
