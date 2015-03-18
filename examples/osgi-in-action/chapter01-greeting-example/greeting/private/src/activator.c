#include <sys/cdefs.h>/**
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
 * activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "greeting_impl.h"

struct greetingActivator {
	service_registration_pt reg;
	greeting_service_pt greetingService;
};

typedef struct greetingActivator *greeting_activator_pt;

celix_status_t bundleActivator_create(bundle_context_pt  __attribute__((unused)) context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	greeting_activator_pt activator;
	*userData = calloc(1, sizeof(struct greetingActivator));
	if (*userData) {
		activator = *userData;
		activator->reg = NULL;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status;

	greeting_activator_pt act = (greeting_activator_pt) userData;

	act->greetingService = calloc(1, sizeof(*act->greetingService));

	if (act->greetingService) {
		act->greetingService->instance = calloc(1, sizeof(*act->greetingService->instance));
		if (act->greetingService->instance) {
			act->greetingService->instance->name = GREETING_SERVICE_NAME;
			act->greetingService->greeting_sayHello = greeting_sayHello;

			status = bundleContext_registerService(context, GREETING_SERVICE_NAME, act->greetingService, NULL, &act->reg);
		} else {
			status = CELIX_ENOMEM;
		}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	celix_status_t status = CELIX_SUCCESS;

	greeting_activator_pt act = (greeting_activator_pt) userData;

	serviceRegistration_unregister(act->reg);
	act->reg = NULL;

	free(act->greetingService->instance);
	free(act->greetingService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	free(userData);
	return CELIX_SUCCESS;
}
