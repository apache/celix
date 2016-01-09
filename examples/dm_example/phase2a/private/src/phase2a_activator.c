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
#include <phase2a_cmp.h>

#include "bundle_activator.h"
#include "dm_activator.h"

#include "phase1.h"
#include "phase2.h"
#include "phase2a_cmp.h"

struct phase2a_activator_struct {
    phase2a_cmp_t *phase2aCmp;
	phase2_t phase2Serv;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	printf("phase2a: dm_create\n");
	*userData = calloc(1, sizeof(struct phase2a_activator_struct));
	return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase2a: dm_init\n");
    celix_status_t status = CELIX_SUCCESS;

    struct phase2a_activator_struct *act = (struct phase2a_activator_struct *)userData;

	act->phase2aCmp = phase2a_create();
	if (act->phase2aCmp != NULL) {

		act->phase2Serv.handle = act->phase2aCmp;
		act->phase2Serv.getData = (void *)phase2a_getData;

		properties_pt props = properties_create();
		properties_set(props, "id", "phase2a");

		dm_component_pt cmp;
		component_create(context, "PHASE2A_PROCESSING_COMPONENT", &cmp);
		component_setImplementation(cmp, act->phase2aCmp);
		component_setCallbacksSafe(cmp, phase2a_cmp_t *, phase2a_init, phase2a_start, phase2a_stop, phase2a_deinit);
		component_addInterface(cmp, PHASE2_NAME, PHASE2_VERSION, &act->phase2Serv, props);


		dm_service_dependency_pt dep;
		serviceDependency_create(&dep);
		serviceDependency_setService(dep, PHASE1_NAME, PHASE1_RANGE_ALL, NULL);

        serviceDependency_setCallbacksSafe(dep, phase2a_cmp_t *, phase1_t *, phase2a_setPhase1, NULL, NULL, NULL, NULL);
		serviceDependency_setRequired(dep, true);
		component_addServiceDependency(cmp, dep);

		dependencyManager_add(manager, cmp);
	} else {
		status = CELIX_ENOMEM;
	}

    return status;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase2a: dm-destroy\n");

	struct phase2a_activator_struct *act = (struct phase2a_activator_struct *)userData;
	if (act->phase2aCmp != NULL) {
		phase2a_destroy(act->phase2aCmp);
	}
	free(act);

	return CELIX_SUCCESS;
}
