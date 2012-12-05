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
 * remote_service_admin_impl.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SERVICE_ADMIN_IMPL_H_
#define REMOTE_SERVICE_ADMIN_IMPL_H_

#include "remote_service_admin.h"
#include "mongoose.h"

#define BUNDLE_STORE_PROPERTY_NAME "RS_BUNDLE"
#define DEFAULT_BUNDLE_STORE "rs_bundles"

struct export_reference {
	endpoint_description_t endpoint;
	service_reference_t reference;
};

struct import_reference {
	endpoint_description_t endpoint;
	service_reference_t reference;
};

struct remote_service_admin {
	apr_pool_t *pool;
	bundle_context_t context;

	hash_map_t exportedServices;
	hash_map_t importedServices;

	struct mg_context *ctx;
};

celix_status_t remoteServiceAdmin_create(apr_pool_t *pool, bundle_context_t context, remote_service_admin_t *admin);
celix_status_t remoteServiceAdmin_stop(remote_service_admin_t admin);

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t admin, service_reference_t reference, properties_t properties, array_list_t *registrations);
celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t admin, array_list_t *services);
celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t admin, array_list_t *services);
celix_status_t remoteServiceAdmin_importService(remote_service_admin_t admin, endpoint_description_t endpoint, import_registration_t *registration);


celix_status_t exportReference_getExportedEndpoint(export_reference_t reference, endpoint_description_t *endpoint);
celix_status_t exportReference_getExportedService(export_reference_t reference);

celix_status_t importReference_getImportedEndpoint(import_reference_t reference);
celix_status_t importReference_getImportedService(import_reference_t reference);

#endif /* REMOTE_SERVICE_ADMIN_IMPL_H_ */
