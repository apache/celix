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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef IMPORT_REGISTRATION_IMPL_H_
#define IMPORT_REGISTRATION_IMPL_H_

#include "remote_service_admin.h"
#include "remote_proxy.h"
#include "service_tracker.h"
#include "log_helper.h"

struct import_registration {
	bundle_context_pt context;
	endpoint_description_pt endpointDescription;

	service_reference_pt reference;
	import_reference_pt importReference;

	remote_service_admin_pt rsa;
	sendToHandle sendToCallback;

	bool closed;
};



struct import_registration_factory
{
	char* serviceName;
	log_helper_pt loghelper;
	remote_proxy_factory_service_pt trackedFactory;
	service_tracker_pt proxyFactoryTracker;
	bundle_context_pt context;
	array_list_pt registrations;
	bundle_pt bundle;
};


celix_status_t importRegistration_create(endpoint_description_pt endpoint, remote_service_admin_pt rsa, sendToHandle callback, bundle_context_pt context, import_registration_pt *registration);
celix_status_t importRegistration_destroy(import_registration_pt registration);

celix_status_t importRegistration_setEndpointDescription(import_registration_pt registration, endpoint_description_pt endpointDescription);
celix_status_t importRegistration_setHandler(import_registration_pt registration, void * handler);
celix_status_t importRegistration_setCallback(import_registration_pt registration, sendToHandle callback);

celix_status_t importRegistration_getException(import_registration_pt registration);
celix_status_t importRegistration_getImportReference(import_registration_pt registration, import_reference_pt *reference);

celix_status_t importRegistration_createProxyFactoryTracker(import_registration_factory_pt registration_factory, service_tracker_pt *tracker);

//celix_status_t importRegistrationFactory_create(apr_pool_t *pool, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory);
celix_status_t importRegistrationFactory_destroy(import_registration_factory_pt* registration_factory);
//celix_status_t importRegistrationFactory_open(import_registration_factory_pt registration_factory);
celix_status_t importRegistrationFactory_close(import_registration_factory_pt registration_factory);
celix_status_t importRegistrationFactory_install(log_helper_pt helper, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory);



#endif /* IMPORT_REGISTRATION_IMPL_H_ */
