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
 * base_driver_private.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BASE_DRIVER_PRIVATE_H_
#define BASE_DRIVER_PRIVATE_H_

#include "base_driver_device.h"

celix_status_t baseDriver_create(base_driver_device_pt *service);
celix_status_t baseDriver_createService(base_driver_device_pt device, base_driver_device_service_pt *service);

celix_status_t baseDriver_noDriverFound(device_pt device);

celix_status_t baseDriver_getNextChar(base_driver_device_pt service, char *c);

celix_status_t baseDriver_destroy(base_driver_device_pt device);
celix_status_t baseDriver_destroyService(base_driver_device_service_pt service);

#endif /* BASE_DRIVER_PRIVATE_H_ */
