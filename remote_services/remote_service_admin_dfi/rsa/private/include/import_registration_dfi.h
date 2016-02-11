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

#ifndef CELIX_IMPORT_REGISTRATION_DFI_H
#define CELIX_IMPORT_REGISTRATION_DFI_H

#include "import_registration.h"
#include "dfi_utils.h"

#include <celix_errno.h>

typedef void (*send_func_type)(void *handle, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus);

celix_status_t importRegistration_create(bundle_context_pt context, endpoint_description_pt description, const char *classObject, const char* serviceVersion,
                                         import_registration_pt *import);
celix_status_t importRegistration_close(import_registration_pt import);
void importRegistration_destroy(import_registration_pt import);

celix_status_t importRegistration_setSendFn(import_registration_pt reg,
                                            send_func_type,
                                            void *handle);
celix_status_t importRegistration_start(import_registration_pt import);
celix_status_t importRegistration_stop(import_registration_pt import);

celix_status_t importRegistration_getService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **service);
celix_status_t importRegistration_ungetService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **service);

#endif //CELIX_IMPORT_REGISTRATION_DFI_H
