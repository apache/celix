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
 * echo_server_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <unistd.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_tracker.h"

#include "math_service.h"
#include "benchmark.h"
#include "benchmark_service.h"
#include "frequency_service.h"

static celix_status_t addingService(void * handle, service_reference_pt reference, void **service);
static celix_status_t addedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t modifiedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t removedService(void * handle, service_reference_pt reference, void * service);

struct activator {
	bundle_context_pt context;
	benchmark_pt benchmark;
	benchmark_service_pt mathService;
	service_tracker_customizer_pt customizer;
	service_tracker_pt tracker;
	service_registration_pt registration;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator * activator = malloc(sizeof(*activator));
	activator->context=context;
	activator->benchmark=NULL;
	activator->mathService = NULL;
	activator->customizer = NULL;
	activator->tracker=NULL;
	activator->registration = NULL;

	*userData = activator;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = userData;

	status = benchmark_create(&activator->benchmark);
	serviceTrackerCustomizer_create(activator, addingService, addedService, modifiedService, removedService, &activator->customizer);

	char filter[128];
	sprintf(filter, "(&(objectClass=%s)(benchmark=%s))", MATH_SERVICE_NAME, benchmark_getName(activator->benchmark));

	serviceTracker_createWithFilter(context, filter, activator->customizer, &activator->tracker);
	serviceTracker_open(activator->tracker);

	activator->mathService = malloc(sizeof(*activator->mathService));
	activator->mathService->handler = (void *)activator->benchmark;
	activator->mathService->name=(void *)benchmark_getName;
	activator->mathService->getSampleFactor=(void *)benchmark_getSampleFactor;
	activator->mathService->run=(void *)benchmark_run;

	status = bundleContext_registerService(activator->context, BENCHMARK_SERVICE_NAME, activator->mathService, NULL, &activator->registration);

	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	struct activator * activator = userData;

	serviceTracker_close(activator->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct activator * activator = userData;

	benchmark_destroy(activator->benchmark);
	activator->benchmark=NULL;

	return CELIX_SUCCESS;
}

static celix_status_t addingService(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	status = bundleContext_getService(activator->context, reference, service);
	return status;

}
static celix_status_t addedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	benchmark_addMathService(activator->benchmark, service);
	return status;
}
static celix_status_t modifiedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	return status;
}

static celix_status_t removedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	benchmark_removeMathService(activator->benchmark, service);
	return status;
}
