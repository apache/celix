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
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "simple_shape.h"
#include "triangle_shape.h"
#include "simple_shape.h"

struct greetingActivator {
	service_registration_pt reg;
	apr_pool_t *pool;
};

typedef struct greetingActivator *greeting_activator_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	apr_pool_t *pool;
	greeting_activator_pt activator;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct greetingActivator));
		activator = *userData;
		activator->reg = NULL;
		activator->pool = pool;
	}
	printf("Triangle created %d\n", status);
	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt ctx) {
	struct greetingActivator * act = (struct greetingActivator *) userData;
	celix_status_t status = CELIX_SUCCESS;
	simple_shape_pt es = triangleShape_create(ctx);
	properties_pt props = properties_create();
	properties_set(props, "name", "triangle");
    status = bundleContext_registerService(ctx, SIMPLE_SHAPE_SERVICE_NAME, es, props, &act->reg);
	printf("Triangle activated %d\n", status);
	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}
