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
 * dm_event.h
 *
 *  \date       17 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_EVENT_H_
#define DM_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "service_reference.h"
#include "bundle_context.h"
#include "bundle.h"

enum dm_event_type {
	DM_EVENT_ADDED,
	DM_EVENT_CHANGED,
	DM_EVENT_REMOVED,
	DM_EVENT_SWAPPED,
};

typedef enum dm_event_type dm_event_type_e;

struct dm_event {
	const void* service;
	unsigned long serviceId;
	long ranking;
	service_reference_pt reference;
	bundle_context_pt context;
	bundle_pt bundle;
	dm_event_type_e event_type;
};

typedef struct dm_event *dm_event_pt;


celix_status_t event_create(dm_event_type_e event_type, bundle_pt bundle, bundle_context_pt context, service_reference_pt reference, const void* service, dm_event_pt *event);
celix_status_t event_destroy(dm_event_pt* event);

celix_status_t event_equals(const void* a, const void* b, bool* equals);

celix_status_t event_getService(dm_event_pt event, const void** service);
celix_status_t event_compareTo(dm_event_pt event, dm_event_pt compareTo, int* compare);

#ifdef __cplusplus
}
#endif

#endif /* DM_EVENT_H_ */
