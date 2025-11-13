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

#ifndef _RSA_SHM_IMPORT_REGISTRATION_H_
#define _RSA_SHM_IMPORT_REGISTRATION_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "import_registration.h"
#include "endpoint_description.h"
#include "celix_rsa_rpc_factory.h"
#include "celix_log_helper.h"
#include "celix_types.h"
#include "celix_errno.h"


celix_status_t importRegistration_create(celix_bundle_context_t* context, celix_log_helper_t* logHelper,
                endpoint_description_t* endpointDesc, celix_rsa_send_request_fp sendRequest, void* sendRequestHandle,
                const celix_rsa_rpc_factory_t* rpcFac, import_registration_t** importRegOut);

void importRegistration_destroy(import_registration_t *import);

celix_status_t importRegistration_getException(import_registration_t *registration);

celix_status_t importRegistration_getImportReference(import_registration_t *registration,
        import_reference_t **reference);

celix_status_t importRegistration_getImportedEndpoint(import_registration_t *registration,
        endpoint_description_t **endpoint);

celix_status_t importReference_getImportedEndpoint(import_reference_t *reference);

celix_status_t importReference_getImportedService(import_reference_t *reference);

#ifdef __cplusplus
}
#endif

#endif /* RSA_SHM_IMPORT_REGISTRATION_H_ */
