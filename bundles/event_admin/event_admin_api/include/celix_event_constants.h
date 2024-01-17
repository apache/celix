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

#define CELIX_EVENT_BUNDLE "bundle"

#define CELIX_EVENT_BUNDLE_ID "bundle.id"

#define CELIX_EVENT_BUNDLE_SIGNER "bundle.signer"

#define CELIX_EVENT_BUNDLE_SYMBOLICNAME "bundle.symbolicName"

#define CELIX_EVENT_BUNDLE_VERSION "bundle.version"

#define CELIX_EVENT_DELIVERY_ASYNC_ORDERED "async.ordered"

#define CELIX_EVENT_DELIVERY_ASYNC_UNORDERED "async.unordered"

#define CELIX_EVENT "event"

#define CELIX_EVENT_DELIVERY "event.delivery"

#define CELIX_EVENT_FILTER "event.filter"

#define CELIX_EVENT_TOPIC "event.topics"

#define CELIX_EVENT_EXCEPTION "exception"

#define CELIX_EVENT_EXCEPTION_CLASS "exception.class"

#define CELIX_EVENT_EXCEPTION_MESSAGE "exception.message"

#define CELIX_EVENT_MESSAGE "message"

#define CELIX_EVENT_SERVICE "service"

#define CELIX_EVENT_SERVICE_ID "service.id"

#define CELIX_EVENT_SERVICE_OBJECTCLASS "service.objectClass"

#define CELIX_EVENT_SERVICE_PID "service.pid"

#define CELIX_EVENT_TIMESTAMP "timestamp"

//end event constants

/**
 * @name Custom event constants for celix
 *
 */
#define CELIX_EVENT_FRAMEWORK_UUID "celix.framework.uuid"

#define CELIX_EVENT_PAYLOAD "celix.event.payload"


#ifdef __cplusplus
}
#endif

#endif //CELIX_EVENT_CONSTANTS_H
