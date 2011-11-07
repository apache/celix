/*
 * import_registration_impl.h
 *
 *  Created on: Oct 14, 2011
 *      Author: alexander
 */

#ifndef IMPORT_REGISTRATION_IMPL_H_
#define IMPORT_REGISTRATION_IMPL_H_

#include "headers.h"
#include "remote_service_admin.h"
#include "remote_proxy.h"

struct import_registration {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;
	remote_service_admin_t rsa;
	endpoint_description_t endpointDescription;

	SERVICE_TRACKER proxyTracker;

	remote_proxy_service_t proxy;

	bool closed;
};

celix_status_t importRegistration_create(apr_pool_t *pool, endpoint_description_t endpoint, remote_service_admin_t rsa, BUNDLE_CONTEXT context, import_registration_t *registration);
celix_status_t importRegistration_close(import_registration_t registration);
celix_status_t importRegistration_getException(import_registration_t registration);
celix_status_t importRegistration_getImportReference(import_registration_t registration);

celix_status_t importRegistration_setEndpointDescription(import_registration_t registration, endpoint_description_t endpointDescription);
celix_status_t importRegistration_startTracking(import_registration_t registration);
celix_status_t importRegistration_stopTracking(import_registration_t registration);

#endif /* IMPORT_REGISTRATION_IMPL_H_ */
