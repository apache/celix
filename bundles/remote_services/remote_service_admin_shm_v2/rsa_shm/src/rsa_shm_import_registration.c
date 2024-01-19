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
#include "rsa_shm_constants.h"
#include "rsa_rpc_factory.h"
#include "remote_constants.h"
#include "celix_log_helper.h"
#include "celix_api.h"
#include "celix_stdlib_cleanup.h"
#include <string.h>
#include <assert.h>

struct import_registration {
    celix_bundle_context_t *context;
    celix_log_helper_t *logHelper;
    endpoint_description_t *endpointDesc;
    long reqSenderSvcId;
    long rpcSvcTrkId;
    rsa_rpc_factory_t *rpcFac;
    long proxySvcId;
};

static void importRegistration_addRpcFac(void *handle, void *svc);
static void importRegistration_removeRpcFac(void *handle, void *svc);

celix_status_t importRegistration_create(celix_bundle_context_t *context,
        celix_log_helper_t *logHelper, endpoint_description_t *endpointDesc,
        long reqSenderSvcId, import_registration_t **importOut) {
    if (context == NULL || logHelper == NULL || endpointDescription_isInvalid(endpointDesc)
            || reqSenderSvcId < 0 || importOut == NULL) {
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

    import->reqSenderSvcId = reqSenderSvcId;
    import->rpcFac = NULL;
    import->proxySvcId = -1;

    const char *rsaShmRpcType = celix_properties_get(endpointDesc->properties, RSA_SHM_RPC_TYPE_KEY, NULL);
    if (rsaShmRpcType == NULL) {
        celix_logHelper_error(logHelper,"RSA import reg: %s property is not exist.", RSA_SHM_RPC_TYPE_KEY);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char filter[128] = {0};
    int bytes = snprintf(filter, sizeof(filter), "(%s=%s)", CELIX_RSA_RPC_TYPE_KEY, rsaShmRpcType);
    if (bytes >= sizeof(filter)) {
        celix_logHelper_error(logHelper, "RSA import reg: The value(%s) of %s is too long.", rsaShmRpcType, CELIX_RSA_RPC_TYPE_KEY);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = filter;
    opts.filter.serviceName = CELIX_RSA_RPC_FACTORY_NAME;
    opts.filter.versionRange = CELIX_RSA_RPC_FACTORY_USE_RANGE;
    opts.callbackHandle = import;
    opts.add = importRegistration_addRpcFac;
    opts.remove = importRegistration_removeRpcFac;
    import->rpcSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(context, &opts);
    if (import->rpcSvcTrkId < 0) {
        celix_logHelper_error(logHelper, "RSA import reg: Error Tracking service for %s.", CELIX_RSA_RPC_FACTORY_NAME);
        return CELIX_SERVICE_EXCEPTION;
    }

    celix_steal_ptr(epDesc);
    *importOut = celix_steal_ptr(import);

    return CELIX_SUCCESS;
}

static void importRegistration_stopRpcSvcTrkDone(void *data) {
    assert(data != NULL);
    import_registration_t *import = (import_registration_t *)data;
    endpointDescription_destroy(import->endpointDesc);
    free(import);
}

void importRegistration_destroy(import_registration_t *import) {
    if (import != NULL) {
        celix_bundleContext_stopTrackerAsync(import->context, import->rpcSvcTrkId,
                import, importRegistration_stopRpcSvcTrkDone);
    }
    return;
}


static void importRegistration_addRpcFac(void *handle, void *svc) {
    assert(handle != NULL);
    celix_status_t status = CELIX_SUCCESS;
    import_registration_t *import = (import_registration_t *)handle;

    if (import->rpcFac != NULL) {
        celix_logHelper_info(import->logHelper,"RSA import reg: A proxy supports only one rpc service.");
        return;
    }
    celix_logHelper_info(import->logHelper,"RSA export reg: RSA rpc service add.");
    rsa_rpc_factory_t *rpcFac = (rsa_rpc_factory_t *)svc;
    long proxySvcId = -1;
    status = rpcFac->createProxy(rpcFac->handle, import->endpointDesc,
            import->reqSenderSvcId, &proxySvcId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(import->logHelper,"RSA import reg: Error Installing %s proxy. %d.",
                import->endpointDesc->serviceName, status);
        return;
    }
    import->proxySvcId = proxySvcId;
    import->rpcFac = (rsa_rpc_factory_t *)svc;

    return;
}

static void importRegistration_removeRpcFac(void *handle, void *svc) {
    assert(handle != NULL);
    import_registration_t *import = (import_registration_t *)handle;

    if (import->rpcFac != svc) {
        celix_logHelper_info(import->logHelper,"RSA import reg: A endponit supports only one rpc service.");
        return;
    }

    celix_logHelper_info(import->logHelper,"RSA import reg: RSA rpc service remove.");

    rsa_rpc_factory_t *rpcFac = (rsa_rpc_factory_t *)svc;
    if (rpcFac != NULL && import->proxySvcId >= 0) {
        rpcFac->destroyProxy(rpcFac->handle, import->proxySvcId);
        import->rpcFac = NULL;
        import->proxySvcId = -1;
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
