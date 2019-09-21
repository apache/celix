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
 * activator.c
 *
 *  Created on: Jul 9, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "event_publisher_impl.h"

#include <stdlib.h>

struct activator {
	bundle_context_pt context;
	event_publisher_pt event_publisher;
	service_tracker_pt tracker;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator = NULL;

	activator = calloc(1, sizeof(*activator));
	activator->context = context;
	*userData = activator;

	event_publisher_pt eventpublisher;
	status = eventPublisherCreate(context, &eventpublisher);
	if(status == CELIX_SUCCESS) {
		activator->event_publisher = eventpublisher;

	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * data = (struct activator *) userData;


	service_tracker_customizer_pt cust = NULL;
		service_tracker_pt tracker = NULL;
		data->context = context;
		serviceTrackerCustomizer_create(data->event_publisher, eventPublisherAddingService, eventPublisherAddedService, eventPublisherModifiedService, eventPublisherRemovedService, &cust);
		serviceTracker_create(context, (char *) EVENT_ADMIN_NAME, cust, &tracker);
		data->tracker = tracker;

		serviceTracker_open(tracker);

	eventPublisherStart(&data->event_publisher);
	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * data = (struct activator *) userData;
	eventPublisherStop(&data->event_publisher);
	serviceTracker_close(data->tracker);
	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}
