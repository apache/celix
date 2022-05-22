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
#include <phase1_cmp.h>

#include <celix_api.h>

#include "phase1.h"

struct phase1_activator_struct {
    phase1_cmp_t *phase1Cmp;
    phase1_t phase1Serv;
};

static celix_status_t activator_start(struct phase1_activator_struct *act, celix_bundle_context_t *ctx) {
    celix_status_t status = CELIX_SUCCESS;
    printf("PHASE1: start\n");
    act->phase1Cmp = phase1_create();
    if (act->phase1Cmp != NULL) {

        act->phase1Serv.handle = act->phase1Cmp;
        act->phase1Serv.getData = (void *)phase1_getData;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, "id", "phase1");

        celix_dm_component_t *cmp= celix_dmComponent_create(ctx, "PHASE1_PROCESSING_COMPONENT");
        celix_dmComponent_setImplementation(cmp, act->phase1Cmp);
        CELIX_DM_COMPONENT_SET_CALLBACKS(cmp, phase1_cmp_t, phase1_init, phase1_start, phase1_stop, phase1_deinit);
        CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(cmp, phase1_cmp_t, phase1_destroy);

        phase1_setComp(act->phase1Cmp, cmp);
        celix_dmComponent_addInterface(cmp, PHASE1_NAME, PHASE1_VERSION, &act->phase1Serv, props);

        celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(ctx);
        celix_dependencyManager_add(mng, cmp);
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

static celix_status_t activator_stop(struct phase1_activator_struct *act, celix_bundle_context_t *ctx) {
    printf("PHASE1: stop\n");
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct phase1_activator_struct, activator_start, activator_stop);