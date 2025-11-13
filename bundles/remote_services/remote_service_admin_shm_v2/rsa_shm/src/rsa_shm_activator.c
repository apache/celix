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
#include "rsa_shm_export_registration.h"
#include "rsa_shm_import_registration.h"
#include "rsa_shm_impl.h"
#include "rsa_shm_constants.h"
#include "remote_constants.h"
#include "remote_service_admin.h"
#include "celix_log_helper.h"
#include "celix_api.h"
#include "celix_stdlib_cleanup.h"
#include <assert.h>
#include <string.h>

typedef struct rsa_shm_activator {
    remote_service_admin_service_t adminService;
}rsa_shm_activator_t;

celix_status_t celix_rsaShmActivator_addRpcFactorySvcDependency(celix_dm_component_t *cmp, const char* rpcType) {
    celix_autoptr(celix_dm_service_dependency_t) rpcFactoryDep = celix_dmServiceDependency_create();
    if (rpcFactoryDep == NULL) {
        return ENOMEM;
    }
    celix_autofree char *filter = NULL;
    int ret = asprintf(&filter, "("CELIX_RSA_RPC_TYPE_KEY"=%s)", rpcType);
    if (ret < 0) {
        return ENOMEM;
    }
    celix_status_t status = celix_dmServiceDependency_setService(rpcFactoryDep, CELIX_RSA_RPC_FACTORY_NAME, CELIX_RSA_RPC_FACTORY_USE_RANGE, filter);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_dmServiceDependency_setRequired(rpcFactoryDep, true);
    celix_dmServiceDependency_setStrategy(rpcFactoryDep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
    celix_dm_service_dependency_callback_options_t opts = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts.addWithProps = celix_rsaShm_addRpcFactorySvc;
    opts.removeWithProps = celix_rsaShm_removeRpcFactorySvc;
    (void)celix_dmServiceDependency_setCallbacksWithOptions(rpcFactoryDep, &opts);
    status = celix_dmComponent_addServiceDependency(cmp, rpcFactoryDep);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_steal_ptr(rpcFactoryDep);
    return CELIX_SUCCESS;
}

celix_status_t rsaShmActivator_start(rsa_shm_activator_t *activator, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    assert(activator != NULL);
    assert(context != NULL);

    celix_autoptr(celix_dm_component_t) rsaCmp = celix_dmComponent_create(context, "rsa_shm");
    if (rsaCmp == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    rsa_shm_t* admin = NULL;
    status = rsaShm_create(context, &admin);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_dmComponent_setImplementation(rsaCmp, admin);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(rsaCmp, rsa_shm_t, rsaShm_destroy);
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (props == NULL) {
        return CELIX_ENOMEM;
    }
    status = celix_properties_set(props, CELIX_RSA_REMOTE_CONFIGS_SUPPORTED, RSA_SHM_CONFIGURATION_TYPE);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    activator->adminService.admin = (void*)admin;
    activator->adminService.exportService = (void*)rsaShm_exportService;

    activator->adminService.getExportedServices = (void*)rsaShm_getExportedServices;
    activator->adminService.getImportedEndpoints = (void*)rsaShm_getImportedEndpoints;
    activator->adminService.importService = (void*)rsaShm_importService;

    activator->adminService.exportReference_getExportedEndpoint = exportReference_getExportedEndpoint;
    activator->adminService.exportReference_getExportedService = exportReference_getExportedService;

    activator->adminService.exportRegistration_close = (void*)rsaShm_removeExportedService;
    activator->adminService.exportRegistration_getException = exportRegistration_getException;
    activator->adminService.exportRegistration_getExportReference = exportRegistration_getExportReference;

    activator->adminService.importReference_getImportedEndpoint = importReference_getImportedEndpoint;
    activator->adminService.importReference_getImportedService = importReference_getImportedService;

    activator->adminService.importRegistration_close = (void*)rsaShm_removeImportedService;
    activator->adminService.importRegistration_getException = importRegistration_getException;
    activator->adminService.importRegistration_getImportReference = importRegistration_getImportReference;
    status = celix_dmComponent_addInterface(rsaCmp, CELIX_RSA_REMOTE_SERVICE_ADMIN, NULL,
                                                           &activator->adminService, celix_steal_ptr(props));
    if (status != CELIX_SUCCESS) {
        return status;
    }

    const char* rpcTypes = celix_bundleContext_getProperty(context, CELIX_RSA_SHM_RPC_TYPES, NULL);
    if (rpcTypes == NULL) {
        status = celix_rsaShmActivator_addRpcFactorySvcDependency(rsaCmp, RSA_SHM_RPC_TYPE_DEFAULT);
        if (status != CELIX_SUCCESS) {
            return status;
        }
    } else {
        const char delimiter[2] = ",";
        char *token, *savePtr;
        celix_autofree char *rpcTypesCopy = celix_utils_strdup(rpcTypes);
        if (rpcTypesCopy == NULL) {
            return ENOMEM;
        }
        token = strtok_r(rpcTypesCopy, delimiter, &savePtr);
        if (token == NULL) {
            return CELIX_ILLEGAL_ARGUMENT;
        }
        while (token != NULL) {
            const char *rpcType = celix_utils_trimInPlace(token);
            status = celix_rsaShmActivator_addRpcFactorySvcDependency(rsaCmp, rpcType);
            if (status != CELIX_SUCCESS) {
                return status;
            }
            token = strtok_r(NULL, delimiter, &savePtr);
        }
    }

    celix_dependency_manager_t* mg = celix_bundleContext_getDependencyManager(context);
    if (mg == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    status = celix_dependencyManager_addAsync(mg, rsaCmp);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_steal_ptr(rsaCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(rsa_shm_activator_t, rsaShmActivator_start, NULL)
