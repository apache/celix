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

#ifndef CELIX_EVENT_CONSTANTS_H
#define CELIX_EVENT_CONSTANTS_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name event constants
 * @see https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.event.html#org.osgi.service.event.EventConstants
 */

/**
 * @brief The bundle id of the bundle relevant to the event. The type of the value for this event property is Long.
 */
#define CELIX_EVENT_BUNDLE_ID "bundle.id"

/**
 * @brief The bundle symbolic name of the bundle relevant to the event. The type of the value for this event property is String.
 */
#define CELIX_EVENT_BUNDLE_SYMBOLICNAME "bundle.symbolicName"

/**
 * @brief The version of the bundle relevant to the event. The type of the value for this event property is Version.
 */
#define CELIX_EVENT_BUNDLE_VERSION "bundle.version"

/**
 * @brief Event handler delivery quality value specifying the Event Handler requires asynchronously delivered events be delivered in order.
 * @see CELIX_EVENT_DELIVERY
 */
#define CELIX_EVENT_DELIVERY_ASYNC_ORDERED "async.ordered"

/**
 * @brief Event handler delivery quality value specifying the Event Handler requires asynchronously delivered events be delivered in any order.
 * @see CELIX_EVENT_DELIVERY
 */
#define CELIX_EVENT_DELIVERY_ASYNC_UNORDERED "async.unordered"

/**
 * @brief Service Registration property specifying the delivery qualities requested by an Event Handler service.
 * @see CELIX_EVENT_DELIVERY_ASYNC_ORDERED,CELIX_EVENT_DELIVERY_ASYNC_UNORDERED
 */
#define CELIX_EVENT_DELIVERY "event.delivery"

/**
 * @brief Service Registration property specifying a filter to further select Event s of interest to an Event Handler service.
 * The type of the value for this service property is String.
 */
#define CELIX_EVENT_FILTER "event.filter"

/**
 * @brief Service registration property specifying the Event topics of interest to an Event Handler service.
 *
 * The Event Handler must use the following grammar to indicate the topics in which it is interested:\n
 * topic-description := '*' | topic ( '/&#42;' )?  \n
 * topic := token ( '/' token )*    \n
 *
 * The event handler also can specify multiple topics by providing a comma separated list of topics.
 *
 * @example
 * - "com/acme/&#42;"                         Indicates all topics beginning with "com/acme/"   \n
 * - "com/acme/MyEvent"                       Indicates only the specific event which topic is "com/acme/MyEvent"  \n
 * - "*"                                      Indicates all topics   \n
 * - "com/acme/MyEvent,com/acme/MyOtherEvent" Indicates the specific events with topics "com/acme/MyEvent" and "com/acme/MyOtherEvent"
 *
 */
#define CELIX_EVENT_TOPIC "event.topics"

/**
 * @brief A service's id. The type of the value for this event property is Long.
 */
#define CELIX_EVENT_SERVICE_ID "service.id"

/**
 * @brief A service's objectClass. The type of the value for this event property is String.
 */
#define CELIX_EVENT_SERVICE_OBJECTCLASS "service.objectClass"

/**
 * @brief A service's persistent identity. The type of the value for this event property is String.
 */
#define CELIX_EVENT_SERVICE_PID "service.pid"

/**
 * @brief The time when the event occurred, as reported by the system time in microseconds. The type of the value for this event property is Long.
 */
#define CELIX_EVENT_TIMESTAMP "timestamp"

//end event constants

#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_CONSTANTS_H
