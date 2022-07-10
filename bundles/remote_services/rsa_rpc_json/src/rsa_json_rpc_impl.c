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
#include <rsa_rpc_endpoint_service.h>
#include <rsa_rpc_request_sender.h>
#include <remote_interceptors_handler.h>
#include <endpoint_description.h>
#include <remote_constants.h>
#include <celix_log_helper.h>
#include <dyn_interface.h>
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
    celix_array_list_t *svcProxyFactories;
    celix_array_list_t *svcEndpoints;
    remote_interceptors_handler_t *interceptorsHandler;
    FILE *callsLogFile;
};

struct rsa_json_rpc_proxy_entry {
    int useCnt;
    rsa_json_rpc_proxy_t *proxy;
};

struct rsa_json_rpc_proxy_fac_entry {
    rsa_json_rpc_t *jsonRpc;
    celix_service_factory_t factory;
    long factorySvcId;
    endpoint_description_t *endpointDesc;
    hash_map_t *proxies;//key is requestingBundle, value is rsa_json_rpc_proxy_entry.
    rsa_rpc_request_sender_t *requestSender;
};

struct rsa_json_rpc_endpoint_entry {
    rsa_json_rpc_endpoint_t *endpoint;
    rsa_rpc_endpoint_service_t svc;
    long svcId;
};

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
    status = celixThreadMutex_create(&rpc->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating endpoint mutex. %d.", status);
        goto mutex_err;
    }
    rpc->svcProxyFactories = celix_arrayList_create();
    assert(rpc->svcProxyFactories != NULL);
    rpc->svcEndpoints = celix_arrayList_create();
    assert(rpc->svcEndpoints != NULL);

    status = remoteInterceptorsHandler_create(ctx, &rpc->interceptorsHandler);
    if (status != CELIX_SUCCESS) {
        goto interceptors_err;
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
interceptors_err:
    celix_arrayList_destroy(rpc->svcEndpoints);
    celix_arrayList_destroy(rpc->svcProxyFactories);
    (void)celixThreadMutex_destroy(&rpc->mutex);
mutex_err:
    free(rpc);
    return status;
}

void rsaJsonRpc_destroy(rsa_json_rpc_t *jsonRpc) {
    if (jsonRpc == NULL) {
        return;
    }
    if (jsonRpc->callsLogFile != NULL && jsonRpc->callsLogFile != stdout) {
        fclose(jsonRpc->callsLogFile);
    }
    remoteInterceptorsHandler_destroy(jsonRpc->interceptorsHandler);
    assert(celix_arrayList_size(jsonRpc->svcEndpoints) == 0);
    celix_arrayList_destroy(jsonRpc->svcEndpoints);
    assert(celix_arrayList_size(jsonRpc->svcProxyFactories) == 0);
    celix_arrayList_destroy(jsonRpc->svcProxyFactories);
    (void)celixThreadMutex_destroy(&jsonRpc->mutex);
    free(jsonRpc);
    return;
}

static void* rsaJsonRpc_getProxyService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties) {
    assert(handle != NULL);
    assert(requestingBundle != NULL);
    assert(svcProperties != NULL);
    celix_status_t status = CELIX_SUCCESS;
    void *svc = NULL;
    struct rsa_json_rpc_proxy_fac_entry *proxyFacEntry = (struct rsa_json_rpc_proxy_fac_entry *)handle;

    struct rsa_json_rpc_proxy_entry *proxyEntry = hashMap_get(proxyFacEntry->proxies, requestingBundle);
    if (proxyEntry == NULL) {
        proxyEntry = calloc(1, sizeof(*proxyEntry));
        assert(proxyEntry != NULL);
        proxyEntry->useCnt = 0;
        struct rsa_json_rpc_proxy_init_param initParam = {
                .ctx = proxyFacEntry->jsonRpc->ctx,
                .logHelper = proxyFacEntry->jsonRpc->logHelper,
                .logFile = proxyFacEntry->jsonRpc->callsLogFile,
                .interceptorsHandler = proxyFacEntry->jsonRpc->interceptorsHandler,
                .requestingBundle = requestingBundle,
                .endpointDesc = proxyFacEntry->endpointDesc,
                .requestSender = proxyFacEntry->requestSender,
        };
        status = rsaJsonRpcProxy_create(&initParam, &proxyEntry->proxy);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(proxyFacEntry->jsonRpc->logHelper,"Error Creating service proxy for %s. %d",
                    proxyFacEntry->endpointDesc->service, status);
            goto service_proxy_err;
        }
        hashMap_put(proxyFacEntry->proxies, (void*)requestingBundle, proxyEntry);
    }
    proxyEntry->useCnt += 1;
    svc = rsaJsonRpcProxy_getService(proxyEntry->proxy);

    return svc;

