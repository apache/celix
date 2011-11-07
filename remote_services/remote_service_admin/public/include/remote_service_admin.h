/*
 * remote_service_admin.h
 *
 *  Created on: Sep 30, 2011
 *      Author: alexander
 */

#ifndef REMOTE_SERVICE_ADMIN_H_
#define REMOTE_SERVICE_ADMIN_H_

#include "endpoint_listener.h"

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
	celix_status_t (*importRegistration_getImportReference)(import_registration_t registration);

};

typedef struct remote_service_admin_service *remote_service_admin_service_t;


#endif /* REMOTE_SERVICE_ADMIN_H_ */
