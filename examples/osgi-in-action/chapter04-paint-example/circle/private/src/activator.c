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

#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "service_registration.h"
#include "bundle_activator.h"
#include "bundle_context.h"
#include "simple_shape.h"
#include "circle_shape.h"
#include "simple_shape.h"

struct activator {
	SERVICE_REGISTRATION reg;
	apr_pool_t *pool;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	struct activator *activator;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct activator));
		activator = *userData;
		activator->reg = NULL;
		activator->pool = pool;
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT ctx) {
	struct activator * act = (struct activator *) userData;
	celix_status_t status = CELIX_SUCCESS;
	SIMPLE_SHAPE es = NULL;
	circleShape_create(ctx, &es);
	PROPERTIES props = properties_create();
	properties_set(props, "name", "circle");
    status = bundleContext_registerService(ctx, SIMPLE_SHAPE_SERVICE_NAME, es, props, &act->reg);
	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * act = (struct activator *) userData;

	status = serviceRegistration_unregister(act->reg);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
