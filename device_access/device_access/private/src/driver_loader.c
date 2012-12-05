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
 * driver_loader.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_general.h>
#include <apr_strings.h>

#include "driver_loader.h"
#include "bundle_context.h"
#include "bundle.h"

struct driver_loader {
	apr_pool_t *pool;

	bundle_context_t context;
	array_list_t loadedDrivers;
};

static apr_status_t driverLoader_destroy(void *loaderP);

celix_status_t driverLoader_create(apr_pool_t *pool, bundle_context_t context, driver_loader_t *loader) {
	celix_status_t status = CELIX_SUCCESS;

	*loader = apr_palloc(pool, sizeof(**loader));
	if (!*loader) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *loader, driverLoader_destroy);

		(*loader)->context = context;
		(*loader)->loadedDrivers = NULL;
		(*loader)->pool = pool;
		status = arrayList_create(pool, &(*loader)->loadedDrivers);
	}

	return status;
}

apr_status_t driverLoader_destroy(void *loaderP) {
	driver_loader_t matcher = loaderP;
	arrayList_destroy(matcher->loadedDrivers);
	return APR_SUCCESS;
}

celix_status_t driverLoader_findDrivers(driver_loader_t loader, apr_pool_t *pool, array_list_t locators, properties_t properties, array_list_t *driversIds) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(pool, driversIds);

	int i;
	for (i = 0; i < arrayList_size(locators); i++) {
		array_list_t drivers;
		driver_locator_service_t locator = arrayList_get(locators, i);

		apr_pool_t *spool;
		apr_status_t aprStatus = apr_pool_create(&spool, pool);
		if (aprStatus != APR_SUCCESS) {
			status = CELIX_ENOMEM;
		} else {
			status = driverLoader_findDriversForLocator(loader, spool, locator, properties, &drivers);
			if (status == CELIX_SUCCESS) {
				arrayList_addAll(*driversIds, drivers);
			}
			arrayList_destroy(drivers);
			apr_pool_destroy(spool);
		}
	}

	return status;
}

celix_status_t driverLoader_findDriversForLocator(driver_loader_t loader, apr_pool_t *pool, driver_locator_service_t locator, properties_t properties, array_list_t *driversIds) {
	celix_status_t status = CELIX_SUCCESS;

	status = locator->findDrivers(locator->locator, pool, properties, driversIds);

	return status;
}

celix_status_t driverLoader_loadDrivers(driver_loader_t loader, apr_pool_t *pool, array_list_t locators, array_list_t driverIds, array_list_t *references) {
	celix_status_t status = CELIX_SUCCESS;
	status = arrayList_create(pool, references);
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(driverIds); i++) {
			array_list_t refs = NULL;
			char *id = arrayList_get(driverIds, i);

			apr_pool_t *spool;
			apr_status_t aprStatus = apr_pool_create(&spool, pool);
			if (aprStatus != APR_SUCCESS) {
				status = CELIX_ENOMEM;
			} else {
				status = driverLoader_loadDriver(loader, pool, locators, id, &refs);
				if (status == CELIX_SUCCESS) {
					arrayList_addAll(*references, refs);
				}
				if (refs != NULL) {
					arrayList_destroy(refs);
				}
				apr_pool_destroy(spool);
			}
		}
	}

	return status;
}

celix_status_t driverLoader_loadDriver(driver_loader_t loader, apr_pool_t *pool, array_list_t locators, char *driverId, array_list_t *references) {
	celix_status_t status = CELIX_SUCCESS;
	status = arrayList_create(pool, references);
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(locators); i++) {
			array_list_t refs = NULL;
			driver_locator_service_t locator = arrayList_get(locators, i);

			apr_pool_t *spool;
			apr_status_t aprStatus = apr_pool_create(&spool, pool);
			if (aprStatus != APR_SUCCESS) {
				status = CELIX_ENOMEM;
			} else {
				status = driverLoader_loadDriverForLocator(loader, pool, locator, driverId, &refs);
				if (status == CELIX_SUCCESS) {
					arrayList_addAll(*references, refs);
				}
				if (refs != NULL) {
					arrayList_destroy(refs);
				}
			}
			apr_pool_destroy(spool);


		}
	}

	return status;
}

celix_status_t driverLoader_loadDriverForLocator(driver_loader_t loader, apr_pool_t *pool, driver_locator_service_t locator, char *driverId, array_list_t *references) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(loader->pool, references);

	apr_pool_t *loadPool;
	apr_status_t aprStatus = apr_pool_create(&loadPool, pool);
	if (aprStatus != APR_SUCCESS) {
		status = CELIX_ENOMEM;
	} else {
		char *filename = NULL;
		status = locator->loadDriver(locator->locator, loadPool, driverId, &filename);
		if (status == CELIX_SUCCESS) {
			bundle_t bundle = NULL;
			apr_pool_t *bundlePool;
			status = bundleContext_getMemoryPool(loader->context, &bundlePool);
			if (status == CELIX_SUCCESS) {
				char *location = apr_pstrcat(bundlePool, DRIVER_LOCATION_PREFIX, driverId, NULL);
				status = bundleContext_installBundle2(loader->context, location, filename, &bundle);
				if (status == CELIX_SUCCESS) {
					status = bundle_start(bundle, 0);
					if (status == CELIX_SUCCESS) {
						status = bundle_getRegisteredServices(bundle, pool, references);
						if (status == CELIX_SUCCESS) {
							arrayList_addAll(loader->loadedDrivers, *references);
						}
					}
				}
			}
		}
		apr_pool_destroy(loadPool);
	}

	return status;
}

celix_status_t driverLoader_unloadDrivers(driver_loader_t loader, driver_attributes_t finalDriver) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_t finalReference = NULL;
	if (finalDriver != NULL) {
		status = driverAttributes_getReference(finalDriver, &finalReference);
	}
	if (status == CELIX_SUCCESS) {
		int i;
		for (i = 0; i < arrayList_size(loader->loadedDrivers); i++) {
			service_reference_t reference = arrayList_get(loader->loadedDrivers, i);
			bool equal = false;
			status = serviceReference_equals(reference, finalReference, &equal);
			if (status == CELIX_SUCCESS && !equal) {
				bundle_t bundle = NULL;
				status = serviceReference_getBundle(reference, &bundle);
				if (status == CELIX_SUCCESS) {
					bundle_uninstall(bundle); // Ignore status
				}
			}
		}
	}

	return status;
}
