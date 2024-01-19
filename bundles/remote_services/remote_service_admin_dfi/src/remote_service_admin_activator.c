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

#include "celix_bundle_activator.h"

#include "remote_service_admin_dfi.h"
#include "export_registration_dfi.h"
#include "import_registration_dfi.h"
#include "remote_service_admin_dfi_constants.h"
#include "remote_constants.h"

typedef struct celix_remote_service_admin_activator {
    remote_service_admin_t *admin;
    remote_service_admin_service_t adminService;
    long svcIdRsa;
} celix_remote_service_admin_activator_t;

static celix_status_t celix_rsa_start(celix_remote_service_admin_activator_t* activator, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;
    activator->svcIdRsa = -1;
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (props == NULL) {
        return CELIX_ENOMEM;
    }
    status = celix_properties_set(props, CELIX_RSA_REMOTE_CONFIGS_SUPPORTED, RSA_DFI_CONFIGURATION_TYPE","CELIX_RSA_DFI_ZEROCONF_CONFIGURATION_TYPE);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = remoteServiceAdmin_create(ctx, &activator->admin);
    if (status == CELIX_SUCCESS) {
        activator->adminService.admin = activator->admin;
        activator->adminService.exportService = remoteServiceAdmin_exportService;

        activator->adminService.getExportedServices = remoteServiceAdmin_getExportedServices;
        activator->adminService.getImportedEndpoints = remoteServiceAdmin_getImportedEndpoints;
        activator->adminService.importService = remoteServiceAdmin_importService;

        activator->adminService.exportReference_getExportedEndpoint = exportReference_getExportedEndpoint;
        activator->adminService.exportReference_getExportedService = exportReference_getExportedService;

        activator->adminService.exportRegistration_close = remoteServiceAdmin_removeExportedService;
        activator->adminService.exportRegistration_getException = exportRegistration_getException;
        activator->adminService.exportRegistration_getExportReference = exportRegistration_getExportReference;

        activator->adminService.importReference_getImportedEndpoint = importReference_getImportedEndpoint;
        activator->adminService.importReference_getImportedService = importReference_getImportedService;

        activator->adminService.importRegistration_close = remoteServiceAdmin_removeImportedService;
        activator->adminService.importRegistration_getException = importRegistration_getException;
        activator->adminService.importRegistration_getImportReference = importRegistration_getImportReference;

        activator->svcIdRsa = celix_bundleContext_registerService(ctx, &activator->adminService, CELIX_RSA_REMOTE_SERVICE_ADMIN,
                                                                  celix_steal_ptr(props));
    }

    return status;
}

static celix_status_t celix_rsa_stop(celix_remote_service_admin_activator_t* activator, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, activator->svcIdRsa);
    remoteServiceAdmin_stop(activator->admin);
    remoteServiceAdmin_destroy(&activator->admin);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_remote_service_admin_activator_t, celix_rsa_start, celix_rsa_stop)


