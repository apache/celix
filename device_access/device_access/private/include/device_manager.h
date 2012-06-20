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

#ifndef DEVICE_MANAGER_H_
#define DEVICE_MANAGER_H_

#include "driver_loader.h"

typedef struct device_manager *device_manager_t;

celix_status_t deviceManager_create(apr_pool_t *pool, BUNDLE_CONTEXT context, device_manager_t *manager);
celix_status_t deviceManager_destroy(device_manager_t manager);

celix_status_t deviceManager_matchAttachDriver(device_manager_t manager, apr_pool_t *attachPool, driver_loader_t loader,
			ARRAY_LIST driverIds, ARRAY_LIST included, ARRAY_LIST excluded, void *service, SERVICE_REFERENCE reference);
celix_status_t deviceManager_noDriverFound(device_manager_t manager, void *service, SERVICE_REFERENCE reference);

celix_status_t deviceManager_locatorAdded(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_locatorModified(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_locatorRemoved(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_deviceAdded(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_deviceModified(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_deviceRemoved(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_driverAdded(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_driverModified(void * handle, SERVICE_REFERENCE ref, void * service);
celix_status_t deviceManager_driverRemoved(void * handle, SERVICE_REFERENCE ref, void * service);

celix_status_t deviceManager_getBundleContext(device_manager_t manager, BUNDLE_CONTEXT *context);

// celix_status_t deviceManager_match(device_manager_t manager, ...);

#endif /* DEVICE_MANAGER_H_ */
