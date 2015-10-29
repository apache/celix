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
 *  \date       Oct 29, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "dm_activator_base.h"

#include "phase2.h"
#include "phase3_cmp.h"

struct phase3_activator_struct {
    phase3_cmp_t *phase3Cmp;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	printf("phase3: dm_create\n");
	*userData = calloc(1, sizeof(struct phase3_activator_struct));
	return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase3: dm_init\n");
    celix_status_t status = CELIX_SUCCESS;

    struct phase3_activator_struct *act = (struct phase3_activator_struct *)userData;

	act->phase3Cmp = phase3_create();
	if (act->phase3Cmp != NULL) {

		properties_pt props = properties_create();
		properties_set(props, "id", "phase3");

		dm_component_pt cmp;
		component_create(context, "PHASE3_PROCESSING_COMPONENT", &cmp);
		component_setImplementation(cmp, act->phase3Cmp);
		component_setCallbacks(cmp, (void *)phase3_init, (void *)phase3_start, (void *)phase3_stop, (void *)phase3_destroy);

		dm_service_dependency_pt dep;
		serviceDependency_create(&dep);
		serviceDependency_setService(dep, PHASE2_NAME, NULL);
        serviceDependency_setCallbacks(dep, NULL, (void *)phase3_addPhase2, NULL, (void *)phase3_removePhase2, NULL);
		serviceDependency_setRequired(dep, true);
		component_addServiceDependency(cmp, dep);

		dependencyManager_add(manager, cmp);
	} else {
		status = CELIX_ENOMEM;
	}

    return status;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase3: dm-destroy\n");

	struct phase3_activator_struct *act = (struct phase3_activator_struct *)userData;
	if (act->phase3Cmp != NULL) {
		phase3_destroy(act->phase3Cmp);
	}
	free(act);

	return CELIX_SUCCESS;
}
