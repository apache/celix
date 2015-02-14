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
 * export_registration_impl.h
 *
 *  \date       Oct 6, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EXPORT_REGISTRATION_IMPL_H_
#define EXPORT_REGISTRATION_IMPL_H_

#include "remote_service_admin.h"
#include "remote_endpoint.h"
#include "service_tracker.h"
#include "log_helper.h"

struct export_registration {
	bundle_context_pt context;
	remote_service_admin_pt rsa;
	endpoint_description_pt endpointDescription;
	service_reference_pt reference;
	log_helper_pt loghelper;

	service_tracker_pt tracker;
	service_tracker_pt endpointTracker;

	remote_endpoint_service_pt endpoint;

	export_reference_pt exportReference;
	bundle_pt bundle;

	bool closed;
};

celix_status_t exportRegistration_create(log_helper_pt helper, service_reference_pt reference, endpoint_description_pt endpoint, remote_service_admin_pt rsa, bundle_context_pt context, export_registration_pt *registration);
celix_status_t exportRegistration_destroy(export_registration_pt *registration);
celix_status_t exportRegistration_open(export_registration_pt registration);
celix_status_t exportRegistration_close(export_registration_pt registration);
celix_status_t exportRegistration_getException(export_registration_pt registration);
celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *reference);

celix_status_t exportRegistration_setEndpointDescription(export_registration_pt registration, endpoint_description_pt endpointDescription);
celix_status_t exportRegistration_startTracking(export_registration_pt registration);
celix_status_t exportRegistration_stopTracking(export_registration_pt registration);

#endif /* EXPORT_REGISTRATION_IMPL_H_ */
