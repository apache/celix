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
 *  Created on: Apr 20, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_TRACKER_H_
#define SERVICE_TRACKER_H_

#include "service_listener.h"
#include "array_list.h"
#include "bundle_context.h"

struct serviceTrackerCustomizer {
	void * handle;
	celix_status_t (*addingService)(void * handle, SERVICE_REFERENCE reference, void **service);
	celix_status_t (*addedService)(void * handle, SERVICE_REFERENCE reference, void * service);
	celix_status_t (*modifiedService)(void * handle, SERVICE_REFERENCE reference, void * service);
	celix_status_t (*removedService)(void * handle, SERVICE_REFERENCE reference, void * service);
};

typedef struct serviceTrackerCustomizer * SERVICE_TRACKER_CUSTOMIZER;

typedef struct serviceTracker * SERVICE_TRACKER;

celix_status_t serviceTracker_create(apr_pool_t *pool, BUNDLE_CONTEXT context, char * service, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker);
celix_status_t serviceTracker_createWithFilter(apr_pool_t *pool, BUNDLE_CONTEXT context, char * filter, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker);

celix_status_t serviceTracker_open(SERVICE_TRACKER tracker);
celix_status_t serviceTracker_close(SERVICE_TRACKER tracker);

SERVICE_REFERENCE tracker_getServiceReference(SERVICE_TRACKER tracker);
ARRAY_LIST tracker_getServiceReferences(SERVICE_TRACKER tracker);

void * tracker_getService(SERVICE_TRACKER tracker);
ARRAY_LIST tracker_getServices(SERVICE_TRACKER tracker);
void * tracker_getServiceByReference(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference);

void tracker_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event);

#endif /* SERVICE_TRACKER_H_ */
