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

#include <celix_api.h>

#include "phase2.h"
#include "phase3_cmp.h"

struct phase3_activator_struct {
    phase3_cmp_t *phase3Cmp;
};

static celix_status_t activator_start(struct phase3_activator_struct *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;
    printf("phase3: start\n");
    act->phase3Cmp = phase3_create();
    if (act->phase3Cmp != NULL) {

        celix_dm_component_t *cmp = celix_dmComponent_create(ctx, "PHASE3_PROCESSING_COMPONENT");
        celix_dmComponent_setImplementation(cmp, act->phase3Cmp);
        CELIX_DM_COMPONENT_SET_CALLBACKS(cmp, phase3_cmp_t, phase3_init, phase3_start, phase3_stop, phase3_deinit);
        CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(cmp, phase3_cmp_t, phase3_destroy);

        celix_dm_service_dependency_t *dep = celix_dmServiceDependency_create();
        celix_dmServiceDependency_setService(dep, PHASE2_NAME, NULL, NULL);
        celix_dmServiceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND); //SUSPEND Strategy is default
        celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
        opts.add = (void*)phase3_addPhase2;
        opts.remove = (void*)phase3_removePhase2;
        celix_dmServiceDependency_setCallbacksWithOptions(dep, &opts);
        celix_dmServiceDependency_setRequired(dep, true);
        celix_dmComponent_addServiceDependency(cmp, dep);

        celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
        celix_dependencyManager_add(mng, cmp);
    } else {
        status = CELIX_ENOMEM;
    }
    return status;

}

static celix_status_t activator_stop(struct phase3_activator_struct *act __attribute__((unused)), celix_bundle_context_t *ctx) {
    printf("phase3: stop\n");
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct phase3_activator_struct, activator_start, activator_stop);
