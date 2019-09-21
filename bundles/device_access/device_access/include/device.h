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
 * device.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include "celix_errno.h"

#define OSGI_DEVICEACCESS_DEVICE_CATEGORY	"DEVICE_CATEGORY"
#define OSGI_DEVICEACCESS_DEVICE_SERIAL	"DEVICE_SERIAL"

#define OSGI_DEVICEACCESS_DEVICE_SERVICE_NAME "device"

static const int OSGI_DEVICEACCESS_DEVICE_MATCH_NONE	= 0;

typedef struct device * device_pt;

struct device_service {
	device_pt device;
	celix_status_t (*noDriverFound)(device_pt device);
};

typedef struct device_service * device_service_pt;

#endif /* DEVICE_H_ */
