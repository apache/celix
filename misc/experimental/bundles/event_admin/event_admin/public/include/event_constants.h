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
/**
 * event_constants.h
 *
 *  \date       Aug 11, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_CONSTANTS_H_
#define EVENT_CONSTANTS_H_

static const char * const EVENT_BUNDLE = "bundle";
static const char * const EVENT_BUNDLE_ID = "bundle.id";
static const char * const EVENT_BUNDLE_SIGNER = "bundle.signer";

static const char * const EVENT_BUNDLE_SYMBOLICNAME = "bundle.symbolicName";


static const char * const EVENT_BUNDLE_VERSION = "bundle.version";

static const char * const EVENT_DELIVERY_ASYNC_ORDERED = "async.ordered";
static const char * const EVENT_DELIVERY_ASYNC_UNORDERED = "async.unordered";
static const char * const EVENT = "event";
static const char * const EVENT_DELIVERY = "event.delivery";
static const char * const EVENT_FILTER = "event.filter";
static const char * const EVENT_TOPIC = "event.topic";
static const char * const EVENT_EXCEPTION = "exception";
static const char * const EVENT_EXCEPTION_CLASS = "exception.class";
static const char * const EVENT_EXCEPTION_MESSAGE = "exception.message";
static const char * const MESSAGE = "message";

static const char * const EVENT_SERVICE = "service";

static const char * const EVENT_SERVICE_ID = "async.ordered";

static const char * const EVENT_SERVICE_OBJECTCLASS = "service.objectClass";

static const char * const EVENT_SERVICE_PID = "service.pid";

static const char * const EVENT_TIMESTAMP = "timestamp";

#endif /* EVENT_CONSTANTS_H_ */
