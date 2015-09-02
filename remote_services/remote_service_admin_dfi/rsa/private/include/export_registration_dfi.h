/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef CELIX_EXPORT_REGISTRATION_DFI_H
#define CELIX_EXPORT_REGISTRATION_DFI_H


#include "export_registration.h"
#include "log_helper.h"
#include "endpoint_description.h"

celix_status_t exportRegistration_create(log_helper_pt helper, void (*closedCallback)(void *handle, export_registration_pt reg), void *handle, service_reference_pt reference, endpoint_description_pt endpoint, bundle_context_pt context, export_registration_pt *registration);
void exportRegistration_destroy(export_registration_pt registration);

celix_status_t exportRegistration_start(export_registration_pt registration);
celix_status_t exportRegistration_stop(export_registration_pt registration);

celix_status_t exportRegistration_call(export_registration_pt export, char *data, int datalength, char **response, int *responseLength);


#endif //CELIX_EXPORT_REGISTRATION_DFI_H
