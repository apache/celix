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
 * activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "greeting_impl.h"
#include "greeting_service.h"
#include "service_registration.h"

struct greetingActivator {
	SERVICE_REGISTRATION reg;
	apr_pool_t *pool;
};

typedef struct greetingActivator *greeting_activator_t;

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	greeting_activator_t activator;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct greetingActivator));
		if (userData) {
			activator = *userData;
			activator->reg = NULL;
			activator->pool = pool;
		} else {
			status = CELIX_ENOMEM;
		}
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;

	greeting_activator_t act = (greeting_activator_t) userData;

	greeting_service_t greetingService = apr_palloc(act->pool, sizeof(*greetingService));

	if (greetingService) {
		greetingService->instance = apr_palloc(act->pool, sizeof(*greetingService->instance));
		if (greetingService->instance) {
			greetingService->instance->name = GREETING_SERVICE_NAME;
			greetingService->greeting_sayHello = greeting_sayHello;

			status = bundleContext_registerService(context, GREETING_SERVICE_NAME, greetingService, NULL, &act->reg);
		} else {
			status = CELIX_ENOMEM;
		}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	greeting_activator_t act = (greeting_activator_t) userData;
	serviceRegistration_unregister(act->reg);
	act->reg = NULL;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
