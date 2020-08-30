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
 * driver_locator_private.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DRIVER_LOCATOR_PRIVATE_H_
#define DRIVER_LOCATOR_PRIVATE_H_

#include "driver_locator.h"

struct driver_locator {
    char *path;
    array_list_pt drivers;
};

celix_status_t driverLocator_findDrivers(driver_locator_pt locator, properties_pt props, array_list_pt *drivers);
celix_status_t driverLocator_loadDriver(driver_locator_pt locator, char *id, char **driver);

#endif /* DRIVER_LOCATOR_PRIVATE_H_ */
