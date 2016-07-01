/**
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
#include "celix/dm/DmActivator.h"
#include "celix/dm/DependencyManager.h"

extern "C" {
#include <stdlib.h>
#include "bundle_context.h"
#include "celix_errno.h"
#include "dm_dependency_manager.h"
#include "bundle_activator.h"
#include "dm_info.h"
#include "dm_activator.h"


struct dm_dependency_activator_base {
    dm_dependency_activator_base(std::shared_ptr<celix::dm::DependencyManager> man, std::shared_ptr<celix::dm::DmActivator> act) : manager(man), activator(act) {}
    bundle_context_pt context {nullptr};
    service_registration_pt reg {nullptr};
    dm_info_service_t info;

    std::shared_ptr<celix::dm::DependencyManager> manager;
    std::shared_ptr<celix::dm::DmActivator> activator;
};

typedef struct dm_dependency_activator_base *dependency_activator_base_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;

    auto man = std::shared_ptr<celix::dm::DependencyManager>{new celix::dm::DependencyManager(context)};
    auto dmAct = std::shared_ptr<celix::dm::DmActivator> {DmActivator::create(*man)};
    dependency_activator_base_pt act = new dm_dependency_activator_base(man, dmAct);

    if (act != nullptr) {
        act->context = context;
        act->reg = nullptr;
        act->info.handle = nullptr;
        act->info.destroyInfo = nullptr;
        act->info.getInfo = nullptr;
    } else {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        *userData = act;
    } else {
        delete act;
    }

    return status;
}

celix_status_t bundleActivator_start(void *userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    dependency_activator_base_pt act = (dependency_activator_base_pt) userData;

    act->activator->init(*act->manager);
    act->manager->start();

    if (status == CELIX_SUCCESS) {
        //Create the service
        act->info.handle = act->manager->cDependencyManager();
        act->info.getInfo = (celix_status_t (*)(void *, dm_dependency_manager_info_pt *)) dependencyManager_getInfo;
        act->info.destroyInfo = (void (*)(void *, dm_dependency_manager_info_pt)) dependencyManager_destroyInfo;

        status = bundleContext_registerService(context, (char *) DM_INFO_SERVICE_NAME, &act->info, NULL, &(act->reg));
    }

    return status;
}

celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
    dependency_activator_base_pt act = (dependency_activator_base_pt) userData;

    // Remove the service
    status = serviceRegistration_unregister(act->reg);
    dependencyManager_removeAllComponents(act->manager->cDependencyManager());

    return status;
}

celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
    dependency_activator_base_pt act = (dependency_activator_base_pt) userData;

    if (act == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    act->activator->deinit(*act->manager);
    act->manager->stop();

    delete act;
    return status;
}

}