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

#include "celix_api.h"
#include "foo2.h"

#include <stdlib.h>

struct activator {
	foo2_t *foo;
};

static celix_status_t activator_start(struct activator *act, celix_bundle_context_t *ctx) {
	celix_status_t status = CELIX_SUCCESS;
	act->foo = foo2_create();
	if (act->foo == NULL) {
		status = CELIX_ENOMEM;
	} else {

		celix_dm_component_t *cmp = celix_dmComponent_create(ctx, "FOO2");
		celix_dmComponent_setImplementation(cmp, act->foo);

		/*
		With the component_setCallbacksSafe we register callbacks when a component is started / stopped using a component
		 with type foo1_t*
		*/
		CELIX_DMCOMPONENT_SETCALLBACKS(cmp, foo2_t*, NULL, foo2_start, foo2_stop, NULL);

		celix_dm_service_dependency_t *dep = celix_dmServiceDependency_create();
		celix_dmServiceDependency_setRequired(dep, false);
		celix_dmServiceDependency_setService(dep, EXAMPLE_NAME, EXAMPLE_CONSUMER_RANGE, NULL);
		celix_dmServiceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);

		/*
		With the serviceDependency_setCallbacksSafe we register callbacks when a service
		is added and about to be removed for the component type foo1_t* and service type example_t*.

		We should protect the usage of the
		service because after removal of the service the memory location of that service
		could be freed
		*/
		celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
		opts.add = (void*)foo2_addExample;
		opts.remove = (void*)foo2_removeExample;
		celix_dmServiceDependency_setCallbacksWithOptions(dep, &opts);
		celix_dmComponent_addServiceDependency(cmp, dep);

		celix_dependencyManager_add(celix_bundleContext_getDependencyManager(ctx), cmp);
	}

	return status;
}

static celix_status_t activator_stop(struct activator *act, celix_bundle_context_t *ctx) {
	celix_dependencyManager_removeAllComponents(celix_bundleContext_getDependencyManager(ctx));
	foo2_destroy(act->foo);
	return CELIX_SUCCESS;
}


CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, activator_start, activator_stop)