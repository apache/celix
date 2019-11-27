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
 * activator.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "driver_locator_private.h"

static const char *DEFAULT_LOCATOR_PATH = "drivers";

struct bundle_instance {
	driver_locator_service_pt service;
	driver_locator_pt locator;
	service_registration_pt locatorRegistration;
};

typedef struct bundle_instance *bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	(*userData) = calloc(1, sizeof(struct bundle_instance));
	if ( (*userData) == NULL ){
		status = CELIX_ENOMEM;
	}
    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt)userData;

    bi->service = calloc(1, sizeof(*(bi->service)));
    bi->locator = calloc(1, sizeof(*(bi->locator)));
    if(bi->service != NULL && bi->locator != NULL){
	bi->service->findDrivers = driverLocator_findDrivers;
	bi->service->loadDriver = driverLocator_loadDriver;

	bi->service->locator = bi->locator;
	bi->locator->drivers = NULL;
	arrayList_create(&bi->locator->drivers);
	bundleContext_getProperty(context, "DRIVER_LOCATOR_PATH", (const char**)&bi->locator->path);
	if (bi->locator->path == NULL ) {
		bi->locator->path = (char *)DEFAULT_LOCATOR_PATH;
	}
	status = bundleContext_registerService(context, OSGI_DEVICEACCESS_DRIVER_LOCATOR_SERVICE_NAME, bi->service, NULL, &bi->locatorRegistration);
    }
    else{
	if(bi->service!=NULL) free(bi->service);
	if(bi->locator!=NULL) free(bi->locator);
	status = CELIX_ENOMEM;
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt)userData;
    serviceRegistration_unregister(bi->locatorRegistration);
    arrayList_destroy(bi->locator->drivers);
    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    return CELIX_SUCCESS;
}
