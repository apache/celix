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
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>

#include <celix_errno.h>
#include <bundle_activator.h>
#include <bundle_context.h>
#include <celixbool.h>
#include <device.h>

#include "consuming_driver_private.h"

struct consuming_driver_bundle_instance {
	bundle_context_pt context;
	apr_pool_t * pool;
	service_registration_pt registration;
};

typedef struct consuming_driver_bundle_instance *consuming_driver_bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	printf("CONSUMING_DRIVER: creating bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool;
	status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		consuming_driver_bundle_instance_pt instance = apr_palloc(pool, sizeof(*instance));
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

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	printf("CONSUMING_DRIVER: starting bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	consuming_driver_bundle_instance_pt bi = userData;

	consuming_driver_pt driver = NULL;
	status = consumingDriver_create(context, bi->pool, &driver);
	if (status == CELIX_SUCCESS) {
		driver_service_pt service = NULL;
		status = consumingDriver_createService(driver, &service);
		if (status == CELIX_SUCCESS) {
			properties_pt props = properties_create();
			properties_set(props, "DRIVER_ID", CONSUMING_DRIVER_ID);
			status = bundleContext_registerService(context, OSGI_DEVICEACCESS_DRIVER_SERVICE_NAME, service, props, &bi->registration);
		}
	}

	if (status == CELIX_SUCCESS) {
		printf("CONSUMING_DRIVER: registered driver service.\n");
	} else {
		char error[256];
		printf("CONSUMING_DRIVER: Could not register driver service. Get error %s\n", celix_strerror(status, error, 256));
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	printf("CONSUMING_DRIVER: stopping bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	consuming_driver_bundle_instance_pt bi = userData;

	if (bi->registration != NULL) {
		serviceRegistration_unregister(bi->registration);
		printf("CONSUMING_DRIVER: unregistered driver service\n");
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	printf("CONSUMING_DRIVER: destroying bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	consuming_driver_bundle_instance_pt bi = userData;
	return status;
}



