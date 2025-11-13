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

#include "rsa_shm_export_registration.h"
#include "celix_rsa_rpc_factory.h"
#include "endpoint_description.h"
#include "celix_log_helper.h"
#include "celix_ref.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include <string.h>
#include <assert.h>

struct export_reference {
    endpoint_description_t *endpoint;
    service_reference_pt reference;
};

struct export_registration {
    struct celix_ref ref;
    celix_bundle_context_t * context;
    celix_log_helper_t* logHelper;
    endpoint_description_t * endpointDesc;
    service_reference_pt reference;
    celix_thread_rwlock_t rpcFacLock;
    const celix_rsa_rpc_factory_t *rpcFac;
    long endpointId;
};

static void exportRegistration_destroy(export_registration_t *export);

celix_status_t exportRegistration_create(celix_bundle_context_t* context, celix_log_helper_t* logHelper,
                                         service_reference_pt reference, endpoint_description_t* endpointDesc,
                                         const celix_rsa_rpc_factory_t* rpcFac, export_registration_t** exportOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (context == NULL || logHelper == NULL || reference == NULL
        || endpointDescription_isInvalid(endpointDesc) || exportOut == NULL || rpcFac == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree export_registration_t *export = calloc(1, sizeof(*export));
    if (export == NULL) {
        celix_logHelper_error(logHelper,"RSA export reg: Failed to alloc registration.");
        return CELIX_ENOMEM;
    }
    celix_ref_init(&export->ref);
    export->context = context;
    export->logHelper = logHelper;

    status = celixThreadRwlock_create(&export->rpcFacLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA export reg: Failed to create rpcFac lock. %d.", status);
        return status;
    }
    celix_autoptr(celix_thread_rwlock_t) rpcFacLock = &export->rpcFacLock;

    celix_autoptr(endpoint_description_t) epDesc = endpointDescription_clone(endpointDesc);
    if (epDesc == NULL) {
        celix_logHelper_error(logHelper,"RSA export reg: Error cloning endpoint desc.");
        return CELIX_ENOMEM;
    }
    export->endpointDesc = epDesc;

    export->rpcFac = rpcFac;
    export->reference = reference;
    status = bundleContext_retainServiceReference(context, reference);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA export reg: Retain refrence for %s failed. %d.", endpointDesc->serviceName, status);
        return status;
    }
    celix_auto(celix_service_ref_guard_t) ref = celix_ServiceRefGuard_init(context, reference);

    status = rpcFac->createEndpoint(rpcFac->handle, epDesc, &export->endpointId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA export reg: Error creating %s endpoint. %d.", endpointDesc->serviceName, status);
        return status;
    }

    celix_steal_ptr(ref.reference);
    celix_steal_ptr(epDesc);
    celix_steal_ptr(rpcFacLock);
    *exportOut = celix_steal_ptr(export);

    return CELIX_SUCCESS;
}

void exportRegistration_stop(export_registration_t *export) {
    assert(export != NULL);
    celix_auto(celix_rwlock_wlock_guard_t) locker = celixRwlockWlockGuard_init(&export->rpcFacLock);
    export->rpcFac->destroyEndpoint(export->rpcFac->handle, export->endpointId);
    export->rpcFac = NULL;
}

void exportRegistration_addRef(export_registration_t *export) {
    assert(export != NULL);
    celix_ref_get(&export->ref);
    return ;
}

void exportRegistration_release(export_registration_t *export) {
    assert(export != NULL);
    celix_ref_put(&export->ref, (void*)exportRegistration_destroy);
    return ;
}

static void exportRegistration_destroy(export_registration_t *export) {
    assert(export != NULL);
    bundleContext_ungetServiceReference(export->context, export->reference);
    endpointDescription_destroy(export->endpointDesc);
    celixThreadRwlock_destroy(&export->rpcFacLock);
    free(export);
    return ;
}

celix_status_t exportRegistration_call(export_registration_t *export, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (export == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_auto(celix_rwlock_rlock_guard_t) locker = celixRwlockRlockGuard_init(&export->rpcFacLock);
    if (export->rpcFac != NULL) {
        status = export->rpcFac->handleRequest(export->rpcFac->handle, export->endpointId, metadata, request, response);
    } else {
        status = CELIX_ILLEGAL_STATE;
        celix_logHelper_error(export->logHelper, "RSA export reg: Error Handling request. Maybe rsa rpc factory service is stopping.");
    }

    return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_t *registration,
        export_reference_t **out) {
    if (registration == NULL || out == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    export_reference_t *ref = calloc(1, sizeof(*ref));
    if (ref == NULL) {
        return CELIX_ENOMEM;
    }

    ref->endpoint = registration->endpointDesc;
    ref->reference = registration->reference;

    *out = ref;

    return CELIX_SUCCESS;
}

celix_status_t exportReference_getExportedEndpoint(export_reference_t *reference,
        endpoint_description_t **endpoint) {
    if (reference == NULL || endpoint == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *endpoint = reference->endpoint;
    return CELIX_SUCCESS;
}

celix_status_t exportReference_getExportedService(export_reference_t *reference,
        service_reference_pt *ref) {
    if (reference == NULL || ref == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *ref = reference->reference;
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START

celix_status_t exportRegistration_getException(export_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

//LCOV_EXCL_STOP
