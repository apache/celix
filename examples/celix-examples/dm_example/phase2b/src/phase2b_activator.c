/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdlib.h>
#include <phase2b_cmp.h>

#include <celix_api.h>

#include "phase1.h"
#include "phase2.h"
#include "phase2b_cmp.h"

struct phase2b_activator_struct {
    phase2b_cmp_t *phase2bCmp;
    phase2_t phase2Serv;
};

static celix_status_t activator_start(struct phase2b_activator_struct *act, celix_bundle_context_t *ctx) {
    printf("phase2b: start\n");
    celix_status_t status = CELIX_SUCCESS;
    act->phase2bCmp = phase2b_create();
    if (act->phase2bCmp != NULL) {

        act->phase2Serv.handle = act->phase2bCmp;
        act->phase2Serv.getData = (void *)phase2b_getData;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "id", "phase2b");

        celix_dm_component_t *cmp = celix_dmComponent_create(ctx, "PHASE2B_PROCESSING_COMPONENT");
        celix_dmComponent_setImplementation(cmp, act->phase2bCmp);
        CELIX_DM_COMPONENT_SET_CALLBACKS(cmp, phase2b_cmp_t, phase2b_init, phase2b_start, phase2b_stop, phase2b_deinit);\
        //note not configuring destroy callback -> destroying component manually in the activator stop callback.

        celix_dmComponent_addInterface(cmp, PHASE2_NAME, PHASE2_VERSION, &act->phase2Serv, props);


        celix_dm_service_dependency_t *dep = celix_dmServiceDependency_create();
        celix_dmServiceDependency_setService(dep, PHASE1_NAME, PHASE1_RANGE_EXACT, NULL);
        celix_dmServiceDependency_setCallback(dep, (void*)phase2b_setPhase1);
        celix_dmServiceDependency_setRequired(dep, true);
        celix_dmServiceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
        celix_dmComponent_addServiceDependency(cmp, dep);

        celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
        celix_dependencyManager_add(mng, cmp);
    } else {
        status = CELIX_ENOMEM;
    }

    return status;
}

static celix_status_t activator_stop(struct phase2b_activator_struct *act, celix_bundle_context_t *ctx) {
    printf("phase2b: stop\n");
    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_removeAllComponents(mng);
    if (act->phase2bCmp != NULL) {
        phase2b_destroy(act->phase2bCmp); //destroy component implementation
    }
    return CELIX_SUCCESS;
}


CELIX_GEN_BUNDLE_ACTIVATOR(struct phase2b_activator_struct, activator_start, activator_stop)
