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
/**
 * remote_service_admin_activator.c
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "bundle_activator.h"
#include "service_registration.h"

#include "remote_service_admin_shm_impl.h"
#include "export_registration_impl.h"
#include "import_registration_impl.h"

struct activator {
	remote_service_admin_t *admin;
	remote_service_admin_service_t *adminService;
	service_registration_t *registration;
};

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator;

	activator = calloc(1, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	} else {
		activator->admin = NULL;
		activator->registration = NULL;

		*userData = activator;
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
	celix_status_t status;
	struct activator *activator = userData;
	remote_service_admin_service_t *remoteServiceAdmin = NULL;

	status = remoteServiceAdmin_create(context, &activator->admin);
	if (status == CELIX_SUCCESS) {
		remoteServiceAdmin = calloc(1, sizeof(*remoteServiceAdmin));
		if (!remoteServiceAdmin) {
			status = CELIX_ENOMEM;
		} else {
			remoteServiceAdmin->admin = activator->admin;
			remoteServiceAdmin->exportService = remoteServiceAdmin_exportService;

			remoteServiceAdmin->getExportedServices = remoteServiceAdmin_getExportedServices;
			remoteServiceAdmin->getImportedEndpoints = remoteServiceAdmin_getImportedEndpoints;
			remoteServiceAdmin->importService = remoteServiceAdmin_importService;

			remoteServiceAdmin->exportReference_getExportedEndpoint = exportReference_getExportedEndpoint;
			remoteServiceAdmin->exportReference_getExportedService = exportReference_getExportedService;

			remoteServiceAdmin->exportRegistration_close = remoteServiceAdmin_removeExportedService;
			remoteServiceAdmin->exportRegistration_getException = exportRegistration_getException;
			remoteServiceAdmin->exportRegistration_getExportReference = exportRegistration_getExportReference;

			remoteServiceAdmin->importReference_getImportedEndpoint = importReference_getImportedEndpoint;
			remoteServiceAdmin->importReference_getImportedService = importReference_getImportedService;

			remoteServiceAdmin->importRegistration_close = remoteServiceAdmin_removeImportedService;
			remoteServiceAdmin->importRegistration_getException = importRegistration_getException;
			remoteServiceAdmin->importRegistration_getImportReference = importRegistration_getImportReference;

			status = bundleContext_registerService(context, OSGI_RSA_REMOTE_SERVICE_ADMIN, remoteServiceAdmin, NULL, &activator->registration);
			activator->adminService = remoteServiceAdmin;
		}
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    serviceRegistration_unregister(activator->registration);
    activator->registration = NULL;

    remoteServiceAdmin_stop(activator->admin);

    remoteServiceAdmin_destroy(&activator->admin);

    free(activator->adminService);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	free(activator);

	return status;
}


