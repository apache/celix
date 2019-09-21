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
 * refining_driver_device.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REFINING_DRIVER_DEVICE_H_
#define REFINING_DRIVER_DEVICE_H_

#include "device.h"

#define REFINING_DRIVER_SERVICE_NAME "refining_driver_device_service"
#define REFINING_DRIVER_DEVICE_CATEGORY "word"
#define REFINING_DRIVER_DEVICE_SERVIC_NAME "refining_driver_device"

typedef struct refining_driver_device *refining_driver_device_pt;

struct refining_driver_device_service {
	struct device_service deviceService; /*NOTE: base_driver_device_service is a device_service.*/
	refining_driver_device_pt refiningDriverDevice;
	celix_status_t (*getNextWord)(refining_driver_device_pt refiningDriverDevice, char **c);
};

typedef struct refining_driver_device_service * refining_driver_device_service_pt;

#endif /* REFINING_DRIVER_DEVICE_H_ */
