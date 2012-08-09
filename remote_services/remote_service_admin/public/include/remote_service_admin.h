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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SERVICE_ADMIN_H_
#define REMOTE_SERVICE_ADMIN_H_

#include "endpoint_listener.h"
#include "service_reference.h"

#define REMOTE_SERVICE_ADMIN "remote_service_admin"

typedef struct export_reference *export_reference_t;
typedef struct export_registration *export_registration_t;
typedef struct import_reference *import_reference_t;
typedef struct import_registration *import_registration_t;
typedef struct remote_service_admin *remote_service_admin_t;

struct remote_service_admin_service {
	remote_service_admin_t admin;
	celix_status_t (*exportService)(remote_service_admin_t admin, SERVICE_REFERENCE reference, PROPERTIES properties, ARRAY_LIST *registrations);
	celix_status_t (*getExportedServices)(remote_service_admin_t admin, ARRAY_LIST *services);
	celix_status_t (*getImportedEndpoints)(remote_service_admin_t admin, ARRAY_LIST *services);
	celix_status_t (*importService)(remote_service_admin_t admin, endpoint_description_t endpoint, import_registration_t *registration);


	celix_status_t (*exportReference_getExportedEndpoint)(export_reference_t reference, endpoint_description_t *endpoint);
	celix_status_t (*exportReference_getExportedService)(export_reference_t reference);

	celix_status_t (*exportRegistration_close)(export_registration_t registration);
	celix_status_t (*exportRegistration_getException)(export_registration_t registration);
	celix_status_t (*exportRegistration_getExportReference)(export_registration_t registration, export_reference_t *reference);

	celix_status_t (*importReference_getImportedEndpoint)(import_reference_t reference);
	celix_status_t (*importReference_getImportedService)(import_reference_t reference);

	celix_status_t (*importRegistration_close)(import_registration_t registration);
	celix_status_t (*importRegistration_getException)(import_registration_t registration);
	celix_status_t (*importRegistration_getImportReference)(import_registration_t registration, import_reference_t *reference);

};

typedef struct remote_service_admin_service *remote_service_admin_service_t;


#endif /* REMOTE_SERVICE_ADMIN_H_ */
