/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef CELIX_IMPORT_REGISTRATION_DFI_H
#define CELIX_IMPORT_REGISTRATION_DFI_H

#include "import_registration.h"

#include <celix_errno.h>

typedef void (*send_func_type)(void *handle, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus);

celix_status_t importRegistration_create(bundle_context_pt context, endpoint_description_pt description, const char *classObject,
                                         import_registration_pt *import);
void importRegistration_destroy(import_registration_pt import);

celix_status_t importRegistration_setSendFn(import_registration_pt reg,
                                            send_func_type,
                                            void *handle);
celix_status_t importRegistration_start(import_registration_pt import);
celix_status_t importRegistration_stop(import_registration_pt import);

celix_status_t importRegistration_getService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **service);
celix_status_t importRegistration_ungetService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **service);

#endif //CELIX_IMPORT_REGISTRATION_DFI_H
