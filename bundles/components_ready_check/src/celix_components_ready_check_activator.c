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

#include "celix_components_ready_check.h"
#include "celix_bundle_activator.h"

typedef struct celix_components_ready_check_activator {
    celix_components_ready_check_t* rdy;
} celix_components_ready_check_activator_t;

static int celix_componentsReadyCheckActivator_start(celix_components_ready_check_activator_t* act, celix_bundle_context_t* ctx) {
    act->rdy = celix_componentsReadyCheck_create(ctx);
    return act->rdy? CELIX_SUCCESS : CELIX_ENOMEM;
}

static int celix_componentsCheckActivator_stop(celix_components_ready_check_activator_t* act, celix_bundle_context_t* ctx) {
    celix_componentsReadyCheck_destroy(act->rdy);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_components_ready_check_activator_t,
                           celix_componentsReadyCheckActivator_start,
                           celix_componentsCheckActivator_stop);
