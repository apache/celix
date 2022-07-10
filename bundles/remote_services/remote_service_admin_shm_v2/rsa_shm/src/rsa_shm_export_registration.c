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

#include <rsa_shm_export_registration.h>
#include <rsa_rpc_service.h>
#include <rsa_rpc_endpoint_service.h>
#include <rsa_shm_constants.h>
#include <endpoint_description.h>
#include <remote_constants.h>
#include <celix_log_helper.h>
#include <celix_ref.h>
#include <celix_api.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

struct export_reference {
    endpoint_description_t *endpoint;
    service_reference_pt reference;
};

typedef struct export_endpoint_service_entry {
    struct celix_ref ref;
    celix_thread_mutex_t mutex; //projects below
    rsa_rpc_endpoint_service_t *endpointSvc;
}export_endpoint_service_entry_t;

struct export_registration {
    struct celix_ref ref;
    celix_bundle_context_t * context;
    celix_log_helper_t* logHelper;
    endpoint_description_t * endpointDesc;
    service_reference_pt reference;
    long rpcSvcTrkId;
    rsa_rpc_service_t *rpcSvc;
    long endpointSvcTrkId;
    long endpointSvcId;
    struct export_endpoint_service_entry *endpointSvcEntry;
};

static void exportRegistration_addRpcSvc(void *handle, void *svc);
static void exportRegistration_removeRpcSvc(void *handle, void *svc);
static void exportRegistration_addEndpointSvc(void *handle, void *svc);
static void exportRegistration_removeEndpointSvc(void *handle, void *svc);
static void exportRegistration_destroy(export_registration_t *export);
static export_endpoint_service_entry_t *exportRegistration_createEpSvcEntry(void);
static void exportRegistration_retainEpSvcEntry(export_endpoint_service_entry_t *epSvcEntry);
static void exportRegistration_releaseEpSvcEntry(export_endpoint_service_entry_t *epSvcEntry);

celix_status_t exportRegistration_create(celix_bundle_context_t *context,
        celix_log_helper_t *logHelper, service_reference_pt reference,
        endpoint_description_t *endpointDesc, export_registration_t **exportOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (context == NULL || logHelper == NULL || reference == NULL
            || endpointDescription_isInvalid(endpointDesc) || exportOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    export_registration_t *export = calloc(1, sizeof(*export));
    assert(export != NULL);
    celix_ref_init(&export->ref);
    export->context = context;
    export->logHelper = logHelper;

    //copy endpoint description
    export->endpointDesc = (endpoint_description_t*)calloc(1, sizeof(endpoint_description_t));
    assert(export->endpointDesc != NULL);
    export->endpointDesc->properties = celix_properties_copy(endpointDesc->properties);
    assert(export->endpointDesc->properties != NULL);
    export->endpointDesc->frameworkUUID = (char*)celix_properties_get(export->endpointDesc->properties,
            OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    export->endpointDesc->serviceId = endpointDesc->serviceId;
    export->endpointDesc->id = (char*)celix_properties_get(export->endpointDesc->properties,
            OSGI_RSA_ENDPOINT_ID, NULL);
    export->endpointDesc->service = strdup(endpointDesc->service);
    assert(export->endpointDesc->service != NULL);

    export->rpcSvc = NULL;
    export->endpointSvcTrkId = -1;
    export->endpointSvcId = -1;
    export->endpointSvcEntry = NULL;
    export->reference = reference;
    status = bundleContext_retainServiceReference(context, reference);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"RSA export reg: Retain refrence for %s failed. %d.", endpointDesc->service,status);
        goto err_retaining_service_ref;
    }

    export->endpointSvcEntry = exportRegistration_createEpSvcEntry();
    if (export->endpointSvcEntry == NULL) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error creating endpoint svc entry.");
        status = CELIX_SERVICE_EXCEPTION;
        goto ep_svc_entry_err;
    }

    /* If the properties of exported service include 'RSA_RPC_TYPE_KEY',
      * then use the specified 'rsa_rpc_service' install the proxy of exported service.
      * Otherwise, use the default 'rsa_rpc_service' install the endpoint of exported service.
     */
     const char *rsaRpcType = celix_properties_get(endpointDesc->properties, RSA_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);
     char filter[128] = {0};
     int bytes = snprintf(filter, sizeof(filter), "(%s=%s)", RSA_RPC_TYPE_KEY, rsaRpcType);
     if (bytes >= sizeof(filter)) {
         celix_logHelper_error(logHelper,"RSA export reg: The value(%s) of %s is too long.", rsaRpcType, RSA_RPC_TYPE_KEY);
         status = CELIX_ILLEGAL_ARGUMENT;
         goto rpc_type_filter_err;
     }
     celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
     opts.filter.filter = filter;
     opts.filter.serviceName = RSA_RPC_SERVICE_NAME;
     opts.filter.versionRange = RSA_RPC_SERVICE_USE_RANGE;
     opts.filter.ignoreServiceLanguage = true;
     opts.callbackHandle = export;
     opts.add = exportRegistration_addRpcSvc;
     opts.remove = exportRegistration_removeRpcSvc;
     export->rpcSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(context, &opts);
     if (export->rpcSvcTrkId < 0) {
         celix_logHelper_error(logHelper,"RSA export reg: Error Tracking service for %s.", RSA_RPC_SERVICE_NAME);
         status = CELIX_SERVICE_EXCEPTION;
         goto tracker_err;
     }

    *exportOut = export;

    return CELIX_SUCCESS;

