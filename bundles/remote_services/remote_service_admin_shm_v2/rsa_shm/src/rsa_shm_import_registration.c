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

#include "rsa_shm_import_registration.h"
#include "celix_rsa_rpc_factory.h"
#include "celix_log_helper.h"
#include "celix_stdlib_cleanup.h"
#include <string.h>

struct import_registration {
    celix_bundle_context_t *context;
    celix_log_helper_t *logHelper;
    endpoint_description_t *endpointDesc;
    const celix_rsa_rpc_factory_t *rpcFac;
    long proxyId;
};

celix_status_t importRegistration_create(celix_bundle_context_t* context, celix_log_helper_t* logHelper,
            endpoint_description_t* endpointDesc, celix_rsa_send_request_fp sendRequest, void* sendRequestHandle,
            const celix_rsa_rpc_factory_t* rpcFac, import_registration_t** importOut) {
    if (context == NULL || logHelper == NULL || endpointDescription_isInvalid(endpointDesc)
            || sendRequest == NULL || rpcFac == NULL || importOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree import_registration_t *import = (import_registration_t *)calloc(1, sizeof(*import));
    if (import == NULL) {
        return CELIX_ENOMEM;
    }
    import->context = context;
    import->logHelper = logHelper;

    import->endpointDesc = endpointDescription_clone(endpointDesc);
    if (import->endpointDesc == NULL) {
        return CELIX_ENOMEM;
    }
    celix_autoptr(endpoint_description_t) epDesc = import->endpointDesc;

    import->rpcFac = rpcFac;
    celix_status_t status = rpcFac->createProxy(rpcFac->handle, endpointDesc, sendRequest, sendRequestHandle, &import->proxyId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA import reg: Error creating %s proxy. %d.", endpointDesc->serviceName, status);
        return status;
    }

    celix_steal_ptr(epDesc);
    *importOut = celix_steal_ptr(import);

    return CELIX_SUCCESS;
}

void importRegistration_destroy(import_registration_t *import) {
    if (import != NULL) {
        import->rpcFac->destroyProxy(import->rpcFac->handle, import->proxyId);
        endpointDescription_destroy(import->endpointDesc);
        free(import);
    }
    return;
}

//LCOV_EXCL_START

celix_status_t importRegistration_getException(import_registration_t *registration CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t importRegistration_getImportReference(import_registration_t *registration CELIX_UNUSED, import_reference_t **reference CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_t *reference CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t importReference_getImportedService(import_reference_t *reference CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

//LCOV_EXCL_STOP

celix_status_t importRegistration_getImportedEndpoint(import_registration_t *registration,
        endpoint_description_t **endpoint) {
    if (registration == NULL || endpoint == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *endpoint = registration->endpointDesc;
    return CELIX_SUCCESS;
}