service_proxy_err:
    free(proxyEntry);
    return NULL;
}

static void rsaJsonRpc_ungetProxyService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties) {
    assert(handle != NULL);
    assert(requestingBundle != NULL);
    assert(svcProperties != NULL);
    struct rsa_json_rpc_proxy_fac_entry *proxyFacEntry = (struct rsa_json_rpc_proxy_fac_entry *)handle;
    struct rsa_json_rpc_proxy_entry *proxyEntry = hashMap_get(proxyFacEntry->proxies, requestingBundle);
    if (proxyEntry != NULL) {
        proxyEntry->useCnt -= 1;
        if (proxyEntry->useCnt == 0) {
            (void)hashMap_remove(proxyFacEntry->proxies, requestingBundle);
            rsaJsonRpcProxy_destroy(proxyEntry->proxy);
            free(proxyEntry);
        }
    }
    return;
}

celix_status_t rsaJsonRpc_installProxy(void *handle, const endpoint_description_t *endpointDesc,
        rsa_rpc_request_sender_t *requestSender, long *proxySvcId) {
    celix_status_t status= CELIX_SUCCESS;

    if (handle == NULL || endpointDescription_isInvalid(endpointDesc)
            || requestSender == NULL || proxySvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    struct rsa_json_rpc_proxy_fac_entry *proxyFacEntry = (struct rsa_json_rpc_proxy_fac_entry*)malloc(sizeof(*proxyFacEntry));
    assert(proxyFacEntry != NULL);
    proxyFacEntry->proxies = hashMap_create(NULL, NULL, NULL, NULL);
    assert(proxyFacEntry->proxies != NULL);
    //copy endpoint description
    proxyFacEntry->endpointDesc = (endpoint_description_t*)calloc(1, sizeof(endpoint_description_t));
    assert(proxyFacEntry->endpointDesc != NULL);
    proxyFacEntry->endpointDesc->properties = celix_properties_copy(endpointDesc->properties);
    assert(proxyFacEntry->endpointDesc->properties != NULL);
    proxyFacEntry->endpointDesc->frameworkUUID = (char*)celix_properties_get(proxyFacEntry->endpointDesc->properties,
            OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    proxyFacEntry->endpointDesc->serviceId = endpointDesc->serviceId;
    proxyFacEntry->endpointDesc->id = (char*)celix_properties_get(proxyFacEntry->endpointDesc->properties,
            OSGI_RSA_ENDPOINT_ID, NULL);
    proxyFacEntry->endpointDesc->service = strdup(endpointDesc->service);
    assert(proxyFacEntry->endpointDesc->service != NULL);

    proxyFacEntry->requestSender = requestSender;
    rsaRpcRequestSender_addRef(proxyFacEntry->requestSender);
    proxyFacEntry->jsonRpc = jsonRpc;
    proxyFacEntry->factory.handle = proxyFacEntry;
    proxyFacEntry->factory.getService = rsaJsonRpc_getProxyService;
    proxyFacEntry->factory.ungetService = rsaJsonRpc_ungetProxyService;
    celix_properties_t *props =  celix_properties_copy(endpointDesc->properties);
    assert(props != NULL);
    proxyFacEntry->factorySvcId = celix_bundleContext_registerServiceFactoryAsync(
            jsonRpc->ctx, &proxyFacEntry->factory, endpointDesc->service, props);
    if (proxyFacEntry->factorySvcId  < 0) {
        celix_logHelper_error(jsonRpc->logHelper, "Register proxy service failed");
        status = CELIX_ILLEGAL_STATE;
        goto register_proxy_svc_fac_err;
    }
    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_arrayList_add(jsonRpc->svcProxyFactories,proxyFacEntry);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *proxySvcId = proxyFacEntry->factorySvcId;

    return CELIX_SUCCESS;

register_proxy_svc_fac_err:
    // props is freed by framework
    (void)rsaRpcRequestSender_release(proxyFacEntry->requestSender);
    endpointDescription_destroy(proxyFacEntry->endpointDesc);
    hashMap_destroy(proxyFacEntry->proxies, false, false);
    free(proxyFacEntry);
    return status;
}

static void rsaJsonRpc_unregisterProxySvcDone(void *data) {
    assert(data != NULL);
    struct rsa_json_rpc_proxy_fac_entry *proxyFacEntry = (struct rsa_json_rpc_proxy_fac_entry *)data;
    (void)rsaRpcRequestSender_release(proxyFacEntry->requestSender);
    endpointDescription_destroy(proxyFacEntry->endpointDesc);
    hashMap_destroy(proxyFacEntry->proxies, false, false);
    free(proxyFacEntry);
    return;
}

void rsaJsonRpc_uninstallProxy(void *handle, long proxySvcId) {
    if (handle == NULL  || proxySvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);
    size_t proxyFacNum = celix_arrayList_size(jsonRpc->svcProxyFactories);
    for (int i = 0; i < proxyFacNum; ++i) {
        struct rsa_json_rpc_proxy_fac_entry *proxyFacEntry =
                celix_arrayList_get(jsonRpc->svcProxyFactories, i);
        if (proxyFacEntry->factorySvcId == proxySvcId) {
            celix_arrayList_remove(jsonRpc->svcProxyFactories, proxyFacEntry);
            celix_bundleContext_unregisterServiceAsync(jsonRpc->ctx, proxySvcId,
                    proxyFacEntry, rsaJsonRpc_unregisterProxySvcDone);
            break;
        }
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

celix_status_t rsaJsonRpc_installEndpoint(void *handle, const endpoint_description_t *endpointDesc,
        long *endpointSvcId) {
    celix_status_t status= CELIX_SUCCESS;
    if (handle == NULL || endpointDescription_isInvalid(endpointDesc) || endpointSvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    struct rsa_json_rpc_endpoint_entry *endpointEntry = calloc(1, sizeof(*endpointEntry));
    assert(endpointEntry != NULL);

    status = rsaJsonRpcEndpoint_create(jsonRpc->ctx, jsonRpc->logHelper, jsonRpc->callsLogFile,
            jsonRpc->interceptorsHandler, endpointDesc, &endpointEntry->endpoint);
    if (status != CELIX_SUCCESS) {
        goto endpoint_err;
    }
    endpointEntry->svc.handle = endpointEntry->endpoint;
    endpointEntry->svc.handleRequest = rsaJsonRpcEndpoint_handleRequest;
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = RSA_RPC_ENDPOINT_SERVICE_NAME;
    opts.serviceVersion = RSA_RPC_ENDPOINT_SERVICE_VERSION;
    opts.svc = &endpointEntry->svc;
    endpointEntry->svcId = celix_bundleContext_registerServiceWithOptionsAsync(jsonRpc->ctx, &opts);
    if (endpointEntry->svcId < 0) {
        celix_logHelper_error(jsonRpc->logHelper, "Error Registering endpoint service.");
        goto endpoint_svc_err;
    }

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_arrayList_add(jsonRpc->svcEndpoints,endpointEntry);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *endpointSvcId = endpointEntry->svcId;

    return CELIX_SUCCESS;

endpoint_svc_err:
    rsaJsonRpcEndpoint_destroy(endpointEntry->endpoint);
endpoint_err:
    free(endpointEntry);
    return status;
}

static void rsaJsonRpc_unregistereEndpointSvcDone(void *data) {
    assert(data != NULL);
    struct rsa_json_rpc_endpoint_entry *endpointEntry = (struct rsa_json_rpc_endpoint_entry *)data;
    rsaJsonRpcEndpoint_destroy(endpointEntry->endpoint);
    free(endpointEntry);
    return;
}

void rsaJsonRpc_uninstallEndpoint(void *handle, long endpointSvcId) {
    if (handle == NULL  || endpointSvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);
    size_t endpointNum = celix_arrayList_size(jsonRpc->svcEndpoints);
    for (int i = 0; i < endpointNum; ++i) {
        struct rsa_json_rpc_endpoint_entry *endpointEntry =
                celix_arrayList_get(jsonRpc->svcEndpoints, i);
        if (endpointEntry->svcId == endpointSvcId) {
            celix_arrayList_remove(jsonRpc->svcEndpoints, endpointEntry);
            celix_bundleContext_unregisterServiceAsync(jsonRpc->ctx, endpointSvcId,
                    endpointEntry, rsaJsonRpc_unregistereEndpointSvcDone);
            break;
        }
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

