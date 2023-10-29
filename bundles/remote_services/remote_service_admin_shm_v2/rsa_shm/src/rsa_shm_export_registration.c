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
#include "rsa_request_handler_service.h"
#include "rsa_rpc_factory.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_log_helper.h"
#include "celix_ref.h"
#include "celix_api.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>

struct export_reference {
    endpoint_description_t *endpoint;
    service_reference_pt reference;
};

typedef struct export_request_handler_service_entry {
    struct celix_ref ref;
    celix_thread_rwlock_t lock; //projects below
    rsa_request_handler_service_t *reqHandlerSvc;
}export_request_handler_service_entry_t;

struct export_registration {
    struct celix_ref ref;
    celix_bundle_context_t * context;
    celix_log_helper_t* logHelper;
    endpoint_description_t * endpointDesc;
    service_reference_pt reference;
    long rpcSvcTrkId;
    rsa_rpc_factory_t *rpcFac;
    long reqHandlerSvcTrkId;
    long reqHandlerSvcId;
    export_request_handler_service_entry_t *reqHandlerSvcEntry;
};

static void exportRegistration_addRpcFac(void *handle, void *svc);
static void exportRegistration_removeRpcFac(void *handle, void *svc);
static void exportRegistration_addRequestHandlerSvc(void *handle, void *svc);
static void exportRegistration_removeRequestHandlerSvc(void *handle, void *svc);
static void exportRegistration_destroy(export_registration_t *export);
static export_request_handler_service_entry_t *exportRegistration_createReqHandlerSvcEntry(void);
static void exportRegistration_retainReqHandlerSvcEntry(export_request_handler_service_entry_t *reqHandlerSvcEntry);
static void exportRegistration_releaseReqHandlerSvcEntry(export_request_handler_service_entry_t *reqHandlerSvcEntry);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(export_request_handler_service_entry_t, exportRegistration_releaseReqHandlerSvcEntry)

