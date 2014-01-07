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
 * export_registration_impl.c
 *
 *  \date       Oct 6, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include <apr_strings.h>

#include "celix_errno.h"

#include "export_registration_impl.h"
#include "remote_service_admin_impl.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "bundle.h"
#include "celix_log.h"

celix_status_t exportRegistration_endpointAdding(void * handle, service_reference_pt reference, void **service);
celix_status_t exportRegistration_endpointAdded(void * handle, service_reference_pt reference, void *service);
celix_status_t exportRegistration_endpointModified(void * handle, service_reference_pt reference, void *service);
celix_status_t exportRegistration_endpointRemoved(void * handle, service_reference_pt reference, void *service);

celix_status_t exportRegistration_createEndpointTracker(export_registration_pt registration, service_tracker_pt *tracker);

celix_status_t exportRegistration_create(apr_pool_t *pool, service_reference_pt reference, endpoint_description_pt endpoint, remote_service_admin_pt rsa, bundle_context_pt context, export_registration_pt *registration) {
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

celix_status_t exportRegistration_startTracking(export_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->endpointTracker == NULL) {
		status = exportRegistration_createEndpointTracker(registration, &registration->endpointTracker);
		if (status == CELIX_SUCCESS) {
			status = serviceTracker_open(registration->endpointTracker);
		}
	}

	return status;
}

celix_status_t exportRegistration_stopTracking(export_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->endpointTracker != NULL) {
		status = serviceTracker_close(registration->endpointTracker);
		if (status != CELIX_SUCCESS) {
		    fw_log(OSGI_FRAMEWORK_LOG_ERROR, "EXPORT_REGISTRATION: Could not close endpoint tracker");
		}
	}
	if (registration->tracker != NULL) {
		status = serviceTracker_close(registration->tracker);
		if (status != CELIX_SUCCESS) {
		    fw_log(OSGI_FRAMEWORK_LOG_ERROR, "EXPORT_REGISTRATION: Could not close service tracker");
		}
	}

	return status;
}

celix_status_t exportRegistration_createEndpointTracker(export_registration_pt registration, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(registration->pool, registration, exportRegistration_endpointAdding,
			exportRegistration_endpointAdded, exportRegistration_endpointModified, exportRegistration_endpointRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(registration->pool, registration->context, OSGI_RSA_REMOTE_ENDPOINT, customizer, tracker);
	}

	return status;
}

celix_status_t exportRegistration_endpointAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_pt registration = handle;

	status = bundleContext_getService(registration->context, reference, service);

	return status;
}

celix_status_t exportRegistration_endpointAdded(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_pt registration = handle;

	remote_endpoint_service_pt endpoint = service;
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

celix_status_t exportRegistration_endpointModified(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_pt registration = handle;

	return status;
}

celix_status_t exportRegistration_endpointRemoved(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	export_registration_pt registration = handle;

	remote_endpoint_service_pt endpoint = service;
	if (registration->endpoint != NULL) {
		registration->endpoint = NULL;
		endpoint->setService(endpoint->endpoint, NULL);
	}

	return status;
}

celix_status_t exportRegistration_open(export_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	char *bundleStore = NULL;
	bundleContext_getProperty(registration->context, BUNDLE_STORE_PROPERTY_NAME, &bundleStore);
	if (bundleStore == NULL) {
		bundleStore = DEFAULT_BUNDLE_STORE;
	}
	char *name = apr_pstrcat(registration->pool, bundleStore, "/", registration->endpointDescription->service, "_endpoint.zip", NULL);
	status = bundleContext_installBundle(registration->context, name, &registration->bundle);
	if (status == CELIX_SUCCESS) {
		status = bundle_start(registration->bundle);
		if (status == CELIX_SUCCESS) {
		}
	}

	return status;
}

celix_status_t exportRegistration_close(export_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	exportRegistration_stopTracking(registration);
	bundle_stop(registration->bundle);
	bundle_uninstall(registration->bundle);
	remoteServiceAdmin_removeExportedService(registration);

	return status;
}

celix_status_t exportRegistration_getException(export_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *reference) {
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

celix_status_t exportRegistration_setEndpointDescription(export_registration_pt registration, endpoint_description_pt endpointDescription) {
	celix_status_t status = CELIX_SUCCESS;

	registration->endpointDescription = endpointDescription;

	return status;
}
