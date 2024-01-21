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

#include "rsa_json_rpc_proxy_impl.h"

#include <assert.h>
#include <stdbool.h>
#include <sys/queue.h>

#include "celix_build_assert.h"
#include "celix_constants.h"
#include "celix_log_helper.h"
#include "celix_long_hash_map.h"
#include "celix_rsa_utils.h"
#include "celix_stdlib_cleanup.h"
#include "celix_version.h"
#include "dfi_utils.h"
#include "endpoint_description.h"
#include "json_rpc.h"
#include "rsa_request_sender_tracker.h"

struct rsa_json_rpc_proxy_factory {
    celix_bundle_context_t* ctx;
    celix_log_helper_t *logHelper;
    FILE *callsLogFile;
    unsigned int serialProtoId;
    celix_service_factory_t factory;
    long factorySvcId;
    endpoint_description_t *endpointDesc;
    celix_long_hash_map_t *proxies;//Key:requestingBundle, Value: rsa_json_rpc_proxy_t *. Work on the celix_event thread , so locks are not required
    remote_interceptors_handler_t *interceptorsHandler;
    rsa_request_sender_tracker_t *reqSenderTracker;
    long reqSenderSvcId;
};

typedef struct rsa_json_rpc_proxy {
    rsa_json_rpc_proxy_factory_t *proxyFactory;
    dyn_interface_type *intfType;
    void *service;
    unsigned int useCnt;
}rsa_json_rpc_proxy_t;

struct rsa_request_sender_callback_data {
    endpoint_description_t *endpointDesc;
    celix_properties_t *metadata;
    const struct iovec *request;
    struct iovec *response;
};

static void* rsaJsonRpcProxy_getService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties);
static void rsaJsonRpcProxy_ungetService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties);
static celix_status_t rsaJsonRpcProxy_create(rsa_json_rpc_proxy_factory_t *proxyFactory,
        const celix_bundle_t *requestingBundle, rsa_json_rpc_proxy_t **proxyOut);
static void rsaJsonRpcProxy_destroy(rsa_json_rpc_proxy_t *proxy);
static void rsaJsonRpcProxy_unregisterFacSvcDone(void *data);

celix_status_t rsaJsonRpcProxy_factoryCreate(celix_bundle_context_t* ctx,
                                             celix_log_helper_t* logHelper,
                                             FILE* logFile,
                                             remote_interceptors_handler_t* interceptorsHandler,
                                             const endpoint_description_t* endpointDesc,
                                             rsa_request_sender_tracker_t* reqSenderTracker,
                                             long requestSenderSvcId,
                                             unsigned int serialProtoId,
                                             rsa_json_rpc_proxy_factory_t** proxyFactoryOut) {
    assert(ctx != NULL);
    assert(logHelper != NULL);
    assert(interceptorsHandler != NULL);
    assert(endpointDesc != NULL);
    assert(reqSenderTracker != NULL);
    assert(requestSenderSvcId > 0);
    assert(proxyFactoryOut != NULL);
    celix_autofree rsa_json_rpc_proxy_factory_t* proxyFactory =
        (rsa_json_rpc_proxy_factory_t*)calloc(1, sizeof(*proxyFactory));
    if (proxyFactory == NULL) {
        return CELIX_ENOMEM;
    }
    proxyFactory->ctx = ctx;
    proxyFactory->logHelper = logHelper;
    proxyFactory->callsLogFile = logFile;
    proxyFactory->interceptorsHandler = interceptorsHandler;
    proxyFactory->reqSenderTracker = reqSenderTracker;
    proxyFactory->reqSenderSvcId = requestSenderSvcId;
    proxyFactory->serialProtoId = serialProtoId;

    CELIX_BUILD_ASSERT(sizeof(long) == sizeof(void*)); // The hash_map uses the pointer as key, so this should be true
    celix_autoptr(celix_long_hash_map_t) proxies = proxyFactory->proxies = celix_longHashMap_create();
    if (proxyFactory->proxies == NULL) {
        celix_logHelper_error(logHelper, "Proxy: Error creating proxy map.");
        return CELIX_ENOMEM;
    }

    celix_autoptr(endpoint_description_t) endpointDescCopy = proxyFactory->endpointDesc =
        endpointDescription_clone(endpointDesc);
    if (proxyFactory->endpointDesc == NULL) {
        celix_logHelper_error(logHelper, "Proxy: Failed to clone endpoint description.");
        return CELIX_ENOMEM;
    }

    proxyFactory->factory.handle = proxyFactory;
    proxyFactory->factory.getService = rsaJsonRpcProxy_getService;
    proxyFactory->factory.ungetService = rsaJsonRpcProxy_ungetService;
    celix_properties_t* svcProperties = NULL;
    celix_status_t status =
        celix_rsaUtils_createServicePropertiesFromEndpointProperties(endpointDesc->properties, &svcProperties);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    assert(svcProperties != NULL);
    proxyFactory->factorySvcId = celix_bundleContext_registerServiceFactoryAsync(
        ctx, &proxyFactory->factory, endpointDesc->serviceName, svcProperties);
    if (proxyFactory->factorySvcId < 0) {
        celix_logHelper_error(logHelper, "Proxy: Error Registering proxy service.");
        return CELIX_SERVICE_EXCEPTION;
    }

    celix_steal_ptr(endpointDescCopy);
    celix_steal_ptr(proxies);
    *proxyFactoryOut = celix_steal_ptr(proxyFactory);
    return CELIX_SUCCESS;
}

