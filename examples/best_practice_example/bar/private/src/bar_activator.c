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

struct activator {
	bar_t *bar;
	example_t exampleService;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *act = calloc(1, sizeof(*act));
	if (act != NULL) {

		act->bar = bar_create();
		act->exampleService.handle = act->bar;
		act->exampleService.method = (void*) bar_method;

		if (act->bar != NULL) {
            *userData = act;
        } else {
            free(act);
        }
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t dm_init(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	dm_component_pt cmp = NULL;
	component_create(context, "BAR", &cmp);
	component_setImplementation(cmp, activator->bar);
	component_addInterface(cmp, EXAMPLE_NAME, EXAMPLE_VERSION, &activator->exampleService, NULL);

	dependencyManager_add(manager, cmp);
    return status;
}

celix_status_t dm_destroy(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	bar_destroy(activator->bar);
	free(activator);
	return status;
};

