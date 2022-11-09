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

#include <rsa_json_rpc_impl.h>
#include <rsa_json_rpc_constants.h>
#include <rsa_json_rpc_endpoint_impl.h>
#include <rsa_json_rpc_proxy_impl.h>
#include <remote_interceptors_handler.h>
#include <endpoint_description.h>
#include <celix_long_hash_map.h>
#include <celix_log_helper.h>
#include <dyn_interface.h>
#include <version.h>
#include <celix_api.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

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
    const char *bundleVer = celix_bundle_getManifestValue(bnd, OSGI_FRAMEWORK_BUNDLE_VERSION);
    if (bundleSymName == NULL || bundleVer == NULL) {
        return 0;
    }
    version_pt version = NULL;
    celix_status_t status = version_createVersionFromString(bundleVer, &version);
    if (status != CELIX_SUCCESS) {
        return 0;
    }
    int major = 0;
    (void)version_getMajor(version, &major);
    celix_version_destroy(version);
    return celix_utils_stringHash(bundleSymName) + major;
}

celix_status_t rsaJsonRpc_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper,
        rsa_json_rpc_t **jsonRpcOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (ctx == NULL || logHelper == NULL || jsonRpcOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *rpc = calloc(1, sizeof(rsa_json_rpc_t));
    assert(rpc != NULL);
    rpc->ctx = ctx;
    rpc->logHelper = logHelper;
    rpc->serialProtoId = rsaJsonRpc_generateSerialProtoId(celix_bundleContext_getBundle(ctx));
    if (rpc->serialProtoId == 0) {
        celix_logHelper_error(logHelper, "Error generating serialization protocol id.");
        goto protocol_id_err;
    }
    status = celixThreadMutex_create(&rpc->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating endpoint mutex. %d.", status);
        goto mutex_err;
    }
    rpc->svcProxyFactories = celix_longHashMap_create();
    assert(rpc->svcProxyFactories != NULL);
    rpc->svcEndpoints = celix_longHashMap_create();
    assert(rpc->svcEndpoints != NULL);

    status = remoteInterceptorsHandler_create(ctx, &rpc->interceptorsHandler);
    if (status != CELIX_SUCCESS) {
        goto interceptors_err;
    }

    status = rsaRequestSenderTracker_create(ctx, "rsa_json_rpc_rst", &rpc->reqSenderTracker);
    if (status != CELIX_SUCCESS) {
        goto rst_err;
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
    *jsonRpcOut = rpc;
    return CELIX_SUCCESS;
rst_err:
    remoteInterceptorsHandler_destroy(rpc->interceptorsHandler);
interceptors_err:
    celix_longHashMap_destroy(rpc->svcEndpoints);
    celix_longHashMap_destroy(rpc->svcProxyFactories);
    (void)celixThreadMutex_destroy(&rpc->mutex);
mutex_err:
protocol_id_err:
    free(rpc);
    return status;
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
        celix_logHelper_error(jsonRpc->logHelper, "Error creating proxy factory for %s.", endpointDesc->service);
        goto err_creating_proxy_fac;
    }
    long factorySvcId = rsaJsonRpcProxy_factorySvcId(proxyFactory);

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcProxyFactories, factorySvcId, proxyFactory);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *proxySvcId = factorySvcId;

    return CELIX_SUCCESS;

err_creating_proxy_fac:
    return status;
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
        goto endpoint_err;
    }
    long reqHandlerSvcId = rsaJsonRpcEndpoint_getRequestHandlerSvcId(endpoint);
    assert(reqHandlerSvcId >= 0);

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcEndpoints, reqHandlerSvcId, endpoint);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *requestHandlerSvcId = reqHandlerSvcId;

    return CELIX_SUCCESS;

endpoint_err:
    return status;
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

