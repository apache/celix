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
#include <assert.h>

typedef struct rsa_shm_activator {
    rsa_shm_t *admin;
    remote_service_admin_service_t adminService;
    long adminSvcId;
    celix_log_helper_t *logHelper;
}rsa_shm_activator_t;


celix_status_t rsaShmActivator_start(rsa_shm_activator_t *activator, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    assert(activator != NULL);
    assert(context != NULL);

    celix_autoptr(celix_log_helper_t) logger = celix_logHelper_create(context, "celix_rsa_shm");
    if (logger == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    celix_autoptr(rsa_shm_t) admin = NULL;
    status = rsaShm_create(context, logger, &admin);
    if (status != CELIX_SUCCESS) {
        return status;
    }
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

    activator->adminSvcId = celix_bundleContext_registerServiceAsync(context, &activator->adminService,
                                                                     CELIX_RSA_REMOTE_SERVICE_ADMIN, celix_steal_ptr(props));
    if (activator->adminSvcId < 0) {
        return CELIX_BUNDLE_EXCEPTION;
    }
    activator->logHelper = celix_steal_ptr(logger);
    activator->admin = celix_steal_ptr(admin);
    return CELIX_SUCCESS;
}

celix_status_t rsaShmActivator_stop(rsa_shm_activator_t *activator, celix_bundle_context_t *context) {
    assert(activator != NULL);
    assert(context != NULL);

    celix_bundleContext_unregisterServiceAsync(context, activator->adminSvcId, NULL, NULL);
    celix_bundleContext_waitForEvents(context);//Ensure that no events use admin
    rsaShm_destroy(activator->admin);
    celix_bundleContext_waitForEvents(context);//Ensure that no events use logHelper
    celix_logHelper_destroy(activator->logHelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(rsa_shm_activator_t, rsaShmActivator_start, rsaShmActivator_stop)