void rsaJsonRpcProxy_factoryDestroy(rsa_json_rpc_proxy_factory_t *proxyFactory) {
    assert(proxyFactory != NULL);
    celix_bundleContext_unregisterServiceAsync(proxyFactory->ctx, proxyFactory->factorySvcId,
            proxyFactory, rsaJsonRpcProxy_unregisterFacSvcDone);
}

long rsaJsonRpcProxy_factorySvcId(rsa_json_rpc_proxy_factory_t *proxyFactory) {
    return proxyFactory->factorySvcId;
}

static void rsaJsonRpcProxy_unregisterFacSvcDone(void *data) {
    assert(data);
    rsa_json_rpc_proxy_factory_t *proxyFactory = (rsa_json_rpc_proxy_factory_t *)data;
    endpointDescription_destroy(proxyFactory->endpointDesc);
    assert(celix_longHashMap_size(proxyFactory->proxies) == 0);
    celix_longHashMap_destroy(proxyFactory->proxies);
    free(proxyFactory);
    return;
}

static void* rsaJsonRpcProxy_getService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties) {
    assert(handle != NULL);
    assert(requestingBundle != NULL);
    assert(svcProperties != NULL);
    celix_status_t status = CELIX_SUCCESS;
    rsa_json_rpc_proxy_factory_t *proxyFactory = (rsa_json_rpc_proxy_factory_t *)handle;

    rsa_json_rpc_proxy_t *proxy = celix_longHashMap_get(proxyFactory->proxies, (long)requestingBundle);
    if (proxy == NULL) {
        status = rsaJsonRpcProxy_create(proxyFactory, requestingBundle, &proxy);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(proxyFactory->logHelper,"Error Creating service proxy for %s. %d",
                    proxyFactory->endpointDesc->serviceName, status);
            return NULL;
        }
        celix_longHashMap_put(proxyFactory->proxies, (long)requestingBundle, proxy);
    }
    proxy->useCnt += 1;

    return proxy->service;
}

static void rsaJsonRpcProxy_ungetService(void *handle, const celix_bundle_t *requestingBundle,
        const celix_properties_t *svcProperties) {
    assert(handle != NULL);
    assert(requestingBundle != NULL);
    assert(svcProperties != NULL);
    rsa_json_rpc_proxy_factory_t *proxyFactory = (rsa_json_rpc_proxy_factory_t *)handle;
    rsa_json_rpc_proxy_t *proxy = celix_longHashMap_get(proxyFactory->proxies, (long)requestingBundle);
    if (proxy != NULL) {
        proxy->useCnt -= 1;
        if (proxy->useCnt == 0) {
            (void)celix_longHashMap_remove(proxyFactory->proxies, (long)requestingBundle);
            rsaJsonRpcProxy_destroy(proxy);
        }
    }
    return;
}

