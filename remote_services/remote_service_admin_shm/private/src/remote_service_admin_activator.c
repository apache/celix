/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * remote_service_admin_activator.c
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "service_registration.h"

#include "remote_service_admin_shm_impl.h"
#include "export_registration_impl.h"
#include "import_registration_impl.h"

struct activator {
	apr_pool_t *pool;
	remote_service_admin_pt admin;
	service_registration_pt registration;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *parentPool = NULL;
	apr_pool_t *pool = NULL;
	struct activator *activator;

	status = bundleContext_getMemoryPool(context, &parentPool);
	if (status == CELIX_SUCCESS) {
		if (apr_pool_create(&pool, parentPool) != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			activator = apr_palloc(pool, sizeof(*activator));
			if (!activator) {
				status = CELIX_ENOMEM;
			} else {
				activator->pool = pool;
				activator->admin = NULL;
				activator->registration = NULL;

				*userData = activator;
			}
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	remote_service_admin_service_pt remoteServiceAdmin = NULL;

	status = remoteServiceAdmin_create(activator->pool, context, &activator->admin);
	if (status == CELIX_SUCCESS) {
		remoteServiceAdmin = apr_palloc(activator->pool, sizeof(*remoteServiceAdmin));
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

			remoteServiceAdmin->exportRegistration_close = exportRegistration_close;
			remoteServiceAdmin->exportRegistration_getException = exportRegistration_getException;
			remoteServiceAdmin->exportRegistration_getExportReference = exportRegistration_getExportReference;

			remoteServiceAdmin->importReference_getImportedEndpoint = importReference_getImportedEndpoint;
			remoteServiceAdmin->importReference_getImportedService = importReference_getImportedService;

			remoteServiceAdmin->importRegistration_close = importRegistration_close;
			remoteServiceAdmin->importRegistration_getException = importRegistration_getException;
			remoteServiceAdmin->importRegistration_getImportReference = importRegistration_getImportReference;

			status = bundleContext_registerService(context, OSGI_RSA_REMOTE_SERVICE_ADMIN, remoteServiceAdmin, NULL, &activator->registration);
		}
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	remoteServiceAdmin_stop(activator->admin);
	serviceRegistration_unregister(activator->registration);
	activator->registration = NULL;


	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	return status;
}


