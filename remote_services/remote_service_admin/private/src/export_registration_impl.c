/*
 * export_registration_impl.c
 *
 *  Created on: Oct 6, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include <apr_strings.h>

#include "celix_errno.h"

#include "export_registration_impl.h"
#include "remote_service_admin_impl.h"
#include "remote_endpoint.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "bundle.h"
#include "celix_log.h"

celix_status_t exportRegistration_endpointAdding(void * handle, SERVICE_REFERENCE reference, void **service);
celix_status_t exportRegistration_endpointAdded(void * handle, SERVICE_REFERENCE reference, void *service);
celix_status_t exportRegistration_endpointModified(void * handle, SERVICE_REFERENCE reference, void *service);
celix_status_t exportRegistration_endpointRemoved(void * handle, SERVICE_REFERENCE reference, void *service);

celix_status_t exportRegistration_createEndpointTracker(export_registration_t registration, SERVICE_TRACKER *tracker);

celix_status_t exportRegistration_create(apr_pool_t *pool, SERVICE_REFERENCE reference, endpoint_description_t endpoint, remote_service_admin_t rsa, BUNDLE_CONTEXT context, export_registration_t *registration) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *mypool = NULL;
	apr_pool_create(&mypool, pool);

	*registration = apr_palloc(mypool, sizeof(**registration));
	if (!*registration) {
		status = CELIX_ENOMEM;
	} else {
		(*registration)->pool = mypool;
		(*registration)->context = context;
		(*registration)->closed = false;
		(*registration)->endpointDescription = endpoint;
		(*registration)->reference = reference;
		(*registration)->rsa = rsa;
		(*registration)->tracker = NULL;
		(*registration)->endpoint = NULL;
		(*registration)->endpointTracker = NULL;
		(*registration)->exportReference = NULL;
		(*registration)->bundle = NULL;
	}

	return status;
}

celix_status_t exportRegistration_startTracking(export_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->endpointTracker == NULL) {
		status = exportRegistration_createEndpointTracker(registration, &registration->endpointTracker);
		if (status == CELIX_SUCCESS) {
			status = serviceTracker_open(registration->endpointTracker);
		}
	}

	return status;
}

celix_status_t exportRegistration_stopTracking(export_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->endpointTracker != NULL) {
		status = serviceTracker_close(registration->endpointTracker);
		if (status != CELIX_SUCCESS) {
			celix_log("EXPORT_REGISTRATION: Could not close endpoint tracker");
		}
	}
	if (registration->tracker != NULL) {
		status = serviceTracker_close(registration->tracker);
		if (status != CELIX_SUCCESS) {
			celix_log("EXPORT_REGISTRATION: Could not close service tracker");
		}
	}

	return status;
}

celix_status_t exportRegistration_createEndpointTracker(export_registration_t registration, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	SERVICE_TRACKER_CUSTOMIZER custumizer = (SERVICE_TRACKER_CUSTOMIZER) apr_palloc(registration->pool, sizeof(*custumizer));
	if (!custumizer) {
		status = CELIX_ENOMEM;
	} else {
		custumizer->handle = registration;
		custumizer->addingService = exportRegistration_endpointAdding;
		custumizer->addedService = exportRegistration_endpointAdded;
		custumizer->modifiedService = exportRegistration_endpointModified;
		custumizer->removedService = exportRegistration_endpointRemoved;

		status = serviceTracker_create(registration->pool, registration->context, REMOTE_ENDPOINT, custumizer, tracker);
	}

	return status;
}

celix_status_t exportRegistration_endpointAdding(void * handle, SERVICE_REFERENCE reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_t registration = handle;

	status = bundleContext_getService(registration->context, reference, service);

	return status;
}

celix_status_t exportRegistration_endpointAdded(void * handle, SERVICE_REFERENCE reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_t registration = handle;

	remote_endpoint_service_t endpoint = service;
	if (registration->endpoint == NULL) {
		registration->endpoint = endpoint;
		void *service = NULL;
		status = bundleContext_getService(registration->context, registration->reference, &service);
		if (status == CELIX_SUCCESS) {
			endpoint->setService(endpoint->endpoint, service);
		}
	}

	return status;
}

celix_status_t exportRegistration_endpointModified(void * handle, SERVICE_REFERENCE reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_t registration = handle;

	return status;
}

celix_status_t exportRegistration_endpointRemoved(void * handle, SERVICE_REFERENCE reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_t registration = handle;

	remote_endpoint_service_t endpoint = service;
	if (registration->endpoint != NULL) {
		registration->endpoint = NULL;
		endpoint->setService(endpoint->endpoint, NULL);
	}

	return status;
}

celix_status_t exportRegistration_open(export_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;

	char *name = apr_pstrcat(registration->pool, BUNDLE_STORE, "/", registration->endpointDescription->service, "_endpoint.zip", NULL);
	status = bundleContext_installBundle(registration->context, name, &registration->bundle);
	if (status == CELIX_SUCCESS) {
		status = bundle_start(registration->bundle, 0);
		if (status == CELIX_SUCCESS) {
		}
	}

	return status;
}

celix_status_t exportRegistration_close(export_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;

	exportRegistration_stopTracking(registration);

	bundle_stop(registration->bundle, 0);
	bundle_uninstall(registration->bundle);

	return status;
}

celix_status_t exportRegistration_getException(export_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_t registration, export_reference_t *reference) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->exportReference == NULL) {
		registration->exportReference = apr_palloc(registration->pool, sizeof(*registration->exportReference));
		if (registration->exportReference == NULL) {
			status = CELIX_ENOMEM;
		} else {
			registration->exportReference->endpoint = registration->endpointDescription;
			registration->exportReference->reference = registration->reference;
		}
	}

	*reference = registration->exportReference;

	return status;
}

celix_status_t exportRegistration_setEndpointDescription(export_registration_t registration, endpoint_description_t endpointDescription) {
	celix_status_t status = CELIX_SUCCESS;

	registration->endpointDescription = endpointDescription;

	return status;
}
