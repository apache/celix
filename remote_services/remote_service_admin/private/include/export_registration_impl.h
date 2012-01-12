/*
 * export_registration.h
 *
 *  Created on: Oct 6, 2011
 *      Author: alexander
 */

#ifndef EXPORT_REGISTRATION_IMPL_H_
#define EXPORT_REGISTRATION_IMPL_H_

#include "remote_service_admin.h"
#include "remote_endpoint.h"
#include "service_tracker.h"

struct export_registration {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;
	remote_service_admin_t rsa;
	endpoint_description_t endpointDescription;
	SERVICE_REFERENCE reference;

	SERVICE_TRACKER tracker;
	SERVICE_TRACKER endpointTracker;

	remote_endpoint_service_t endpoint;

	export_reference_t exportReference;
	BUNDLE bundle;

	bool closed;
};

celix_status_t exportRegistration_create(apr_pool_t *pool, SERVICE_REFERENCE reference, endpoint_description_t endpoint, remote_service_admin_t rsa, BUNDLE_CONTEXT context, export_registration_t *registration);
celix_status_t exportRegistration_open(export_registration_t registration);
celix_status_t exportRegistration_close(export_registration_t registration);
celix_status_t exportRegistration_getException(export_registration_t registration);
celix_status_t exportRegistration_getExportReference(export_registration_t registration, export_reference_t *reference);

celix_status_t exportRegistration_setEndpointDescription(export_registration_t registration, endpoint_description_t endpointDescription);
celix_status_t exportRegistration_startTracking(export_registration_t registration);
celix_status_t exportRegistration_stopTracking(export_registration_t registration);

#endif /* EXPORT_REGISTRATION_IMPL_H_ */
