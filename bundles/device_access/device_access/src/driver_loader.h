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
 * driver_loader.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DRIVER_LOADER_H_
#define DRIVER_LOADER_H_

#include "driver_locator.h"
#include "driver_attributes.h"

#define DRIVER_LOCATION_PREFIX "_DD_"

typedef struct driver_loader *driver_loader_pt;

celix_status_t driverLoader_create(bundle_context_pt context, driver_loader_pt *loader);
celix_status_t driverLoader_destroy(driver_loader_pt *loader);

celix_status_t driverLoader_findDrivers(driver_loader_pt loader, array_list_pt locators, properties_pt properties, array_list_pt *driversIds);
celix_status_t driverLoader_findDriversForLocator(driver_loader_pt loader, driver_locator_service_pt locator, properties_pt properties, array_list_pt *driversIds);

celix_status_t driverLoader_loadDrivers(driver_loader_pt loader, array_list_pt locators, array_list_pt driverIds, array_list_pt *references);
celix_status_t driverLoader_loadDriver(driver_loader_pt loader, array_list_pt locators, char *driverId, array_list_pt *references);
celix_status_t driverLoader_loadDriverForLocator(driver_loader_pt loader, driver_locator_service_pt locator, char *driverId, array_list_pt *references);

celix_status_t driverLoader_unloadDrivers(driver_loader_pt loader, driver_attributes_pt finalDriver);

#endif /* DRIVER_LOADER_H_ */
