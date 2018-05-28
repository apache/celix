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

#include "dm_activator.h"
#include "bar.h"

#include <stdlib.h>

typedef struct activator {
	bar_t *bar;
	example_t exampleService;
} activator_t;

static celix_status_t activator_start(activator_t *act, celix_bundle_context_t *ctx) {
	celix_status_t status = CELIX_SUCCESS;
	act->bar = bar_create();
	act->exampleService.handle = act->bar;
	act->exampleService.method = (void*) bar_method;
	if (act->bar == NULL) {
		status = CELIX_ENOMEM;
	} else {
		dm_component_pt cmp = NULL;
		component_create(ctx, "BAR", &cmp);
		component_setImplementation(cmp, act->bar);
		component_addInterface(cmp, EXAMPLE_NAME, EXAMPLE_VERSION, &act->exampleService, NULL);
		dependencyManager_add(celix_bundleContext_getDependencyManager(ctx), cmp);
	}
	return status;
}

static celix_status_t activator_stop(activator_t *act, celix_bundle_context_t *ctx) {
	dependencyManager_removeAllComponents(celix_bundleContext_getDependencyManager(ctx));
	bar_destroy(act->bar);
	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_t, activator_start, activator_stop)