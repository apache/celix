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
 * import_registration_impl.h
 *
 *  \date       Oct 14, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef IMPORT_REGISTRATION_IMPL_H_
#define IMPORT_REGISTRATION_IMPL_H_

#include "remote_service_admin.h"
#include "remote_proxy.h"
#include "service_tracker.h"

struct import_registration {
	apr_pool_t *pool;
	bundle_context_pt context;
	remote_service_admin_pt rsa;
	endpoint_description_pt endpointDescription;

	service_tracker_pt proxyTracker;

	service_reference_pt reference;
	remote_proxy_service_pt proxy;
	import_reference_pt importReference;
	bundle_pt bundle;

	bool closed;
};

celix_status_t importRegistration_create(apr_pool_t *pool, endpoint_description_pt endpoint, remote_service_admin_pt rsa, bundle_context_pt context, import_registration_pt *registration);
celix_status_t importRegistration_open(import_registration_pt registration);
celix_status_t importRegistration_close(import_registration_pt registration);
celix_status_t importRegistration_getException(import_registration_pt registration);
celix_status_t importRegistration_getImportReference(import_registration_pt registration, import_reference_pt *reference);

celix_status_t importRegistration_setEndpointDescription(import_registration_pt registration, endpoint_description_pt endpointDescription);
celix_status_t importRegistration_setHandler(import_registration_pt registration, void * handler);
celix_status_t importRegistration_setCallback(import_registration_pt registration, sendToHandle callback);

celix_status_t importRegistration_startTracking(import_registration_pt registration);
celix_status_t importRegistration_stopTracking(import_registration_pt registration);

#endif /* IMPORT_REGISTRATION_IMPL_H_ */
