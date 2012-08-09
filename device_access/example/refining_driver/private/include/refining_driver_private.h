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
 * refining_driver_private.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
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

typedef struct refining_driver *refining_driver_t;

celix_status_t refiningDriver_create(BUNDLE_CONTEXT context, apr_pool_t *pool, refining_driver_t *driver);
celix_status_t refiningDriver_destroy(refining_driver_t driver);

celix_status_t refiningDriver_createService(refining_driver_t driver, driver_service_t *service);

celix_status_t refiningDriver_createDevice(refining_driver_t driver, SERVICE_REFERENCE reference, base_driver_device_service_t baseDevice, refining_driver_device_t *device);
celix_status_t refiningDriver_destroyDevice(refining_driver_device_t device);


celix_status_t refiningDriver_attach(void *driver, SERVICE_REFERENCE reference, char **result);
celix_status_t refiningDriver_match(void *driver, SERVICE_REFERENCE reference, int *value);


celix_status_t refiningDriverDevice_noDriverFound(device_t device);
celix_status_t refiningDriverDevice_createService(refining_driver_device_t, refining_driver_device_service_t *service);
celix_status_t refiningDriverDevice_getNextWord(refining_driver_device_t refiningDriverDevice, apr_pool_t *pool, char **word);

#endif /* REFINING_DRIVER_PRIVATE_H_ */
