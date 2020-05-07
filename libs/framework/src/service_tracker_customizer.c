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
 * service_tracker_customizer.c
 *
 *  \date       Nov 15, 2012
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include "service_tracker_customizer.h"

#include "celix_log.h"

celix_status_t serviceTrackerCustomizer_create(void *handle,
		adding_callback_pt addingFunction, added_callback_pt addedFunction,
		modified_callback_pt modifiedFunction, removed_callback_pt removedFunction, service_tracker_customizer_pt *customizer) {
	celix_status_t status = CELIX_SUCCESS;

	if (handle == NULL || *customizer != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*customizer = malloc(sizeof(**customizer));
		if (!*customizer) {
			status = CELIX_ENOMEM;
		} else {
			(*customizer)->handle = handle;
			(*customizer)->addingService = addingFunction;
			(*customizer)->addedService = addedFunction;
			(*customizer)->modifiedService = modifiedFunction;
			(*customizer)->removedService = removedFunction;
		}
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create customizer");

	return status;
}

celix_status_t serviceTrackerCustomizer_destroy(service_tracker_customizer_pt customizer) {
	customizer->handle = NULL;
	customizer->addingService = NULL;
	customizer->addedService = NULL;
	customizer->modifiedService = NULL;
	customizer->removedService = NULL;

	free(customizer);

	return CELIX_SUCCESS;
}

celix_status_t serviceTrackerCustomizer_getHandle(service_tracker_customizer_pt customizer, void **handle) {
	celix_status_t status = CELIX_SUCCESS;

	*handle = customizer->handle;

	return status;
}

celix_status_t serviceTrackerCustomizer_getAddingFunction(service_tracker_customizer_pt customizer, adding_callback_pt *function) {
	celix_status_t status = CELIX_SUCCESS;

	*function = customizer->addingService;

	return status;
}

celix_status_t serviceTrackerCustomizer_getAddedFunction(service_tracker_customizer_pt customizer, added_callback_pt *function) {
	celix_status_t status = CELIX_SUCCESS;

	*function = customizer->addedService;

	return status;
}

celix_status_t serviceTrackerCustomizer_getModifiedFunction(service_tracker_customizer_pt customizer, modified_callback_pt *function) {
	celix_status_t status = CELIX_SUCCESS;

	*function = customizer->modifiedService;

	return status;
}

celix_status_t serviceTrackerCustomizer_getRemovedFunction(service_tracker_customizer_pt customizer, removed_callback_pt *function) {
	celix_status_t status = CELIX_SUCCESS;

	*function = customizer->removedService;

	return status;
}
