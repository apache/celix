/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef CELIX_IMPORT_REGISTRATION_H
#define CELIX_IMPORT_REGISTRATION_H

#include "celix_errno.h"
#include "endpoint_description.h"
#include "service_reference.h"

typedef struct import_registration *import_registration_pt;

typedef struct import_reference *import_reference_pt;

celix_status_t importRegistration_close(import_registration_pt registration);
celix_status_t importRegistration_getException(import_registration_pt registration);
celix_status_t importRegistration_getImportReference(import_registration_pt registration, import_reference_pt *reference);

celix_status_t importReference_getImportedEndpoint(import_reference_pt reference);
celix_status_t importReference_getImportedService(import_reference_pt reference);

#endif //CELIX_IMPORT_REGISTRATION_H
