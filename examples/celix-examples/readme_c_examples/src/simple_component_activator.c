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

//src/simple_component_activator.c
#include <stdio.h>
#include <celix_api.h>

//********************* COMPONENT *******************************/

typedef struct simple_component {
    int transitionCount; //not protected, only updated and read in the celix event thread.
} simple_component_t;

static simple_component_t* simpleComponent_create() {
    simple_component_t* cmp = calloc(1, sizeof(*cmp));
    cmp->transitionCount = 1;
    return cmp;
}

static void simpleComponent_destroy(simple_component_t* cmp) {
    free(cmp);
}

static int simpleComponent_init(simple_component_t* cmp) { // <------------------------------------------------------<1>
    printf("Initializing simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_start(simple_component_t* cmp) {
    printf("Starting simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_stop(simple_component_t* cmp) {
    printf("Stopping simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_deinit(simple_component_t* cmp) {
    printf("De-initializing simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}


//********************* ACTIVATOR *******************************/

typedef struct simple_component_activator {
    //nop
} simple_component_activator_t;

static celix_status_t simpleComponentActivator_start(simple_component_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    simple_component_t* impl = simpleComponent_create(); // <--------------------------------------------------------<2>

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "simple_component_1"); // <--------------------------<3>
    celix_dmComponent_setImplementation(dmCmp, impl); // <-----------------------------------------------------------<4>
    CELIX_DM_COMPONENT_SET_CALLBACKS(
            dmCmp,
            simple_component_t,
            simpleComponent_init,
            simpleComponent_start,
            simpleComponent_stop,
            simpleComponent_deinit); // <----------------------------------------------------------------------------<5>
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            simple_component_t,
            simpleComponent_destroy); // <---------------------------------------------------------------------------<6>

    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp); // <---------------------------------------------------------------------<7>
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(simple_component_activator_t, simpleComponentActivator_start, NULL) // <------------------<8>