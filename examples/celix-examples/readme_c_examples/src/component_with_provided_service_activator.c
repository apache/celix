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

//src/component_with_provided_service_activator.c
#include <stdlib.h>
#include <celix_api.h>
#include <celix_shell_command.h>

//********************* COMPONENT *******************************/

typedef struct component_with_provided_service {
    int callCount; //atomic
} component_with_provided_service_t;

static component_with_provided_service_t* componentWithProvidedService_create() {
    component_with_provided_service_t* cmp = calloc(1, sizeof(*cmp));
    return cmp;
}

static void componentWithProvidedService_destroy(component_with_provided_service_t* cmp) {
    free(cmp);
}

static bool componentWithProvidedService_executeCommand(
        component_with_provided_service_t *cmp,
        const char *commandLine,
        FILE *outStream,
        FILE *errorStream __attribute__((unused))) {
    int count = __atomic_add_fetch(&cmp->callCount, 1, __ATOMIC_SEQ_CST);
    fprintf(outStream, "Hello from cmp. command called %i times. commandLine: %s\n", count, commandLine);
    return true;
}

//********************* ACTIVATOR *******************************/

typedef struct component_with_provided_service_activator {
    celix_shell_command_t shellCmd; // <-----------------------------------------------------------------------------<1>
} component_with_provided_service_activator_t;

static celix_status_t componentWithProvidedServiceActivator_start(component_with_provided_service_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    component_with_provided_service_t* impl = componentWithProvidedService_create();

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "component_with_provided_service_1");
    celix_dmComponent_setImplementation(dmCmp, impl);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            component_with_provided_service_t,
            componentWithProvidedService_destroy);

    //configure provided service
    act->shellCmd.handle = impl;
    act->shellCmd.executeCommand = (void*)componentWithProvidedService_executeCommand;
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "hello_component");
    celix_dmComponent_addInterface(
            dmCmp,
            CELIX_SHELL_COMMAND_SERVICE_NAME,
            CELIX_SHELL_COMMAND_SERVICE_VERSION,
            &act->shellCmd,
            props); // <---------------------------------------------------------------------------------------------<2>


    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(
        component_with_provided_service_activator_t,
        componentWithProvidedServiceActivator_start,
        NULL)
