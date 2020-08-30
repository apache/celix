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
 * driver_loader.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "driver_loader.h"
#include "bundle_context.h"
#include "bundle.h"

struct driver_loader {
	bundle_context_pt context;
	array_list_pt loadedDrivers;
};

celix_status_t driverLoader_create(bundle_context_pt context, driver_loader_pt *loader) {
	celix_status_t status = CELIX_SUCCESS;

	*loader = calloc(1, sizeof(**loader));
	if (!*loader) {
		status = CELIX_ENOMEM;
	} else {
		(*loader)->context = context;
		(*loader)->loadedDrivers = NULL;
		status = arrayList_create(&(*loader)->loadedDrivers);
	}

	return status;
}

celix_status_t driverLoader_destroy(driver_loader_pt *loader) {
	if((*loader) != NULL){
		arrayList_destroy((*loader)->loadedDrivers);
		free((*loader));
		(*loader)=NULL;
	}
	return CELIX_SUCCESS;
}

celix_status_t driverLoader_findDrivers(driver_loader_pt loader, array_list_pt locators, properties_pt properties, array_list_pt *driversIds) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(driversIds);

	int i;
	for (i = 0; i < arrayList_size(locators); i++) {
		array_list_pt drivers;
		driver_locator_service_pt locator = arrayList_get(locators, i);

		status = driverLoader_findDriversForLocator(loader, locator, properties, &drivers);
		if (status == CELIX_SUCCESS) {
			arrayList_addAll(*driversIds, drivers);
		}
		arrayList_destroy(drivers);
	}

	return status;
}

celix_status_t driverLoader_findDriversForLocator(driver_loader_pt loader, driver_locator_service_pt locator, properties_pt properties, array_list_pt *driversIds) {
	celix_status_t status = CELIX_SUCCESS;

	status = locator->findDrivers(locator->locator, properties, driversIds);

	return status;
}

celix_status_t driverLoader_loadDrivers(driver_loader_pt loader, array_list_pt locators, array_list_pt driverIds, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;
	status = arrayList_create(references);
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(driverIds); i++) {
			array_list_pt refs = NULL;
			char *id = arrayList_get(driverIds, i);

			status = driverLoader_loadDriver(loader, locators, id, &refs);
			if (status == CELIX_SUCCESS) {
				arrayList_addAll(*references, refs);
			}
			if (refs != NULL) {
				arrayList_destroy(refs);
			}
		}
	}

	return status;
}

celix_status_t driverLoader_loadDriver(driver_loader_pt loader, array_list_pt locators, char *driverId, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;
	status = arrayList_create(references);
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(locators); i++) {
			array_list_pt refs = NULL;
			driver_locator_service_pt locator = arrayList_get(locators, i);

			status = driverLoader_loadDriverForLocator(loader, locator, driverId, &refs);
			if (status == CELIX_SUCCESS) {
				arrayList_addAll(*references, refs);
			}

			if (refs != NULL) {
				arrayList_destroy(refs);
			}
		}
	}

	return status;
}

celix_status_t driverLoader_loadDriverForLocator(driver_loader_pt loader, driver_locator_service_pt locator, char *driverId, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;
	//The list is created in the bundle_getRegisteredServices chain
	//arrayList_create(references);

	char *filename = NULL;
	status = locator->loadDriver(locator->locator, driverId, &filename);
	if (status == CELIX_SUCCESS) {
		bundle_pt bundle = NULL;
		int length = strlen(DRIVER_LOCATION_PREFIX) + strlen(driverId);
		char location[length+2];
		snprintf(location, length+2, "%s%s", DRIVER_LOCATION_PREFIX, driverId);
		status = bundleContext_installBundle2(loader->context, location, filename, &bundle);
		if (status == CELIX_SUCCESS) {
			status = bundle_start(bundle);
			if (status == CELIX_SUCCESS) {
				status = bundle_getRegisteredServices(bundle, references);
				if (status == CELIX_SUCCESS) {
					arrayList_addAll(loader->loadedDrivers, *references);
				}
			}
		}
	}

	return status;
}

celix_status_t driverLoader_unloadDrivers(driver_loader_pt loader, driver_attributes_pt finalDriver) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_pt finalReference = NULL;
	if (finalDriver != NULL) {
		status = driverAttributes_getReference(finalDriver, &finalReference);
	}
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(loader->loadedDrivers); i++) {
			service_reference_pt reference = arrayList_get(loader->loadedDrivers, i);
			bool equal = false;
			status = serviceReference_equals(reference, finalReference, &equal);
			if (status == CELIX_SUCCESS && !equal) {
				bundle_pt bundle = NULL;
				status = serviceReference_getBundle(reference, &bundle);
				if (status == CELIX_SUCCESS) {
					bundle_uninstall(bundle); // Ignore status
				}
			}
		}
	}

	return status;
}
