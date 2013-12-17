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
 * activator.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_strings.h>

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
    apr_pool_t *pool;
    status = bundleContext_getMemoryPool(context, &pool);
    if (status == CELIX_SUCCESS) {
    	bundle_instance_pt bi = apr_palloc(pool, sizeof(struct bundle_instance));
        if (userData != NULL) {
        	bi->service=NULL;
        	bi->locator=NULL;
        	bi->locatorRegistration=NULL;
        	(*userData)=bi;
        } else {
        	status = CELIX_ENOMEM;
        }
    }
    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *pool;
    status = bundleContext_getMemoryPool(context, &pool);
    bundle_instance_pt bi = (bundle_instance_pt)userData;
    if (status == CELIX_SUCCESS) {
        struct activatorData * data = (struct activatorData *) userData;
        bi->service = apr_palloc(pool, sizeof(*(bi->service)));
        bi->service->findDrivers = driverLocator_findDrivers;
        bi->service->loadDriver = driverLocator_loadDriver;

        bi->locator = apr_pcalloc(pool, sizeof(*(bi->locator)));
        bi->service->locator = bi->locator;
        bi->locator->pool = pool;
        bi->locator->drivers = NULL;
        arrayList_create(&bi->locator->drivers);
        bundleContext_getProperty(context, "DRIVER_LOCATOR_PATH", &bi->locator->path);
        if (bi->locator->path == NULL ) {
        	bi->locator->path = (char *)DEFAULT_LOCATOR_PATH;
        }
        bundleContext_registerService(context, OSGI_DEVICEACCESS_DRIVER_LOCATOR_SERVICE_NAME, bi->service, NULL, &bi->locatorRegistration);
    } else {
        status = CELIX_START_ERROR;
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
