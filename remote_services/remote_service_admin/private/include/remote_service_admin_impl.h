/*
 * remote_service_admin_impl.h
 *
 *  Created on: Sep 30, 2011
 *      Author: alexander
 */

#ifndef REMOTE_SERVICE_ADMIN_IMPL_H_
#define REMOTE_SERVICE_ADMIN_IMPL_H_

#include "remote_service_admin.h"
#include "mongoose.h"

#define BUNDLE_STORE_PROPERTY_NAME "RS_BUNDLE"
#define DEFAULT_BUNDLE_STORE "rs_bundles"

struct export_reference {
	endpoint_description_t endpoint;
	SERVICE_REFERENCE reference;
};

struct import_reference {
	endpoint_description_t endpoint;
	SERVICE_REFERENCE reference;
};

struct remote_service_admin {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;

	HASH_MAP exportedServices;
	HASH_MAP importedServices;

	struct mg_context *ctx;
};

celix_status_t remoteServiceAdmin_create(apr_pool_t *pool, BUNDLE_CONTEXT context, remote_service_admin_t *admin);
celix_status_t remoteServiceAdmin_stop(remote_service_admin_t admin);

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t admin, SERVICE_REFERENCE reference, PROPERTIES properties, ARRAY_LIST *registrations);
celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t admin, ARRAY_LIST *services);
celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t admin, ARRAY_LIST *services);
celix_status_t remoteServiceAdmin_importService(remote_service_admin_t admin, endpoint_description_t endpoint, import_registration_t *registration);


celix_status_t exportReference_getExportedEndpoint(export_reference_t reference, endpoint_description_t *endpoint);
celix_status_t exportReference_getExportedService(export_reference_t reference);

celix_status_t importReference_getImportedEndpoint(import_reference_t reference);
celix_status_t importReference_getImportedService(import_reference_t reference);

#endif /* REMOTE_SERVICE_ADMIN_IMPL_H_ */