tracker_err:
rpc_type_filter_err:
    exportRegistration_releaseEpSvcEntry(export->endpointSvcEntry);
ep_svc_entry_err:
    bundleContext_ungetServiceReference(context, reference);
err_retaining_service_ref:
    endpointDescription_destroy(export->endpointDesc);
    free(export);
    return status;
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
    exportRegistration_releaseEpSvcEntry(export->endpointSvcEntry);
    bundleContext_ungetServiceReference(export->context, export->reference);
    endpointDescription_destroy(export->endpointDesc);
    free(export);
}

static void exportRegistration_destroy(export_registration_t *export) {
    assert(export != NULL);
    celix_bundleContext_stopTrackerAsync(export->context, export->rpcSvcTrkId,
            export, exportRegistration_destroyCallback);
    return ;
}

static export_endpoint_service_entry_t *exportRegistration_createEpSvcEntry(void) {
    celix_status_t status = CELIX_SUCCESS;
    export_endpoint_service_entry_t *endpointSvcEntry =
            (export_endpoint_service_entry_t *)calloc(1, sizeof(*endpointSvcEntry));
    assert(endpointSvcEntry != NULL);
    celix_ref_init(&endpointSvcEntry->ref);
    status = celixThreadMutex_create(&endpointSvcEntry->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        goto mutex_err;
    }
    endpointSvcEntry->endpointSvc = NULL;
    return endpointSvcEntry;
mutex_err:
    free(endpointSvcEntry);
    return NULL;
}

static void exportRegistration_retainEpSvcEntry(export_endpoint_service_entry_t *epSvcEntry) {
    celix_ref_get(&epSvcEntry->ref);
}

static bool exportRegistration_destroyEpSvcEntry(struct celix_ref *ref) {
    export_endpoint_service_entry_t *epSvcEntry = (export_endpoint_service_entry_t *)ref;
    celixThreadMutex_destroy(&epSvcEntry->mutex);
    free(epSvcEntry);
    return true;
}

static void exportRegistration_releaseEpSvcEntry(export_endpoint_service_entry_t *epSvcEntry) {
    (void)celix_ref_put(&epSvcEntry->ref, exportRegistration_destroyEpSvcEntry);
}

static void exportRegistration_addRpcSvc(void *handle, void *svc) {
    assert(handle != NULL);
    celix_status_t status = CELIX_SUCCESS;
    export_registration_t *export = (export_registration_t *)handle;

    if (export->rpcSvc != NULL) {
        celix_logHelper_info(export->logHelper,"RSA export reg: A endpoint supports only one rpc service.");
        return;
    }
    celix_logHelper_info(export->logHelper,"RSA export reg: RSA rpc service add.");
    rsa_rpc_service_t *rpcSvc = (rsa_rpc_service_t *)svc;
    long endpointSvcId = -1;
    status = rpcSvc->installEndpoint(rpcSvc->handle, export->endpointDesc,
            &endpointSvcId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error Installing %s endpoint. %d.",
                export->endpointDesc->service, status);
        goto err_installing_endpoint;
    }

    char filter[32] = {0};// It is longer than the size of "service.id" + endpointSvcId
    (void)snprintf(filter, sizeof(filter), "(%s=%ld)", OSGI_FRAMEWORK_SERVICE_ID,
            endpointSvcId);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = filter;
    opts.filter.serviceName = RSA_RPC_ENDPOINT_SERVICE_NAME;
    opts.filter.versionRange = RSA_RPC_ENDPOINT_SERVICE_USE_RANGE;
    opts.filter.ignoreServiceLanguage = true;
    opts.callbackHandle = export->endpointSvcEntry;
    exportRegistration_retainEpSvcEntry(export->endpointSvcEntry);
    opts.add = exportRegistration_addEndpointSvc;
    opts.remove = exportRegistration_removeEndpointSvc;
    export->endpointSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(export->context, &opts);
    if (export->endpointSvcTrkId < 0) {
        celix_logHelper_error(export->logHelper,"RSA export reg: Error Tracking service for %s(%d)", RSA_RPC_ENDPOINT_SERVICE_NAME, export->endpointSvcId);
        goto err_tracking_endpoint_svc;
    }
    export->endpointSvcId = endpointSvcId;
    export->rpcSvc = (rsa_rpc_service_t *)svc;

    return;
