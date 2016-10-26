/**
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

/*
 * dm_event.c
 *
 *  \date       18 Dec 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <constants.h>
#include <utils.h>

#include "dm_event.h"

celix_status_t event_create(dm_event_type_e event_type, bundle_pt bundle, bundle_context_pt context, service_reference_pt reference, const void *service, dm_event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

	*event = calloc(1, sizeof(**event));
	if (!*event) {
		status = CELIX_ENOMEM;
	}

	const char* serviceIdStr = NULL;
	serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_ID, &serviceIdStr);
	unsigned long servId = strtoul(serviceIdStr,NULL,10);

	//FIXME service ranking can dynamicly change, but service reference can be removed at any time.
	const char* rankingStr = NULL;
	serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_RANKING, &rankingStr);
	long ranking = rankingStr == NULL ? 0 : atol(rankingStr);

	if (status == CELIX_SUCCESS) {
		(*event)->bundle = bundle;
		(*event)->event_type = event_type;
		(*event)->context = context;
		(*event)->reference = reference;
		(*event)->service = service;
		(*event)->serviceId = servId;
		(*event)->ranking = ranking;
	}

	return status;
}

celix_status_t event_destroy(dm_event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*event) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		free(*event);
		*event = NULL;
	}

	return status;
}

celix_status_t event_equals(const void *a, const void *b, bool *equals) {
	celix_status_t status = CELIX_SUCCESS;

	if (!a || !b) {
		*equals = false;
	} else {
		dm_event_pt a_ptr = (dm_event_pt)a;
		dm_event_pt b_ptr = (dm_event_pt)b;

		*equals = a_ptr->serviceId == b_ptr->serviceId;
	}

	return status;
}

celix_status_t event_compareTo(dm_event_pt event, dm_event_pt compareTo, int *compare) {
	celix_status_t status = CELIX_SUCCESS;

	*compare = utils_compareServiceIdsAndRanking(event->serviceId, event->ranking, compareTo->serviceId, compareTo->ranking);

	return status;
}

celix_status_t event_getService(dm_event_pt event, const void **service) {
	*service = event->service;
	return CELIX_SUCCESS;
}
