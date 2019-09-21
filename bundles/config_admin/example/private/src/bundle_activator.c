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

#include <stdlib.h>
#include <pthread.h>

#include "celix_constants.h"
#include <bundle_context.h>
#include <service_tracker.h>

#include "example.h"
#include "managed_service.h"

struct activator {
	bundle_context_pt context;
	example_pt example;
	managed_service_service_pt managed;
	service_registration_pt managedServiceRegistry;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = calloc(1, sizeof(struct activator));
	if (activator != NULL) {
		(*userData) = activator;
		activator->example = NULL;
		example_create(&activator->example);
		activator->managed = NULL;
		activator->managedServiceRegistry = NULL;

	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	example_start(activator->example);
    
    activator->managed = calloc(1, sizeof(*activator->managed));
    properties_pt props = properties_create();

	if (activator->managed != NULL && props != NULL) {
        properties_set(props, "service.pid", "org.example.config.admin"); 
        activator->managed->managedService = (void *)activator->example;
        activator->managed->updated = (void *)example_updated;
		bundleContext_registerService(context, (char *)  MANAGED_SERVICE_SERVICE_NAME, activator->managed, props, &activator->managedServiceRegistry);
	}

	return status;
}

celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	if (activator->managed != NULL) {
		serviceRegistration_unregister(activator->managedServiceRegistry);
	}
	example_stop(activator->example);

	return status;
}

celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	example_destroy(activator->example);
	if (activator->managed != NULL) {
		free(activator->managed);
	}

	return status;
}
