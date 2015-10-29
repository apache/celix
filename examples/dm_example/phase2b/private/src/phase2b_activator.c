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
#include <phase2b_cmp.h>

#include "bundle_activator.h"
#include "dm_activator_base.h"

#include "phase1.h"
#include "phase2.h"
#include "phase2b_cmp.h"

struct phase2b_activator_struct {
    phase2b_cmp_t *phase2bCmp;
	phase2_t phase2Serv;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	printf("phase2b: dm_create\n");
	*userData = calloc(1, sizeof(struct phase2b_activator_struct));
	return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase2b: dm_init\n");
    celix_status_t status = CELIX_SUCCESS;

    struct phase2b_activator_struct *act = (struct phase2b_activator_struct *)userData;

	act->phase2bCmp = phase2b_create();
	if (act->phase2bCmp != NULL) {

		act->phase2Serv.handle = act->phase2bCmp;
		act->phase2Serv.getData = (void *)phase2b_getData;

		properties_pt props = properties_create();
		properties_set(props, "id", "phase2b");

		dm_component_pt cmp;
		component_create(context, "PHASE2B_PROCESSING_COMPONENT", &cmp);
		component_setImplementation(cmp, act->phase2bCmp);
		component_setCallbacksSafe(cmp, phase2b_cmp_t *, phase2b_init, phase2b_start, phase2b_stop, phase2b_deinit);
		component_addInterface(cmp, PHASE2_NAME, &act->phase2Serv, props);


		dm_service_dependency_pt dep;
		serviceDependency_create(&dep);
		serviceDependency_setService(dep, PHASE1_NAME, NULL);
		serviceDependency_setCallbacksSafe(dep, phase2b_cmp_t *, phase1_t *, phase2b_setPhase1, NULL, NULL, NULL, NULL);
		serviceDependency_setRequired(dep, true);
		component_addServiceDependency(cmp, dep);

		dependencyManager_add(manager, cmp);
	} else {
		status = CELIX_ENOMEM;
	}

    return status;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("phase2b: dm-destroy\n");

	struct phase2b_activator_struct *act = (struct phase2b_activator_struct *)userData;
	if (act->phase2bCmp != NULL) {
		phase2b_destroy(act->phase2bCmp);
	}
	free(act);

	return CELIX_SUCCESS;
}
