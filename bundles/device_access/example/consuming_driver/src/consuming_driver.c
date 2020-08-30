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
 * consuming_driver.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <service_tracker.h>
#include <service_reference.h>

#include "celixbool.h"
#include "consuming_driver_private.h"
#include "refining_driver_device.h"


struct consuming_driver {
	bundle_context_pt context;
	array_list_pt references;
};

celix_status_t consumingDriver_destroy(consuming_driver_pt driver) {
	printf("CONSUMING_DRIVER: cleanup\n");
	if (driver->references != NULL) {
		array_list_iterator_pt iterator = arrayListIterator_create(driver->references);
		while (arrayListIterator_hasNext(iterator)) {
			service_reference_pt reference = arrayListIterator_next(iterator);
			bool result;
			bundleContext_ungetService(driver->context, reference, &result);
		}
		arrayListIterator_destroy(iterator);

		arrayList_destroy(driver->references);
		driver->references=NULL;
	}


	return CELIX_SUCCESS;
}

celix_status_t consumingDriver_create(bundle_context_pt context, consuming_driver_pt *driver) {
	celix_status_t status = CELIX_SUCCESS;
	(*driver) = calloc(1, sizeof(**driver));
	if ((*driver) != NULL) {
		(*driver)->context=context;
		(*driver)->references=NULL;

		status = arrayList_create(&(*driver)->references);
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t consumingDriver_createService(consuming_driver_pt driver, driver_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = calloc(1, sizeof(**service));
	if ((*service) != NULL) {
		(*service)->driver = driver;
		(*service)->attach = consumingDriver_attach;
		(*service)->match = consumingDriver_match;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t consumingDriver_attach(void * driverHandler, service_reference_pt reference, char **result) {
	printf("CONSUMING_DRIVER: attached called\n");
	celix_status_t status = CELIX_SUCCESS;
	consuming_driver_pt driver = driverHandler;
	(*result) = NULL;
	refining_driver_device_service_pt device_service = NULL;

	status = bundleContext_getService(driver->context, reference, (void **)&device_service);
	if (status == CELIX_SUCCESS) {
		arrayList_add(driver->references, reference);
		//consume the device
		for (int i=0; i<15; i++) {
			char *str = NULL;
			device_service->getNextWord(device_service->refiningDriverDevice, &str);
			printf("CONSUMING_DEVICE: Word Device result is %s\n", str);
		}
	}
	return status;
}

celix_status_t consumingDriver_match(void *driverHandler, service_reference_pt reference, int *value) {
	printf("CONSUMING_DRIVER: match called\n");
	int match=0;
	celix_status_t status = CELIX_SUCCESS;

    const char* category = NULL;
    status = serviceReference_getProperty(reference, OSGI_DEVICEACCESS_DEVICE_CATEGORY, &category);
    if (status == CELIX_SUCCESS) {
        if (strcmp(category, REFINING_DRIVER_DEVICE_CATEGORY) == 0) {
            match = 10;
        }
    }

	(*value) = match;
	return status;
}

