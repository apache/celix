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
 * bundle_event.h
 *
 *  \date       Jun 28, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_EVENT_H_
#define BUNDLE_EVENT_H_

enum bundle_event_type
{
	OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED = 0x00000001,
	OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED = 0x00000002,
	OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED = 0x00000004,
	OSGI_FRAMEWORK_BUNDLE_EVENT_UPDATED = 0x00000008,
	OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED = 0x00000010,
	OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED = 0x00000020,
	OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED = 0x00000040,
	OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING = 0x00000080,
	OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING = 0x00000100,
	OSGI_FRAMEWORK_BUNDLE_EVENT_LAZY_ACTIVATION = 0x00000200,
};

typedef enum bundle_event_type bundle_event_type_e;
typedef struct bundle_event *bundle_event_pt;

#include "service_reference.h"
#include "bundle.h"

struct bundle_event {
    long bundleId;
    char* bundleSymbolicName;
	bundle_event_type_e type;
};

#endif /* BUNDLE_EVENT_H_ */
