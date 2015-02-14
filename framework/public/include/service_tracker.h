/**
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
 * service_tracker.h
 *
 *  \date       Apr 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_TRACKER_H_
#define SERVICE_TRACKER_H_

#include "service_listener.h"
#include "array_list.h"
#include "bundle_context.h"
#include "service_tracker_customizer.h"
#include "framework_exports.h"

typedef struct serviceTracker * service_tracker_pt;

FRAMEWORK_EXPORT celix_status_t serviceTracker_create(bundle_context_pt context, char * service, service_tracker_customizer_pt customizer, service_tracker_pt *tracker);
FRAMEWORK_EXPORT celix_status_t serviceTracker_createWithFilter(bundle_context_pt context, char * filter, service_tracker_customizer_pt customizer, service_tracker_pt *tracker);

FRAMEWORK_EXPORT celix_status_t serviceTracker_open(service_tracker_pt tracker);
FRAMEWORK_EXPORT celix_status_t serviceTracker_close(service_tracker_pt tracker);
FRAMEWORK_EXPORT celix_status_t serviceTracker_destroy(service_tracker_pt tracker);

FRAMEWORK_EXPORT service_reference_pt serviceTracker_getServiceReference(service_tracker_pt tracker);
FRAMEWORK_EXPORT array_list_pt serviceTracker_getServiceReferences(service_tracker_pt tracker);

FRAMEWORK_EXPORT void * serviceTracker_getService(service_tracker_pt tracker);
FRAMEWORK_EXPORT array_list_pt serviceTracker_getServices(service_tracker_pt tracker);
FRAMEWORK_EXPORT void * serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference);

FRAMEWORK_EXPORT void serviceTracker_serviceChanged(service_listener_pt listener, service_event_pt event);

#endif /* SERVICE_TRACKER_H_ */
