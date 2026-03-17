/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <celix_bundle_activator.h>
#include <celix_log_helper.h>
#include <remote_service_admin.h>
#include <remote_constants.h>


struct remote_service_admin {
    celix_bundle_context_t *context;
    celix_log_helper_t *loghelper;
};

typedef struct my_remote_service_admin_activator {
    remote_service_admin_t admin;
    remote_service_admin_service_t adminService;
    long svcIdRsa;
} my_remote_service_admin_activator_t;

static celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t *admin, char *serviceId, celix_properties_t *properties, celix_array_list_t** registrations) {
    (void)properties;
    (void)registrations;
    celix_logHelper_info(admin->loghelper, "%s called: serviceId=%s\n", __FUNCTION__, serviceId);
    return CELIX_SUCCESS;
}

static celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t *admin, celix_array_list_t** services) {
    celix_status_t status = CELIX_SUCCESS;
    (void)services;
    celix_logHelper_info(admin->loghelper, "%s called\n", __FUNCTION__);
    return status;
}

static celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t *admin, celix_array_list_t** services) {
    celix_status_t status = CELIX_SUCCESS;
    (void)services;
    celix_logHelper_info(admin->loghelper, "%s called\n", __FUNCTION__);
    return status;
}

static celix_status_t remoteServiceAdmin_importService(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, import_registration_t **out) {
    const char *importConfigs = celix_properties_get(endpointDescription->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    celix_logHelper_info(admin->loghelper, "%s called: %s\n", __FUNCTION__, importConfigs);
    return CELIX_ILLEGAL_ARGUMENT;
}

static celix_status_t remoteServiceAdmin_removeExportedService(remote_service_admin_t *admin, export_registration_t *registration) {
    (void)registration;
    celix_logHelper_info(admin->loghelper, "%s called\n", __FUNCTION__);
    return CELIX_SUCCESS;
}

static celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_t *admin, import_registration_t *registration) {
    (void)registration;
    celix_logHelper_info(admin->loghelper, "%s called\n", __FUNCTION__);
    return CELIX_SUCCESS;
}

celix_status_t remoteServiceAdmin_destroyEndpointDescription(endpoint_description_t **description)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

static celix_status_t my_rsa_start(my_remote_service_admin_activator_t* activator, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;
    activator->admin.context = ctx;
    activator->admin.loghelper = celix_logHelper_create(ctx, "my_rsa");
    activator->svcIdRsa = -1;

    if (status == CELIX_SUCCESS) {
        activator->adminService.admin = &activator->admin;
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

        activator->svcIdRsa = celix_bundleContext_registerService(ctx, &activator->adminService, CELIX_RSA_REMOTE_SERVICE_ADMIN, NULL);
    }

    return status;
}

static celix_status_t my_rsa_stop(my_remote_service_admin_activator_t* activator, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, activator->svcIdRsa);
    celix_logHelper_destroy(activator->admin.loghelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(my_remote_service_admin_activator_t, my_rsa_start, my_rsa_stop)
