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

#include "rsa_json_rpc_endpoint_impl.h"
#include "remote_interceptors_handler.h"
#include "endpoint_description.h"
#include "dfi_utils.h"
#include "json_rpc.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include "celix_constants.h"
#include <sys/uio.h>
#include <jansson.h>
#include <assert.h>
#include <string.h>

struct rsa_json_rpc_endpoint {
    celix_bundle_context_t* ctx;
    celix_log_helper_t *logHelper;
    FILE *callsLogFile;
    endpoint_description_t *endpointDesc;
    unsigned int serialProtoId;
    remote_interceptors_handler_t *interceptorsHandler;
    long svcTrackerId;
    celix_thread_rwlock_t lock; //projects below
    void *service;
    dyn_interface_type *intfType;
};

static void rsaJsonRpcEndpoint_stopSvcTrackerDone(void *data);
static void rsaJsonRpcEndpoint_addSvcWithOwner(void *handle, void *service,
        const celix_properties_t *props, const celix_bundle_t *svcOwner);
static void rsaJsonRpcEndpoint_removeSvcWithOwner(void *handle, void *service,
        const celix_properties_t *props, const celix_bundle_t *svcOwner);

celix_status_t rsaJsonRpcEndpoint_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper,
        FILE *logFile, remote_interceptors_handler_t *interceptorsHandler,
        const endpoint_description_t *endpointDesc, unsigned int serialProtoId,
        rsa_json_rpc_endpoint_t **endpointOut) {
    assert(ctx != NULL);
    assert(logHelper != NULL);
    assert(interceptorsHandler != NULL);
    assert(endpointDesc != NULL);
    assert(endpointOut != NULL);
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree rsa_json_rpc_endpoint_t* endpoint = calloc(1, sizeof(*endpoint));
    if (endpoint == NULL) {
        return CELIX_ENOMEM;
    }
    endpoint->ctx = ctx;
    endpoint->logHelper = logHelper;
    endpoint->callsLogFile = logFile;
    endpoint->serialProtoId = serialProtoId;
    celix_autoptr(endpoint_description_t) endpointDescCopy = endpoint->endpointDesc = endpointDescription_clone(endpointDesc);
    if (endpoint->endpointDesc == NULL) {
        celix_logHelper_error(logHelper, "RSA json rpc endpoint: Error cloning endpoint description for %s.",
                endpointDesc->serviceName);
        return CELIX_ENOMEM;
    }

    endpoint->interceptorsHandler = interceptorsHandler;
    endpoint->service = NULL;
    endpoint->intfType = NULL;
    status = celixThreadRwlock_create(&endpoint->lock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "RSA json rpc endpoint: Error initilizing lock for %s. %d.",
                endpointDesc->serviceName, status);
        return status;
    }
    celix_autoptr(celix_thread_rwlock_t) lock = &endpoint->lock;

    char filter[32] = {0};// It is longer than the size of "service.id" + serviceId
    (void)snprintf(filter, sizeof(filter), "(%s=%ld)", CELIX_FRAMEWORK_SERVICE_ID, endpointDesc->serviceId);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = filter;
    opts.callbackHandle = endpoint;
    opts.addWithOwner = rsaJsonRpcEndpoint_addSvcWithOwner;
    opts.removeWithOwner = rsaJsonRpcEndpoint_removeSvcWithOwner;
    endpoint->svcTrackerId = celix_bundleContext_trackServicesWithOptions(endpoint->ctx, &opts);//sync call, as we need the service is tracked before returning
    if (endpoint->svcTrackerId < 0) {
        celix_logHelper_error(logHelper, "RSA json rpc endpoint: Error Registering %s tracker.", endpointDesc->serviceName);
        return CELIX_ILLEGAL_STATE;
    }

    celix_steal_ptr(lock);
    celix_steal_ptr(endpointDescCopy);
    *endpointOut = celix_steal_ptr(endpoint);
    return CELIX_SUCCESS;
}

static void rsaJsonRpcEndpoint_stopSvcTrackerDone(void *data) {
    assert(data != NULL);
    rsa_json_rpc_endpoint_t *endpoint = (rsa_json_rpc_endpoint_t *)data;
    (void)celixThreadRwlock_destroy(&endpoint->lock);
    endpointDescription_destroy(endpoint->endpointDesc);
    free(endpoint);
    return;
}

void rsaJsonRpcEndpoint_destroy(rsa_json_rpc_endpoint_t *endpoint) {
    if (endpoint != NULL) {
        celix_bundleContext_stopTrackerAsync(endpoint->ctx, endpoint->svcTrackerId, endpoint, rsaJsonRpcEndpoint_stopSvcTrackerDone);
    }
    return;
}

long rsaJsonRpcEndpoint_getId(rsa_json_rpc_endpoint_t *endpoint) {
    return endpoint->svcTrackerId;
}

