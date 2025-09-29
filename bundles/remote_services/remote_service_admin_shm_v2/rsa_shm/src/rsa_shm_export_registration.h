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

#ifndef _RSA_SHM_EXPORT_REGISTRATION_H_
#define _RSA_SHM_EXPORT_REGISTRATION_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "export_registration.h"
#include "endpoint_description.h"
#include "celix_rsa_rpc_factory.h"
#include "celix_cleanup.h"
#include "celix_log_helper.h"
#include "celix_types.h"
#include "celix_properties.h"
#include "celix_errno.h"
#include <sys/uio.h>


celix_status_t exportRegistration_create(celix_bundle_context_t* context,
                                         celix_log_helper_t* logHelper, service_reference_pt reference,
                                         endpoint_description_t* endpointDesc, const celix_rsa_rpc_factory_t* rpcFac, export_registration_t** exportOut);

void exportRegistration_stop(export_registration_t *registration);

void exportRegistration_addRef(export_registration_t *registration);

void exportRegistration_release(export_registration_t *registration);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(export_registration_t, exportRegistration_release)

celix_status_t exportRegistration_getExportReference(export_registration_t *registration,
        export_reference_t **out);

celix_status_t exportRegistration_getException(export_registration_t *registration);

celix_status_t exportReference_getExportedEndpoint(export_reference_t *reference,
        endpoint_description_t **endpoint);

celix_status_t exportReference_getExportedService(export_reference_t *reference,
        service_reference_pt *ref);

celix_status_t exportRegistration_call(export_registration_t *exportReg, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_EXPORT_REGISTRATION_H_ */
