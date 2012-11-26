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

#include "base_driver_private.h"
#include "base_driver_device.h"

struct base_driver_bundle_instance {
	bundle_context_t context;
	apr_pool_t * pool;
	ARRAY_LIST serviceRegistrations;
};

typedef struct base_driver_bundle_instance *base_driver_bundle_instance_t;

celix_status_t bundleActivator_create(bundle_context_t context, void **userData) {
	printf("BASE_DRIVER: creating bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool;
	status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		base_driver_bundle_instance_t instance = apr_palloc(pool, sizeof(*instance));
		if (instance != NULL) {
			instance->context = context;
			instance->pool = pool;
			status = arrayList_create(pool, &instance->serviceRegistrations);
			if (status == CELIX_SUCCESS) {
				(*userData) = instance;
			}
		} else {
			status = CELIX_ENOMEM;
		}
	}
	return status;
}

static celix_status_t bundleActivator_registerBaseDriverDevice(base_driver_bundle_instance_t bi, char *serial) {
	celix_status_t status = CELIX_SUCCESS;
	base_driver_device_t device = NULL;
	status = baseDriver_create(bi->pool, &device);
	if (status == CELIX_SUCCESS) {
		base_driver_device_service_t service = NULL;
		status = baseDriver_createService(device, &service);
		if (status == CELIX_SUCCESS) {
			PROPERTIES props = properties_create();
			properties_set(props, DEVICE_CATEGORY, BASE_DRIVER_DEVICE_CATEGORY);
			properties_set(props, DEVICE_SERIAL, serial);
			SERVICE_REGISTRATION service_registration = NULL;
			status = bundleContext_registerService(bi->context, DEVICE_SERVICE_NAME, service, props, &service_registration);
			if (status == CELIX_SUCCESS) {
				arrayList_add(bi->serviceRegistrations, service_registration);
//				service_registration = NULL;
//				status = bundleContext_registerService(bi->context, BASE_DRIVER_SERVICE_NAME, service, NULL, &service_registration);
//				if (status == CELIX_SUCCESS) {
//					arrayList_add(bi->serviceRegistrations, service_registration);
//				}
			}
		}
	}

	if (status == CELIX_SUCCESS) {
		printf("BASE_DRIVER: Successfully registered device service with serial %s.\n", serial);
	} else {
		printf("BASE_DRIVER: Unsuccessfully registered device service with serial %s. Got error: %s\n",
				serial, celix_strerror(status));
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_t context) {
	printf("BASE_DRIVER: starting bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	base_driver_bundle_instance_t bundleInstance = userData;
	status = bundleActivator_registerBaseDriverDevice(bundleInstance, "0001");
//	if (status == CELIX_SUCCESS) {
//		status = bundleActivator_registerBaseDriverDevice(bundleInstance, "0002");
//	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_t context) {
	printf("BASE_DRIVER: stopping bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	base_driver_bundle_instance_t bundleInstance = userData;

	ARRAY_LIST_ITERATOR iterator = arrayListIterator_create(bundleInstance->serviceRegistrations);
	while (arrayListIterator_hasNext(iterator)) {
		SERVICE_REGISTRATION reg = arrayListIterator_next(iterator);
		printf("BASE_DRIVER: unregistering service\n");
		celix_status_t unregStatus = serviceRegistration_unregister(reg);
		if (unregStatus != CELIX_SUCCESS) {
			status = CELIX_ILLEGAL_STATE;
			fprintf(stderr, "Cannot unregister service. Got error %s\n", celix_strerror(unregStatus));
		} else {
			printf("BASE_DRIVER: unregistered base device service\n");
		}
	}
	arrayListIterator_destroy(iterator);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_t context) {
	printf("BASE_DRIVER: destroying bundle\n");
	celix_status_t status = CELIX_SUCCESS;
	base_driver_bundle_instance_t bundleInstance = userData;

	arrayList_destroy(bundleInstance->serviceRegistrations);
	return status;
}