celix_status_t exportRegistration_create(celix_bundle_context_t *context,
        celix_log_helper_t *logHelper, service_reference_pt reference,
        endpoint_description_t *endpointDesc, export_registration_t **exportOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (context == NULL || logHelper == NULL || reference == NULL
            || endpointDescription_isInvalid(endpointDesc) || exportOut == NULL) {
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

    celix_autoptr(endpoint_description_t) epDesc = endpointDescription_clone(endpointDesc);
    if (epDesc == NULL) {
        celix_logHelper_error(logHelper,"RSA export reg: Error cloning endpoint desc.");
        return CELIX_ENOMEM;
    }
    export->endpointDesc = epDesc;

    export->rpcFac = NULL;
    export->reqHandlerSvcTrkId = -1;
    export->reqHandlerSvcId = -1;
    export->reqHandlerSvcEntry = NULL;
    export->reference = reference;
    status = bundleContext_retainServiceReference(context, reference);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA export reg: Retain refrence for %s failed. %d.", endpointDesc->serviceName, status);
        return status;
    }
    celix_auto(celix_service_ref_guard_t) ref = celix_ServiceRefGuard_init(context, reference);
    celix_autoptr(export_request_handler_service_entry_t) reqHandlerSvcEntry = exportRegistration_createReqHandlerSvcEntry();
    if (reqHandlerSvcEntry == NULL) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error creating endpoint svc entry.");
        return CELIX_ENOMEM;
    }
    export->reqHandlerSvcEntry = reqHandlerSvcEntry;

    const char *serviceImportedConfigs = celix_properties_get(endpointDesc->properties,
            OSGI_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (serviceImportedConfigs == NULL) {
        celix_logHelper_error(logHelper,"RSA export reg: service.imported.configs property is not exist.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    char *rsaRpcType = NULL;
    celix_autofree char *icCopy = strdup(serviceImportedConfigs);
    const char delimiter[2] = ",";
    char *token, *savePtr;
    token = strtok_r(icCopy, delimiter, &savePtr);
    while (token != NULL) {
        rsaRpcType = celix_utils_trimInPlace(token);
        if (strncmp(rsaRpcType, RSA_RPC_TYPE_PREFIX, sizeof(RSA_RPC_TYPE_PREFIX) - 1) == 0) {
            break;
        }
        rsaRpcType = NULL;
        token = strtok_r(NULL, delimiter, &savePtr);
    }
    if (rsaRpcType == NULL) {
        celix_logHelper_error(logHelper,"RSA export reg: %s property is not exist.", RSA_RPC_TYPE_KEY);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char filter[128] = {0};
    int bytes = snprintf(filter, sizeof(filter), "(%s=%s)", RSA_RPC_TYPE_KEY, rsaRpcType);
    if (bytes >= sizeof(filter)) {
        celix_logHelper_error(logHelper,"RSA export reg: The value(%s) of %s is too long.", rsaRpcType, RSA_RPC_TYPE_KEY);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = filter;
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.filter.versionRange = RSA_RPC_FACTORY_USE_RANGE;
    opts.callbackHandle = export;
    opts.add = exportRegistration_addRpcFac;
    opts.remove = exportRegistration_removeRpcFac;
    export->rpcSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(context, &opts);
    if (export->rpcSvcTrkId < 0) {
        celix_logHelper_error(logHelper,"RSA export reg: Error Tracking service for %s.", RSA_RPC_FACTORY_NAME);
        return CELIX_SERVICE_EXCEPTION;
    }

    celix_steal_ptr(reqHandlerSvcEntry);
    celix_steal_ptr(ref.reference);
    celix_steal_ptr(epDesc);
    *exportOut = celix_steal_ptr(export);

    return CELIX_SUCCESS;
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

static void exportRegistration_destroyCallback(void* data) {
    assert(data != NULL);
    export_registration_t *export = (export_registration_t *)data;
    exportRegistration_releaseReqHandlerSvcEntry(export->reqHandlerSvcEntry);
    bundleContext_ungetServiceReference(export->context, export->reference);
    endpointDescription_destroy(export->endpointDesc);
    free(export);
    return;
}

static void exportRegistration_destroy(export_registration_t *export) {
    assert(export != NULL);
    celix_bundleContext_stopTrackerAsync(export->context, export->rpcSvcTrkId,
            export, exportRegistration_destroyCallback);
    return ;
}

static export_request_handler_service_entry_t *exportRegistration_createReqHandlerSvcEntry(void) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree export_request_handler_service_entry_t *reqHandlerSvcEntry =
            (export_request_handler_service_entry_t *)calloc(1, sizeof(*reqHandlerSvcEntry));
    if (reqHandlerSvcEntry == NULL) {
        return NULL;
    }
    celix_ref_init(&reqHandlerSvcEntry->ref);
    status = celixThreadRwlock_create(&reqHandlerSvcEntry->lock, NULL);
    if (status != CELIX_SUCCESS) {
        return NULL;
    }
    reqHandlerSvcEntry->reqHandlerSvc = NULL;
    return celix_steal_ptr(reqHandlerSvcEntry);
}

static void exportRegistration_retainReqHandlerSvcEntry(export_request_handler_service_entry_t *reqHandlerSvcEntry) {
    celix_ref_get(&reqHandlerSvcEntry->ref);
}

static bool exportRegistration_destroyReqHandlerSvcEntry(struct celix_ref *ref) {
    export_request_handler_service_entry_t *reqHandlerSvcEntry = (export_request_handler_service_entry_t *)ref;
    celixThreadRwlock_destroy(&reqHandlerSvcEntry->lock);
    free(reqHandlerSvcEntry);
    return true;
}

static void exportRegistration_releaseReqHandlerSvcEntry(export_request_handler_service_entry_t *reqHandlerSvcEntry) {
    (void)celix_ref_put(&reqHandlerSvcEntry->ref, exportRegistration_destroyReqHandlerSvcEntry);
}

static void exportRegistration_addRpcFac(void *handle, void *svc) {
    assert(handle != NULL);
    celix_status_t status = CELIX_SUCCESS;
    export_registration_t *export = (export_registration_t *)handle;

    if (export->rpcFac != NULL) {
        celix_logHelper_info(export->logHelper,"RSA export reg: A endpoint supports only one rpc service.");
        return;
    }
    celix_logHelper_info(export->logHelper,"RSA export reg: RSA rpc service add.");
    rsa_rpc_factory_t *rpcFac = (rsa_rpc_factory_t *)svc;
    long reqHandlerSvcId = -1;
    status = rpcFac->createEndpoint(rpcFac->handle, export->endpointDesc,
            &reqHandlerSvcId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error Installing %s endpoint. %d.",
                export->endpointDesc->serviceName, status);
        return;
    }

    char filter[32] = {0};// It is longer than the size of "service.id" + reqHandlerSvcId
    (void)snprintf(filter, sizeof(filter), "(%s=%ld)", CELIX_FRAMEWORK_SERVICE_ID,
            reqHandlerSvcId);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = filter;
    opts.filter.serviceName = RSA_REQUEST_HANDLER_SERVICE_NAME;
    opts.filter.versionRange = RSA_REQUEST_HANDLER_SERVICE_USE_RANGE;
    opts.callbackHandle = export->reqHandlerSvcEntry;
    // exportRegistration_removeRequestHandlerSvc maybe occur after exportRegistration_destroyCallback. Therefore,Using refrence count here.
    exportRegistration_retainReqHandlerSvcEntry(export->reqHandlerSvcEntry);
    opts.add = exportRegistration_addRequestHandlerSvc;
    opts.remove = exportRegistration_removeRequestHandlerSvc;
    export->reqHandlerSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(export->context, &opts);
    if (export->reqHandlerSvcTrkId < 0) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error Tracking service for %s(%ld)", RSA_REQUEST_HANDLER_SERVICE_NAME, export->reqHandlerSvcId);
        exportRegistration_releaseReqHandlerSvcEntry(export->reqHandlerSvcEntry);
        rpcFac->destroyEndpoint(rpcFac->handle, reqHandlerSvcId);
        return;
    }
    export->reqHandlerSvcId = reqHandlerSvcId;
    export->rpcFac = (rsa_rpc_factory_t *)svc;
    return;
}

static void exportRegistration_removeRpcFac(void *handle, void *svc) {
    assert(handle != NULL);
    export_registration_t *export = (export_registration_t *)handle;

    if (export->rpcFac != svc) {
        celix_logHelper_info(export->logHelper,"RSA import reg: A endponit supports only one rpc service.");
        return;
    }

    celix_logHelper_info(export->logHelper,"RSA import reg: RSA rpc service remove.");

    rsa_rpc_factory_t *rpcFac = (rsa_rpc_factory_t *)svc;
    if (rpcFac != NULL && export->reqHandlerSvcId >= 0) {
        celix_bundleContext_stopTrackerAsync(export->context, export->reqHandlerSvcTrkId,
                export->reqHandlerSvcEntry, (void*)exportRegistration_releaseReqHandlerSvcEntry);
        rpcFac->destroyEndpoint(rpcFac->handle, export->reqHandlerSvcId);
        export->reqHandlerSvcId = -1;
        export->rpcFac = NULL;
    }
    return;
}


static void exportRegistration_addRequestHandlerSvc(void *handle, void *svc) {
    assert(handle != NULL);
    assert(svc != NULL);
    struct export_request_handler_service_entry *reqHandlerSvcEntry =
            (struct export_request_handler_service_entry *)handle;
    celix_auto(celix_rwlock_wlock_guard_t) locker = celixRwlockWlockGuard_init(&reqHandlerSvcEntry->lock);
    reqHandlerSvcEntry->reqHandlerSvc = (rsa_request_handler_service_t *)svc;
    return;
}

static void exportRegistration_removeRequestHandlerSvc(void *handle, void *svc) {
    assert(handle != NULL);
    assert(svc != NULL);
    struct export_request_handler_service_entry *reqHandlerSvcEntry =
            (struct export_request_handler_service_entry *)handle;
    celix_auto(celix_rwlock_wlock_guard_t) locker = celixRwlockWlockGuard_init(&reqHandlerSvcEntry->lock);
    if (svc == reqHandlerSvcEntry->reqHandlerSvc) {
        reqHandlerSvcEntry->reqHandlerSvc = NULL;
    }
    return;
}

celix_status_t exportRegistration_call(export_registration_t *export, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (export == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    struct export_request_handler_service_entry *reqHandlerSvcEntry = export->reqHandlerSvcEntry;
    assert(reqHandlerSvcEntry != NULL);
    celix_auto(celix_rwlock_rlock_guard_t) locker = celixRwlockRlockGuard_init(&reqHandlerSvcEntry->lock);
    if (reqHandlerSvcEntry->reqHandlerSvc != NULL) {
        status =  reqHandlerSvcEntry->reqHandlerSvc->handleRequest(reqHandlerSvcEntry->reqHandlerSvc->handle, metadata, request, response);
    } else {
        status = CELIX_ILLEGAL_STATE;
        celix_logHelper_error(export->logHelper, "RSA export reg: Error Handling request. Please ensure rsa rpc servie is active.");
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
