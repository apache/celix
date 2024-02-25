/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CELIX_IMPORT_REGISTRATION_DFI_H
#define CELIX_IMPORT_REGISTRATION_DFI_H

#include "import_registration.h"
#include "dfi_utils.h"
#include "endpoint_description.h"

#include <celix_errno.h>

typedef celix_status_t (*send_func_type)(void *handle, endpoint_description_t *endpointDescription, char *request, celix_properties_t *metadata, char **reply);

celix_status_t importRegistration_create(
        celix_bundle_context_t *context,
        endpoint_description_t *description,
        const char *classObject,
        const char* serviceVersion,
        send_func_type sendFn,
        void* sendFnHandle,
        FILE *logFile,
        import_registration_t **import);
void importRegistration_destroy(import_registration_t *import);

celix_status_t importRegistration_start(import_registration_t *import);

endpoint_description_t* importRegistration_getEndpointDescription(import_registration_t *registration);

#endif //CELIX_IMPORT_REGISTRATION_DFI_H
