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
#include <phase1_cmp.h>

#include "bundle_activator.h"
#include "dm_activator.h"

#include "phase1.h"

struct phase1_activator_struct {
    phase1_cmp_t *phase1Cmp;
	phase1_t phase1Serv;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	printf("PHASE1: dm_create\n");
	*userData = calloc(1, sizeof(struct phase1_activator_struct));
	return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("PHASE1: dm_init\n");
    celix_status_t status = CELIX_SUCCESS;


    struct phase1_activator_struct *act = (struct phase1_activator_struct *)userData;

	act->phase1Cmp = phase1_create();
	if (act->phase1Cmp != NULL) {

		act->phase1Serv.handle = act->phase1Cmp;
		act->phase1Serv.getData = (void *)phase1_getData;

		properties_pt props = properties_create();
		properties_set(props, "id", "phase1");

		dm_component_pt cmp;
		component_create(context, "PHASE1_PROCESSING_COMPONENT", &cmp);
		component_setImplementation(cmp, act->phase1Cmp);
		component_setCallbacksSafe(cmp, phase1_cmp_t *, phase1_init, phase1_start, phase1_stop, phase1_deinit);
		component_addInterface(cmp, PHASE1_NAME, PHASE1_VERSION, &act->phase1Serv, props);

		dependencyManager_add(manager, cmp);
	} else {
		status = CELIX_ENOMEM;
	}

    return status;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("PHASE1: dm-destroy\n");

	struct phase1_activator_struct *act = (struct phase1_activator_struct *)userData;
	if (act->phase1Cmp != NULL) {
		phase1_destroy(act->phase1Cmp);
	}
	free(act);

	return CELIX_SUCCESS;
}
