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


#ifndef BUNDLE_EVENT_H_
#define BUNDLE_EVENT_H_

#include "celix_bundle_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED CELIX_BUNDLE_EVENT_INSTALLED
#define	OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED CELIX_BUNDLE_EVENT_STARTED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED CELIX_BUNDLE_EVENT_STOPPED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_UPDATED CELIX_BUNDLE_EVENT_UPDATED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED CELIX_BUNDLE_EVENT_UNINSTALLED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED CELIX_BUNDLE_EVENT_RESOLVED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED CELIX_BUNDLE_EVENT_UNRESOLVED
#define OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING CELIX_BUNDLE_EVENT_STARTING
#define OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING CELIX_BUNDLE_EVENT_STOPPING


typedef celix_bundle_event_type_e bundle_event_type_e;
typedef struct celix_bundle_event *bundle_event_pt;
typedef struct celix_bundle_event bundle_event_t;


#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_EVENT_H_ */
