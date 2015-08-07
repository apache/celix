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
 * remote_service_admin.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SERVICE_ADMIN_H_
#define REMOTE_SERVICE_ADMIN_H_

#include "endpoint_listener.h"
#include "service_reference.h"
#include "export_registration.h"
#include "import_registration.h"

#define OSGI_RSA_REMOTE_SERVICE_ADMIN "remote_service_admin"

typedef struct import_registration_factory *import_registration_factory_pt;
typedef struct remote_service_admin *remote_service_admin_pt;

struct remote_service_admin_service {
	remote_service_admin_pt admin;
	celix_status_t (*exportService)(remote_service_admin_pt admin, char *serviceId, properties_pt properties, array_list_pt *registrations);
	celix_status_t (*removeExportedService)(export_registration_pt registration);
	celix_status_t (*getExportedServices)(remote_service_admin_pt admin, array_list_pt *services);
	celix_status_t (*getImportedEndpoints)(remote_service_admin_pt admin, array_list_pt *services);
	celix_status_t (*importService)(remote_service_admin_pt admin, endpoint_description_pt endpoint, import_registration_pt *registration);

	celix_status_t (*exportReference_getExportedEndpoint)(export_reference_pt reference, endpoint_description_pt *endpoint);
	celix_status_t (*exportReference_getExportedService)(export_reference_pt reference);

	celix_status_t (*exportRegistration_close)(export_registration_pt registration);
	celix_status_t (*exportRegistration_getException)(export_registration_pt registration);
	celix_status_t (*exportRegistration_getExportReference)(export_registration_pt registration, export_reference_pt *reference);
	celix_status_t (*exportRegistration_freeExportReference)(export_reference_pt *reference);
	celix_status_t (*exportRegistration_getEndpointDescription)(export_registration_pt registration, endpoint_description_pt endpointDescription);

	celix_status_t (*importReference_getImportedEndpoint)(import_reference_pt reference);
	celix_status_t (*importReference_getImportedService)(import_reference_pt reference);

	celix_status_t (*importRegistration_close)(remote_service_admin_pt admin, import_registration_pt registration);
	celix_status_t (*importRegistration_getException)(import_registration_pt registration);
	celix_status_t (*importRegistration_getImportReference)(import_registration_pt registration, import_reference_pt *reference);

};

typedef struct remote_service_admin_service *remote_service_admin_service_pt;


#endif /* REMOTE_SERVICE_ADMIN_H_ */
