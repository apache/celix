/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef CELIX_EXPORT_REGISTRATION_H
#define CELIX_EXPORT_REGISTRATION_H

#include "celix_errno.h"
#include "endpoint_description.h"
#include "service_reference.h"

typedef struct export_registration *export_registration_pt;

typedef struct export_reference *export_reference_pt;

celix_status_t exportRegistration_close(export_registration_pt registration);
celix_status_t exportRegistration_getException(export_registration_pt registration);
celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *reference);

celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint);
celix_status_t exportReference_getExportedService(export_reference_pt reference);

#endif //CELIX_EXPORT_REGISTRATION_H
