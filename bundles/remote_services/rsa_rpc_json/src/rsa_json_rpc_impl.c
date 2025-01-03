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

#include "rsa_json_rpc_impl.h"
#include "rsa_json_rpc_constants.h"
#include "rsa_json_rpc_endpoint_impl.h"
#include "rsa_json_rpc_proxy_impl.h"
#include "remote_interceptors_handler.h"
#include "endpoint_description.h"
#include "celix_long_hash_map.h"
#include "celix_log_helper.h"
#include "dyn_interface.h"
#include "celix_stdlib_cleanup.h"
#include "celix_version.h"
#include "celix_threads.h"
#include "celix_constants.h"
#include "celix_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

struct rsa_json_rpc {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    celix_thread_mutex_t mutex; //It protects svcProxyFactories and svcEndpoints
    celix_long_hash_map_t *svcProxyFactories;// Key: proxy factory service id, Value: rsa_json_rpc_proxy_factory_t
    celix_long_hash_map_t *svcEndpoints;// Key:request handler service id, Value: rsa_json_rpc_endpoint_t
    remote_interceptors_handler_t *interceptorsHandler;
    rsa_request_sender_tracker_t *reqSenderTracker;
    unsigned int serialProtoId; //Serialization protocol ID
    FILE *callsLogFile;
};

static unsigned int rsaJsonRpc_generateSerialProtoId(celix_bundle_t *bnd) {
    const char *bundleSymName = celix_bundle_getSymbolicName(bnd);
    const celix_version_t* bundleVer = celix_bundle_getVersion(bnd);
    if (bundleSymName == NULL || bundleVer == NULL) {
        return 0;
    }
    int major = celix_version_getMajor(bundleVer);
    return celix_utils_stringHash(bundleSymName) + major;
}

celix_status_t rsaJsonRpc_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper,
        rsa_json_rpc_t **jsonRpcOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (ctx == NULL || logHelper == NULL || jsonRpcOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree rsa_json_rpc_t *rpc = calloc(1, sizeof(rsa_json_rpc_t));
    if (rpc == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for rsa_json_rpc_t.");
        return CELIX_ENOMEM;
    }
    rpc->ctx = ctx;
    rpc->logHelper = logHelper;
    rpc->serialProtoId = rsaJsonRpc_generateSerialProtoId(celix_bundleContext_getBundle(ctx));
    if (rpc->serialProtoId == 0) {
        celix_logHelper_error(logHelper, "Error generating serialization protocol id.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    status = celixThreadMutex_create(&rpc->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating endpoint mutex. %d.", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &rpc->mutex;
    celix_autoptr(celix_long_hash_map_t) svcProxyFactories = rpc->svcProxyFactories = celix_longHashMap_create();
    assert(rpc->svcProxyFactories != NULL);
    celix_autoptr(celix_long_hash_map_t) svcEndpoints = rpc->svcEndpoints = celix_longHashMap_create();
    assert(rpc->svcEndpoints != NULL);

    status = remoteInterceptorsHandler_create(ctx, &rpc->interceptorsHandler);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating remote interceptors handler. %d.", status);
        return status;
    }
    celix_autoptr(remote_interceptors_handler_t) interceptorsHandler = rpc->interceptorsHandler;

    status = rsaRequestSenderTracker_create(ctx, logHelper, &rpc->reqSenderTracker);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    bool logCalls = celix_bundleContext_getPropertyAsBool(ctx, RSA_JSON_RPC_LOG_CALLS_KEY, RSA_JSON_RPC_LOG_CALLS_DEFAULT);
    if (logCalls) {
        const char *f = celix_bundleContext_getProperty(ctx, RSA_JSON_RPC_LOG_CALLS_FILE_KEY, RSA_JSON_RPC_LOG_CALLS_FILE_DEFAULT);
        if (strncmp(f, "stdout", strlen("stdout")) == 0) {
            rpc->callsLogFile = stdout;
        } else {
            rpc->callsLogFile = fopen(f, "w");
            if (rpc->callsLogFile == NULL) {
                celix_logHelper_warning(logHelper, "Error opening file '%s' for logging calls. %d", f, errno);
            }
        }
    }
    celix_steal_ptr(interceptorsHandler);
    celix_steal_ptr(svcEndpoints);
    celix_steal_ptr(svcProxyFactories);
    celix_steal_ptr(mutex);
    *jsonRpcOut = celix_steal_ptr(rpc);
    return CELIX_SUCCESS;
}

void rsaJsonRpc_destroy(rsa_json_rpc_t *jsonRpc) {
    if (jsonRpc != NULL) {
        if (jsonRpc->callsLogFile != NULL && jsonRpc->callsLogFile != stdout) {
            fclose(jsonRpc->callsLogFile);
        }
        rsaRequestSenderTracker_destroy(jsonRpc->reqSenderTracker);
        remoteInterceptorsHandler_destroy(jsonRpc->interceptorsHandler);
        assert(celix_longHashMap_size(jsonRpc->svcEndpoints) == 0);
        celix_longHashMap_destroy(jsonRpc->svcEndpoints);
        assert(celix_longHashMap_size(jsonRpc->svcProxyFactories) == 0);
        celix_longHashMap_destroy(jsonRpc->svcProxyFactories);
        (void)celixThreadMutex_destroy(&jsonRpc->mutex);
        free(jsonRpc);
    }
    return;
}

celix_status_t rsaJsonRpc_createProxy(void *handle, const endpoint_description_t *endpointDesc,
        long requestSenderSvcId, long *proxySvcId) {
    celix_status_t status= CELIX_SUCCESS;

    if (handle == NULL || endpointDescription_isInvalid(endpointDesc)
            || requestSenderSvcId < 0  || proxySvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    rsa_json_rpc_proxy_factory_t *proxyFactory = NULL;
    status = rsaJsonRpcProxy_factoryCreate(jsonRpc->ctx, jsonRpc->logHelper,
            jsonRpc->callsLogFile, jsonRpc->interceptorsHandler, endpointDesc,
            jsonRpc->reqSenderTracker, requestSenderSvcId, jsonRpc->serialProtoId, &proxyFactory);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(jsonRpc->logHelper, "Error creating proxy factory for %s.", endpointDesc->serviceName);
        return status;
    }
    long factorySvcId = rsaJsonRpcProxy_factorySvcId(proxyFactory);

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcProxyFactories, factorySvcId, proxyFactory);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *proxySvcId = factorySvcId;

    return CELIX_SUCCESS;
}

