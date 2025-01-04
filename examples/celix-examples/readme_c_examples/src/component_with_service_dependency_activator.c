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

//src/component_with_service_dependency_activator.c
#include <stdlib.h>
#include <celix_bundle_activator.h>
#include <celix_shell_command.h>

//********************* COMPONENT *******************************/

typedef struct component_with_service_dependency {
    celix_shell_command_t* highestRankingCmdShell; //only updated when component is not active or suspended
    celix_thread_mutex_t mutex; //protects cmdShells
    celix_array_list_t* cmdShells;
} component_with_service_dependency_t;

static component_with_service_dependency_t* componentWithServiceDependency_create() {
    component_with_service_dependency_t* cmp = calloc(1, sizeof(*cmp));
    celixThreadMutex_create(&cmp->mutex, NULL); // <-----------------------------------------------------------------<1>
    cmp->cmdShells = celix_arrayList_createPointerArray();
    return cmp;
}

static void componentWithServiceDependency_destroy(component_with_service_dependency_t* cmp) {
    celix_arrayList_destroy(cmp->cmdShells);
    celixThreadMutex_destroy(&cmp->mutex);
    free(cmp);
}

static void componentWithServiceDependency_setHighestRankingShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd) {
    printf("New highest ranking service (can be NULL): %p\n", shellCmd);
    cmp->highestRankingCmdShell = shellCmd; // <---------------------------------------------------------------------<2>
}

static void componentWithServiceDependency_addShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd,
        const celix_properties_t* props) {
    long id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    printf("Adding shell command service with service.id %li\n", id);
    celixThreadMutex_lock(&cmp->mutex); // <-------------------------------------------------------------------------<3>
    celix_arrayList_add(cmp->cmdShells, shellCmd);
    celixThreadMutex_unlock(&cmp->mutex);
}

static void componentWithServiceDependency_removeShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd,
        const celix_properties_t* props) {
    long id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    printf("Removing shell command service with service.id %li\n", id);
    celixThreadMutex_lock(&cmp->mutex);
    celix_arrayList_remove(cmp->cmdShells, shellCmd);
    celixThreadMutex_unlock(&cmp->mutex);
}

//********************* ACTIVATOR *******************************/

typedef struct component_with_service_dependency_activator {
    //nop
} component_with_service_dependency_activator_t;

static celix_status_t componentWithServiceDependencyActivator_start(component_with_service_dependency_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    component_with_service_dependency_t* impl = componentWithServiceDependency_create();

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "component_with_service_dependency_1");
    celix_dmComponent_setImplementation(dmCmp, impl);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            component_with_service_dependency_t,
            componentWithServiceDependency_destroy);

    //create mandatory service dependency with cardinality one and with a suspend-strategy
    celix_dm_service_dependency_t* dep1 = celix_dmServiceDependency_create(); // <-----------------------------------<4>
    celix_dmServiceDependency_setService(dep1, CELIX_SHELL_COMMAND_SERVICE_NAME, NULL, NULL); // <-------------------<5>
    celix_dmServiceDependency_setStrategy(dep1, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND); // <------------------------<6>
    celix_dmServiceDependency_setRequired(dep1, true); // <----------------------------------------------------------<7>
    celix_dm_service_dependency_callback_options_t opts1 = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS; // <--<8>
    opts1.set = (void*)componentWithServiceDependency_setHighestRankingShellCommand; // <----------------------------<9>
    celix_dmServiceDependency_setCallbacksWithOptions(dep1, &opts1); // <-------------------------------------------<10>
    celix_dmComponent_addServiceDependency(dmCmp, dep1); // <-------------------------------------------------------<11>

    //create optional service dependency with cardinality many and with a locking-strategy
    celix_dm_service_dependency_t* dep2 = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep2, CELIX_SHELL_COMMAND_SERVICE_NAME, NULL, NULL);
    celix_dmServiceDependency_setStrategy(dep2, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);  // <----------------------<12>
    celix_dmServiceDependency_setRequired(dep2, false); // <--------------------------------------------------------<13>
    celix_dm_service_dependency_callback_options_t opts2 = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts2.addWithProps = (void*)componentWithServiceDependency_addShellCommand;  // <-------------------------------<14>
    opts2.removeWithProps = (void*)componentWithServiceDependency_removeShellCommand;
    celix_dmServiceDependency_setCallbacksWithOptions(dep2, &opts2);
    celix_dmComponent_addServiceDependency(dmCmp, dep2);

    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(
        component_with_service_dependency_activator_t,
        componentWithServiceDependencyActivator_start,
        NULL)
