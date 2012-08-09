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
 * export_registration.h
 *
 *  \date       Oct 6, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
