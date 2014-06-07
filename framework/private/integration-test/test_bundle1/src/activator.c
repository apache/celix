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
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "framework_listener.h"

struct userData {
    framework_listener_pt listener;
};

celix_status_t test_frameworkEvent(void *listener, framework_event_pt event);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	apr_pool_t *pool;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct userData));
		((struct userData *)(*userData))->listener = NULL;
	} else {
		status = CELIX_START_ERROR;
	}
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	struct userData * activator = (struct userData *) userData;
	apr_pool_t *pool;

	bundleContext_getMemoryPool(context, &pool);

	activator->listener = apr_palloc(pool, sizeof(*activator->listener));
    activator->listener->handle = activator;
    activator->listener->frameworkEvent = test_frameworkEvent;
    bundleContext_addFrameworkListener(context, activator->listener);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	struct userData * data = (struct userData *) userData;

	// do not remove listener, fw should remove it.

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}

celix_status_t test_frameworkEvent(void *listener, framework_event_pt event) {
    return CELIX_SUCCESS;
}
