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

#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>

#include <celix_errno.h>
#include <bundle_activator.h>
#include <bundle_context.h>
#include <celixbool.h>
#include <device.h>

#include "refining_driver_private.h"

struct refining_driver_bundle_instance {
	BUNDLE_CONTEXT context;
	apr_pool_t * pool;
	SERVICE_REGISTRATION registration;
};

typedef struct refining_driver_bundle_instance *refining_driver_bundle_instance_t;

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	printf("REFINING_DRIVER: creating bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool;
	status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		refining_driver_bundle_instance_t instance = apr_palloc(pool, sizeof(*instance));
		if (instance != NULL) {
			instance->context = context;
			instance->pool = pool;
			instance->registration = NULL;
			(*userData) = instance;
		} else {
			status = CELIX_ENOMEM;
		}
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	printf("REFINING_DRIVER: starting bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_bundle_instance_t bi = userData;

	refining_driver_t driver = NULL;
	status = refiningDriver_create(context, bi->pool, &driver);
	if (status == CELIX_SUCCESS) {
		driver_service_t service = NULL;
		status = refiningDriver_createService(driver, &service);
		if (status == CELIX_SUCCESS) {
			PROPERTIES props = properties_create();
			properties_set(props, "DRIVER_ID", REFINING_DRIVER_ID);
			status = bundleContext_registerService(context, DRIVER_SERVICE_NAME, service, props, &bi->registration);
		}
	}

	if (status == CELIX_SUCCESS) {
		printf("REFINING_DRIVER: registered driver service.\n");
	} else {
		printf("REFINING_DRIVER: Could not register driver service. Get error %s\n", celix_strerror(status));
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	printf("REFINING_DRIVER: stopping bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_bundle_instance_t bi = userData;

	if (bi->registration != NULL) {
		serviceRegistration_unregister(bi->registration);
		printf("REFINING_DRIVER: unregistered driver service\n");
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	printf("REFINING_DRIVER: destroying bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_bundle_instance_t bi = userData;
	return status;
}



