/*
 * remote_service_admin_activator.c
 *
 *  Created on: Sep 30, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include "headers.h"
#include "bundle_activator.h"
#include "service_registration.h"

#include "remote_service_admin_impl.h"
#include "export_registration_impl.h"
#include "import_registration_impl.h"

struct activator {
	apr_pool_t *pool;
	remote_service_admin_t admin;
	SERVICE_REGISTRATION registration;
};

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
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

				*userData = activator;
			}
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	remote_service_admin_service_t remoteServiceAdmin = NULL;

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

			status = bundleContext_registerService(context, REMOTE_SERVICE_ADMIN, remoteServiceAdmin, NULL, &activator->registration);
		}
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	remoteServiceAdmin_stop(activator->admin);
	serviceRegistration_unregister(activator->registration);
	activator->registration = NULL;


	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	return status;
}