void rsaJsonRpc_destroyProxy(void *handle, long proxySvcId) {
    if (handle == NULL  || proxySvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);
    rsa_json_rpc_proxy_factory_t *proxyFactory =
            celix_longHashMap_get(jsonRpc->svcProxyFactories, proxySvcId);
    if (proxyFactory != NULL) {
        (void)celix_longHashMap_remove(jsonRpc->svcProxyFactories, proxySvcId);
        rsaJsonRpcProxy_factoryDestroy(proxyFactory);
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

celix_status_t rsaJsonRpc_createEndpoint(void *handle, const endpoint_description_t *endpointDesc,
        long *requestHandlerSvcId) {
    celix_status_t status= CELIX_SUCCESS;
    if (handle == NULL || endpointDescription_isInvalid(endpointDesc) || requestHandlerSvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    rsa_json_rpc_endpoint_t *endpoint = NULL;
    status = rsaJsonRpcEndpoint_create(jsonRpc->ctx, jsonRpc->logHelper, jsonRpc->callsLogFile,
            jsonRpc->interceptorsHandler, endpointDesc, jsonRpc->serialProtoId, &endpoint);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    long reqHandlerSvcId = rsaJsonRpcEndpoint_getRequestHandlerSvcId(endpoint);
    assert(reqHandlerSvcId >= 0);

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcEndpoints, reqHandlerSvcId, endpoint);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *requestHandlerSvcId = reqHandlerSvcId;

    return CELIX_SUCCESS;
}

void rsaJsonRpc_destroyEndpoint(void *handle, long requestHandlerSvcId) {
    if (handle == NULL  || requestHandlerSvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);

    rsa_json_rpc_endpoint_t *endpoint =
            celix_longHashMap_get(jsonRpc->svcEndpoints, requestHandlerSvcId);
    if (endpoint != NULL) {
        (void)celix_longHashMap_remove(jsonRpc->svcEndpoints, requestHandlerSvcId);
        rsaJsonRpcEndpoint_destroy(endpoint);
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

