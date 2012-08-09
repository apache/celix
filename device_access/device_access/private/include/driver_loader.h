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
 * driver_loader.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef DRIVER_LOADER_H_
#define DRIVER_LOADER_H_

#include "driver_locator.h"
#include "driver_attributes.h"

#define DRIVER_LOCATION_PREFIX "_DD_"

typedef struct driver_loader *driver_loader_t;

celix_status_t driverLoader_create(apr_pool_t *pool, BUNDLE_CONTEXT context, driver_loader_t *loader);

celix_status_t driverLoader_findDrivers(driver_loader_t loader, apr_pool_t *pool, ARRAY_LIST locators, PROPERTIES properties, ARRAY_LIST *driversIds);
celix_status_t driverLoader_findDriversForLocator(driver_loader_t loader, apr_pool_t *pool, driver_locator_service_t locator, PROPERTIES properties, ARRAY_LIST *driversIds);

celix_status_t driverLoader_loadDrivers(driver_loader_t loader, apr_pool_t *pool, ARRAY_LIST locators, ARRAY_LIST driverIds, ARRAY_LIST *references);
celix_status_t driverLoader_loadDriver(driver_loader_t loader, apr_pool_t *pool, ARRAY_LIST locators, char *driverId, ARRAY_LIST *references);
celix_status_t driverLoader_loadDriverForLocator(driver_loader_t loader, apr_pool_t *pool, driver_locator_service_t locator, char *driverId, ARRAY_LIST *references);

celix_status_t driverLoader_unloadDrivers(driver_loader_t loader, driver_attributes_t finalDriver);

#endif /* DRIVER_LOADER_H_ */
