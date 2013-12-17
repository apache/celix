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
 * driver_attributes.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_strings.h>

#include "driver_attributes.h"
#include "bundle.h"
#include "celixbool.h"
#include "driver_loader.h"
#include "device.h"
#include "constants.h"

struct driver_attributes {
	apr_pool_t *pool;

	bundle_pt bundle;
	service_reference_pt reference;
	driver_service_pt driver;
	bool dynamic;
};

celix_status_t driverAttributes_create(apr_pool_t *pool, service_reference_pt reference, driver_service_pt driver, driver_attributes_pt *attributes) {
	celix_status_t status = CELIX_SUCCESS;

	*attributes = apr_palloc(pool, sizeof(**attributes));
	if (!*attributes) {
		status = CELIX_ENOMEM;
	} else {
		bundle_pt bundle = NULL;
		bundle_archive_pt bundleArchive = NULL;
		status = serviceReference_getBundle(reference, &bundle);

		if (status == CELIX_SUCCESS) {
			status = bundle_getArchive(bundle, &bundleArchive);

			if (status == CELIX_SUCCESS) {
				(*attributes)->pool = pool;
				(*attributes)->reference = reference;
				(*attributes)->driver = driver;
				(*attributes)->bundle = bundle;

				char *location;
				status = bundleArchive_getLocation(bundleArchive, &location);
				if (status == CELIX_SUCCESS) {
					(*attributes)->dynamic = strncmp(location, DRIVER_LOCATION_PREFIX, 4) == 0 ? true : false;
				}

			}
		}
	}

	return status;
}

celix_status_t driverAttributes_getReference(driver_attributes_pt driverAttributes, service_reference_pt *reference) {
	*reference = driverAttributes->reference;

	return CELIX_SUCCESS;
}

celix_status_t driverAttributes_getDriverId(driver_attributes_pt driverAttributes, char **driverId) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
	status = serviceReference_getServiceRegistration(driverAttributes->reference, &registration);

	if (status == CELIX_SUCCESS) {
		properties_pt properties = NULL;
		status = serviceRegistration_getProperties(registration, &properties);

		if (status == CELIX_SUCCESS) {
			char *id_prop = properties_get(properties, "DRIVER_ID");

			if (!id_prop) {
				status = CELIX_ENOMEM;
			} else {
				*driverId = apr_pstrdup(driverAttributes->pool,id_prop);

				if (*driverId == NULL) {
					status = CELIX_ENOMEM;
				}
			}
		}
	}

	return status;
}

celix_status_t driverAttributes_match(driver_attributes_pt driverAttributes, service_reference_pt reference, int *match) {
	celix_status_t status = CELIX_SUCCESS;

	status = driverAttributes->driver->match(driverAttributes->driver->driver, reference, match);

	return status;
}

celix_status_t static get_property_from_registration(service_reference_pt ref, char *key, char **prop_value)  {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
	status = serviceReference_getServiceRegistration(ref, &registration);

	if (status == CELIX_SUCCESS) {
		properties_pt properties = NULL;
		status = serviceRegistration_getProperties(registration, &properties);

		if (status == CELIX_SUCCESS) {
			(*prop_value) = properties_get(properties, key);

			if (*prop_value == NULL) {
				status = CELIX_ENOMEM;
			}
		}
	}

	return status;
}


celix_status_t driverAttributes_isInUse(driver_attributes_pt driverAttributes, bool *inUse) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt references = NULL;
	status = bundle_getServicesInUse(driverAttributes->bundle, &references);
	if (status == CELIX_SUCCESS) {
		if (references == NULL || arrayList_size(references) == 0) {
			*inUse = false;
		} else {
			int i;
			for (i = 0; i < arrayList_size(references); i++) {
				service_reference_pt ref = arrayList_get(references, i);
				char *object = NULL;
				status = get_property_from_registration(ref, (char *) OSGI_FRAMEWORK_OBJECTCLASS, &object);

				if (status == CELIX_SUCCESS) {
					char *category = NULL;
					status = get_property_from_registration(ref, "DEVICE_CATEGORY", &category);

					if (status == CELIX_SUCCESS) {
						if ((object != NULL && strcmp(object, OSGI_DEVICEACCESS_DEVICE_SERVICE_NAME) == 0) || (category != NULL)) {
							*inUse = true;
						}
					}
				}
			}
		}
	}

	return status;
}

celix_status_t driverAttributes_attach(driver_attributes_pt driverAttributes, service_reference_pt reference, char **attach) {
	celix_status_t status = CELIX_SUCCESS;

	status = driverAttributes->driver->attach(driverAttributes->driver->driver, reference, attach);

	return status;
}

celix_status_t driverAttributes_tryUninstall(driver_attributes_pt driverAttributes) {
	celix_status_t status = CELIX_SUCCESS;

	bool inUse = false;

	status = driverAttributes_isInUse(driverAttributes, &inUse);
	if (status == CELIX_SUCCESS) {
		if (!inUse && driverAttributes->dynamic) {
			status = bundle_uninstall(driverAttributes->bundle);
		}
	}

	return status;
}