static void rsaJsonRpcEndpoint_addSvcWithOwner(void *handle, void *service,
        const celix_properties_t *props, const celix_bundle_t *svcOwner) {
    assert(handle != NULL);
    assert(service != NULL);
    assert(props != NULL);
    assert(svcOwner != NULL);
    celix_status_t status = CELIX_SUCCESS;
    rsa_json_rpc_endpoint_t *endpoint = (rsa_json_rpc_endpoint_t *)handle;
    celix_autoptr(dyn_interface_type) intfType = NULL;
    const char *serviceName = celix_properties_get(endpoint->endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_NAME, "unknown-service");

    celix_auto(celix_rwlock_wlock_guard_t) lock = celixRwlockWlockGuard_init(&endpoint->lock);

    status = dfi_findAndParseInterfaceDescriptor(endpoint->logHelper,endpoint->ctx,
            svcOwner, endpoint->endpointDesc->serviceName, &intfType);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(endpoint->logHelper, "Endpoint: Error Parsing service descriptor for %s.", serviceName);
        return;
    }

    // Check version
    const char* intfVersion = dynInterface_getVersionString(intfType);
    const char *serviceVersion = celix_properties_get(endpoint->endpointDesc->properties,CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
    if (serviceVersion == NULL) {
        celix_logHelper_error(endpoint->logHelper, "Endpoint: Error getting service version for %s.", serviceName);
        return;
    }
    if(strcmp(serviceVersion, intfVersion)!=0){
        celix_logHelper_error(endpoint->logHelper, "Endpoint: %s version (%s) and interface version from the descriptor (%s) are not the same!", serviceName, serviceVersion,intfVersion);
        return;
    }

    endpoint->service = service;
    endpoint->intfType = celix_steal_ptr(intfType);
    return;
}

static void rsaJsonRpcEndpoint_removeSvcWithOwner(void *handle, void *service,
        const celix_properties_t *props, const celix_bundle_t *svcOwner) {
    assert(handle != NULL);
    (void)props;
    (void)svcOwner;
    rsa_json_rpc_endpoint_t *endpoint = (rsa_json_rpc_endpoint_t *)handle;
    celix_auto(celix_rwlock_wlock_guard_t) lock = celixRwlockWlockGuard_init(&endpoint->lock);
    if (endpoint->service == service) {
        endpoint->service = NULL;
        dynInterface_destroy(endpoint->intfType);
        endpoint->intfType = NULL;
    }
    return;
}

celix_status_t rsaJsonRpcEndpoint_handleRequest(rsa_json_rpc_endpoint_t *endpoint, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *responseOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (endpoint == NULL || request == NULL || request->iov_base == NULL
            || request->iov_len == 0 || responseOut == NULL || metadata == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    responseOut->iov_base = NULL;
    responseOut->iov_len = 0;

    long serialProtoId  = celix_properties_getAsLong(metadata, "SerialProtocolId", 0);
    if (serialProtoId != endpoint->serialProtoId) {
        celix_logHelper_error(endpoint->logHelper, "Serialization protocol ID mismatch. expect:%ld actual:%u.", serialProtoId, endpoint->serialProtoId);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    json_error_t error;
    json_auto_t* jsRequest = json_loads((char *)request->iov_base, 0, &error);
    if (jsRequest == NULL) {
        celix_logHelper_error(endpoint->logHelper, "Parse request json string failed for %s.", (char *)request->iov_base);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char *sig;
    int rc = json_unpack(jsRequest, "{s:s}", "m", &sig);
    if (rc != 0) {
        celix_logHelper_error(endpoint->logHelper, "Error requesting method for %s.", (char *)request->iov_base);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char *szResponse = NULL;
    bool cont = remoteInterceptorHandler_invokePreExportCall(endpoint->interceptorsHandler,
            endpoint->endpointDesc->properties, sig, &metadata);
    if (cont) {
        celixThreadRwlock_readLock(&endpoint->lock);
        if (endpoint->service != NULL) {
            int rc1 = jsonRpc_call(endpoint->intfType, endpoint->service, (char *)request->iov_base, &szResponse);
            status = (rc1 != 0) ? CELIX_SERVICE_EXCEPTION : CELIX_SUCCESS;
            if (rc1 != 0) {
                celix_logHelper_logTssErrors(endpoint->logHelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(endpoint->logHelper, "Error calling remote service. Got error code %d", rc1);
            }
        } else {
            status = CELIX_ILLEGAL_STATE;
            celix_logHelper_error(endpoint->logHelper, "%s is null, please try again.", endpoint->endpointDesc->serviceName);
        }
        celixThreadRwlock_unlock(&endpoint->lock);

        remoteInterceptorHandler_invokePostExportCall(endpoint->interceptorsHandler,
                endpoint->endpointDesc->properties, sig, metadata);
    } else {
        celix_logHelper_error(endpoint->logHelper, "%s has been intercepted.", endpoint->endpointDesc->serviceName);
        status = CELIX_INTERCEPTOR_EXCEPTION;
    }

    if (szResponse != NULL) {
        responseOut->iov_base = szResponse;
        responseOut->iov_len = strlen(szResponse) + 1;// make it include '\0'
    }

    if (endpoint->callsLogFile != NULL) {
        fprintf(endpoint->callsLogFile, "ENDPOINT REMOTE CALL:\n\tservice=%s\n\tservice_id=%lu\n\trequest_payload=%s\n\trequest_response=%s\n\tstatus=%i\n",
                endpoint->endpointDesc->serviceName, endpoint->endpointDesc->serviceId, (char *)request->iov_base, szResponse, status);
        fflush(endpoint->callsLogFile);
    }

    return status;
}