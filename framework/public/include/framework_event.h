/*
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * framework_event.h
 *
 *  \date       Oct 8, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FRAMEWORK_EVENT_H_
#define FRAMEWORK_EVENT_H_

enum framework_event_type
{
	OSGI_FRAMEWORK_EVENT_STARTED = 0x00000001,
	OSGI_FRAMEWORK_EVENT_ERROR = 0x00000002,
	OSGI_FRAMEWORK_EVENT_PACKAGES_REFRESHED = 0x00000004,
	OSGI_FRAMEWORK_EVENT_STARTLEVEL_CHANGED = 0x00000008,
	OSGI_FRAMEWORK_EVENT_WARNING = 0x00000010,
	OSGI_FRAMEWORK_EVENT_INFO = 0x00000020,
	OSGI_FRAMEWORK_EVENT_STOPPED = 0x00000040,
	OSGI_FRAMEWORK_EVENT_STOPPED_UPDATE = 0x00000080,
	OSGI_FRAMEWORK_EVENT_STOPPED_BOOTCLASSPATH_MODIFIED = 0x00000100,
	OSGI_FRAMEWORK_EVENT_WAIT_TIMEDOUT = 0x00000200,
};

typedef enum framework_event_type framework_event_type_e;
typedef struct framework_event *framework_event_pt;

#include "service_reference.h"
#include "bundle.h"

struct framework_event {
	bundle_pt bundle;
	framework_event_type_e type;
	celix_status_t errorCode;
	char *error;
};

#endif /* FRAMEWORK_EVENT_H_ */
