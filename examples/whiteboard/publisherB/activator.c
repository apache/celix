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
 * activator.c
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "publisher_private.h"

struct activatorData {
	PUBLISHER_SERVICE ps;
	PUBLISHER pub;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
    apr_pool_t *pool;
    celix_status_t status = bundleContext_getMemoryPool(context, &pool);
    if (status == CELIX_SUCCESS) {
        *userData = apr_palloc(pool, sizeof(struct activatorData));
    } else {
        status = CELIX_START_ERROR;
    }
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *pool;
    status = bundleContext_getMemoryPool(context, &pool);
    if (status == CELIX_SUCCESS) {
        struct activatorData * data = (struct activatorData *) userData;
        data->ps = apr_pcalloc(pool, sizeof(*(data->ps)));
        data->pub = apr_pcalloc(pool, sizeof(*(data->pub)));
        data->ps->invoke = publisher_invoke;
        data->ps->publisher = data->pub;

        PROPERTIES props = properties_create();
		properties_set(props, "id", "B");

    	SERVICE_REGISTRATION service_registration = NULL;
    	bundleContext_registerService(context, PUBLISHER_NAME, data->ps, props, &service_registration);
    } else {
        status = CELIX_START_ERROR;
    }
    return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
    return CELIX_SUCCESS;
}
