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
 * driver_attributes.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DRIVER_ATTRIBUTES_H_
#define DRIVER_ATTRIBUTES_H_

#include "driver.h"

typedef struct driver_attributes *driver_attributes_pt;

celix_status_t driverAttributes_create(service_reference_pt reference, driver_service_pt driver, driver_attributes_pt *attributes);
celix_status_t driverAttributes_destroy(driver_attributes_pt attributes);

celix_status_t driverAttributes_getReference(driver_attributes_pt driverAttributes, service_reference_pt *reference);
celix_status_t driverAttributes_getDriverId(driver_attributes_pt driverAttributes, char **driverId);

celix_status_t driverAttributes_match(driver_attributes_pt driverAttributes, service_reference_pt reference, int *match);
celix_status_t driverAttributes_attach(driver_attributes_pt driverAttributes, service_reference_pt reference, char **attach);

celix_status_t driverAttributes_isInUse(driver_attributes_pt driverAttributes, bool *inUse);

celix_status_t driverAttributes_tryUninstall(driver_attributes_pt driverAttributes);

#endif /* DRIVER_ATTRIBUTES_H_ */