err_tracking_endpoint_svc:
    exportRegistration_releaseEpSvcEntry(export->endpointSvcEntry);
    rpcSvc->uninstallEndpoint(rpcSvc->handle, endpointSvcId);
err_installing_endpoint:
    return;
}

static void exportRegistration_removeRpcSvc(void *handle, void *svc) {
    assert(handle != NULL);
    export_registration_t *export = (export_registration_t *)handle;

    if (export->rpcSvc != svc) {
        celix_logHelper_info(export->logHelper,"RSA import reg: A endponit supports only one rpc service.");
        return;
    }

    celix_logHelper_info(export->logHelper,"RSA import reg: RSA rpc service remove.");

    rsa_rpc_service_t *rpcSvc = (rsa_rpc_service_t *)svc;
    if (rpcSvc != NULL && export->endpointSvcId >= 0) {
        celix_bundleContext_stopTrackerAsync(export->context, export->endpointSvcTrkId,
                export->endpointSvcEntry, (void*)exportRegistration_releaseEpSvcEntry);
        rpcSvc->uninstallEndpoint(rpcSvc->handle, export->endpointSvcId);
        export->endpointSvcId = -1;
        export->rpcSvc = NULL;
    }
    return;
}


static void exportRegistration_addEndpointSvc(void *handle, void *svc) {
    assert(handle != NULL);
    assert(svc != NULL);
    struct export_endpoint_service_entry *endpointSvcEntry =
            (struct export_endpoint_service_entry *)handle;
    celixThreadMutex_lock(&endpointSvcEntry->mutex);
    endpointSvcEntry->endpointSvc = (rsa_rpc_endpoint_service_t *)svc;
    celixThreadMutex_unlock(&endpointSvcEntry->mutex);

    return;
}

static void exportRegistration_removeEndpointSvc(void *handle, void *svc) {
    assert(handle != NULL);
    assert(svc != NULL);
    struct export_endpoint_service_entry *endpointSvcEntry =
            (struct export_endpoint_service_entry *)handle;
    celixThreadMutex_lock(&endpointSvcEntry->mutex);
    if (svc == endpointSvcEntry->endpointSvc) {
        endpointSvcEntry->endpointSvc = NULL;
    }
    celixThreadMutex_unlock(&endpointSvcEntry->mutex);
    return;
}

celix_status_t exportRegistration_call(export_registration_t *export, celix_properties_t **metadata,
        const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (export == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    struct export_endpoint_service_entry *endpointSvcEntry = export->endpointSvcEntry;
    assert(endpointSvcEntry != NULL);
    celixThreadMutex_lock(&endpointSvcEntry->mutex);
    if (endpointSvcEntry->endpointSvc != NULL) {
        // XXX The handleRequest function is blocked, how can we move it to outside of critical section.
        status =  endpointSvcEntry->endpointSvc->handleRequest(endpointSvcEntry->endpointSvc->handle, metadata, request, response);
    } else {
        status = CELIX_ILLEGAL_STATE;
        celix_logHelper_error(export->logHelper, "RSA export reg: Error Handling request. Please ensure rsa rpc servie is active.");
    }
    celixThreadMutex_unlock(&endpointSvcEntry->mutex);

    return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_t *registration,
        export_reference_t **out) {
    if (registration == NULL || out == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    export_reference_t *ref = calloc(1, sizeof(*ref));
    assert(ref != NULL);

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

celix_status_t exportRegistration_getException(export_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}
