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
 * refining_driver_private.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REFINING_DRIVER_PRIVATE_H_
#define REFINING_DRIVER_PRIVATE_H_

#include <celix_errno.h>
#include <service_reference.h>
#include <driver.h>

#include "refining_driver_device.h"
#include "base_driver_device.h"

#define REFINING_DRIVER_ID "REFINING_DRIVER"

typedef struct refining_driver *refining_driver_pt;

celix_status_t refiningDriver_create(bundle_context_pt context, refining_driver_pt *driver);
celix_status_t refiningDriver_destroy(refining_driver_pt driver);

celix_status_t refiningDriver_createService(refining_driver_pt driver, driver_service_pt *service);

celix_status_t refiningDriver_createDevice(refining_driver_pt driver, service_reference_pt reference, base_driver_device_service_pt baseDevice, refining_driver_device_pt *device);
celix_status_t refiningDriver_destroyDevice(refining_driver_device_pt device);


celix_status_t refiningDriver_attach(void *driver, service_reference_pt reference, char **result);
celix_status_t refiningDriver_match(void *driver, service_reference_pt reference, int *value);


celix_status_t refiningDriverDevice_noDriverFound(device_pt device);
celix_status_t refiningDriverDevice_createService(refining_driver_device_pt, refining_driver_device_service_pt *service);
celix_status_t refiningDriverDevice_destroyService(refining_driver_device_service_pt service);
celix_status_t refiningDriverDevice_getNextWord(refining_driver_device_pt refiningDriverDevice, char **word);

#endif /* REFINING_DRIVER_PRIVATE_H_ */