static celix_status_t rsaJsonRpcProxy_useReqSenderSvcCallback(void *handle, rsa_request_sender_service_t *svc) {
    assert(handle != NULL);
    assert(svc != NULL);
    struct rsa_request_sender_callback_data *data = (struct rsa_request_sender_callback_data *)handle;
    return svc->sendRequest(svc->handle, data->endpointDesc, data->metadata,
            data->request, data->response);
}

static void rsaJsonRpcProxy_serviceFunc(void *userData, void *args[], void *returnVal) {
    celix_status_t  status = CELIX_SUCCESS;
    if (returnVal == NULL) {
        return;
    }
    if ((args == NULL) || (*((void **)args[0]) == NULL)) {
        *(celix_status_t *)returnVal = CELIX_ILLEGAL_ARGUMENT;
        return;
    }
    assert(userData != NULL);
    struct method_entry *entry = userData;
    rsa_json_rpc_proxy_t *proxy = *((void **)args[0]);
    rsa_json_rpc_proxy_factory_t *proxyFactory = proxy->proxyFactory;
    assert(proxyFactory != NULL);

    char *invokeRequest = NULL;
    int rc = jsonRpc_prepareInvokeRequest(entry->dynFunc, entry->id, args, &invokeRequest);
    if (rc != 0) {
        celix_logHelper_logTssErrors(proxyFactory->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(proxyFactory->logHelper, "Error preparing invoke request for %s", entry->name);
        *(celix_status_t *)returnVal = CELIX_SERVICE_EXCEPTION;
        return;
    }

    struct iovec replyIovec = {NULL,0};
    celix_properties_t *metadata = celix_properties_create();
    if (metadata == NULL) {
        celix_logHelper_error(proxyFactory->logHelper,"Error creating metadata for %s", entry->name);
        free(invokeRequest);
        *(celix_status_t *)returnVal = CELIX_ENOMEM;
        return;
    }
    celix_properties_setLong(metadata, "SerialProtocolId", proxyFactory->serialProtoId);
    bool cont = remoteInterceptorHandler_invokePreProxyCall(proxyFactory->interceptorsHandler,
            proxyFactory->endpointDesc->properties, entry->name, &metadata);
    if (cont) {
        struct iovec requestIovec = {invokeRequest,strlen(invokeRequest) + 1};
        struct rsa_request_sender_callback_data data= {
                .endpointDesc = proxyFactory->endpointDesc,
                .metadata = metadata,
                .request = &requestIovec,
                .response = &replyIovec
        };
        status = rsaRequestSenderTracker_useService(proxyFactory->reqSenderTracker, proxyFactory->reqSenderSvcId,
                &data, rsaJsonRpcProxy_useReqSenderSvcCallback);
        if (status == CELIX_SUCCESS && dynFunction_hasReturn(entry->dynFunc)) {
            if (replyIovec.iov_base != NULL) {
                int rsErrno = CELIX_SUCCESS;
                int retVal = jsonRpc_handleReply(entry->dynFunc,
                        (const char *)replyIovec.iov_base , args, &rsErrno);
                if(retVal != 0) {
                    status = CELIX_SERVICE_EXCEPTION;
                    celix_logHelper_logTssErrors(proxyFactory->logHelper, CELIX_LOG_LEVEL_ERROR);
                    celix_logHelper_error(proxyFactory->logHelper, "Error handling reply for %s", entry->name);
                } else if (rsErrno != CELIX_SUCCESS) {
                    //return the invocation error of remote service function
                    status = rsErrno;
                }
            } else {
                celix_logHelper_error(proxyFactory->logHelper,"Expect service proxy has return, but reply is empty.");
                status = CELIX_ILLEGAL_ARGUMENT;
            }
        } else if (status != CELIX_SUCCESS) {
            celix_logHelper_error(proxyFactory->logHelper,"Service proxy send request failed. %d", status);
        }
        remoteInterceptorHandler_invokePostProxyCall(proxyFactory->interceptorsHandler,
                proxyFactory->endpointDesc->properties, entry->name, metadata);
    } else {
        celix_logHelper_error(proxyFactory->logHelper, "%s has been intercepted.", proxyFactory->endpointDesc->serviceName);
        status = CELIX_INTERCEPTOR_EXCEPTION;
    }

    //free metadata
    if(metadata != NULL) {
        celix_properties_destroy(metadata);
    }

    if (proxyFactory->callsLogFile != NULL) {
        fprintf(proxyFactory->callsLogFile, "PROXY REMOTE CALL:\n\tservice=%s\n\tservice_id=%lu\n\trequest_payload=%s\n\trequest_response=%s\n\tstatus=%i\n",
                proxyFactory->endpointDesc->serviceName, proxyFactory->endpointDesc->serviceId, invokeRequest, (char *)replyIovec.iov_base, status);
        fflush(proxyFactory->callsLogFile);
    }


    free(invokeRequest); //Allocated by json_dumps in jsonRpc_prepareInvokeRequest
    if (replyIovec.iov_base) {
        free(replyIovec.iov_base); //Allocated by json_dumps
    }

    *(celix_status_t *) returnVal = status;

    return;
}

static celix_status_t rsaJsonRpcProxy_create(rsa_json_rpc_proxy_factory_t *proxyFactory,
        const celix_bundle_t *requestingBundle, rsa_json_rpc_proxy_t **proxyOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree rsa_json_rpc_proxy_t *proxy = calloc(1, sizeof(*proxy));
    if (proxy == NULL) {
        return CELIX_ENOMEM;
    }
    proxy->proxyFactory = proxyFactory;
    proxy->useCnt = 0;

    celix_autoptr(dyn_interface_type) intfType = NULL;
    status = dfi_findAndParseInterfaceDescriptor(proxyFactory->logHelper,
            proxyFactory->ctx, requestingBundle, proxyFactory->endpointDesc->serviceName, &intfType);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    proxy->intfType = intfType;

    //Check service version
    const char *providerVerStr = celix_properties_get(proxyFactory->endpointDesc->properties,CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
    if (providerVerStr == NULL) {
        celix_logHelper_error(proxyFactory->logHelper, "Proxy: Error getting provider service version.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autoptr(celix_version_t) providerVersion = celix_version_createVersionFromString(providerVerStr);
    if (providerVersion == NULL) {
        status = CELIX_ENOMEM;
        celix_logHelper_error(proxyFactory->logHelper, "Proxy: Error converting service version type. %d.", status);
        return status;
    }
    celix_version_t *consumerVersion = NULL;
    bool isCompatible = false;
    dynInterface_getVersion(intfType,&consumerVersion);
    isCompatible = celix_version_isCompatible(consumerVersion, providerVersion);
    if(!isCompatible){
        celix_logHelper_error(proxyFactory->logHelper, "Proxy: Service version mismatch, consumer has %d.%d.%d, provider has %s.",
                              celix_version_getMajor(consumerVersion), celix_version_getMinor(consumerVersion),
                              celix_version_getMicro(consumerVersion) , providerVerStr);
        return CELIX_SERVICE_EXCEPTION;
    }

    size_t intfMethodNb = dynInterface_nrOfMethods(intfType);
    proxy->service = calloc(1 + intfMethodNb, sizeof(void *));//The interface includes 'void *handle' and its methods
    if (proxy->service == NULL) {
        celix_logHelper_error(proxyFactory->logHelper, "Proxy: Failed to allocate memory for service.");
        return CELIX_ENOMEM;
    }
    celix_autofree void **service = (void **)proxy->service;
    service[0] = proxy;
    struct methods_head *list = NULL;
    dynInterface_methods(intfType, &list);
    struct method_entry *entry = NULL;
    void (*fn)(void) = NULL;
    int index = 0;
    TAILQ_FOREACH(entry, list, entries) {
        int rc = dynFunction_createClosure(entry->dynFunc, rsaJsonRpcProxy_serviceFunc, entry, &fn);
        if (rc != 0) {
            celix_logHelper_error(proxyFactory->logHelper, "Proxy: Failed to create closure for service function %s.", entry->name);
            return CELIX_SERVICE_EXCEPTION;
        }
        service[++index] = fn;
    }

    celix_steal_ptr(service);
    celix_steal_ptr(intfType);
    *proxyOut = celix_steal_ptr(proxy);

    return CELIX_SUCCESS;
};

static void rsaJsonRpcProxy_destroy(rsa_json_rpc_proxy_t *proxy) {
    free(proxy->service);
    dynInterface_destroy(proxy->intfType);
    free(proxy);
    return;
}


