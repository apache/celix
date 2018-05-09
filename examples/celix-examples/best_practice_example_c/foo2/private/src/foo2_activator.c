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
#include "foo2.h"

#include <stdlib.h>

struct activator {
	foo2_t *foo;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *act = calloc(1, sizeof(*act));
	if (act != NULL) {
		act->foo = foo2_create();
        if (act->foo != NULL) {
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
	component_create(context, "FOO2", &cmp);
	component_setImplementation(cmp, activator->foo);

	/*
	With the component_setCallbacksSafe we register callbacks when a component is started / stopped using a component
	 with type foo1_t*
	*/
    component_setCallbacksSafe(cmp, foo2_t*, NULL, foo2_start, foo2_stop, NULL);

	dm_service_dependency_pt dep = NULL;
	serviceDependency_create(&dep);
	serviceDependency_setRequired(dep, false);
	serviceDependency_setService(dep, EXAMPLE_NAME, EXAMPLE_CONSUMER_RANGE, NULL);
	serviceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);

	/*
	With the serviceDependency_setCallbacksSafe we register callbacks when a service
	is added and about to be removed for the component type foo1_t* and service type example_t*.

	We should protect the usage of the
 	service because after removal of the service the memory location of that service
	could be freed
	*/
    serviceDependency_setCallbacksSafe(dep, foo2_t*, const example_t*, NULL, foo2_addExample, NULL, foo2_removeExample, NULL);
	component_addServiceDependency(cmp, dep);

	dependencyManager_add(manager, cmp);

    return status;
}

celix_status_t dm_destroy(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	foo2_destroy(activator->foo);
	free(activator);
	return status;
};

