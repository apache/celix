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

#include <rsa_json_rpc_proxy_impl.h>
#include <rsa_rpc_request_sender.h>
#include <json_rpc.h>
#include <dfi_utils.h>
#include <celix_api.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <assert.h>


struct rsa_json_rpc_proxy {
    celix_bundle_context_t* ctx;
    celix_log_helper_t *logHelper;
    FILE *callsLogFile;
    const endpoint_description_t *endpointDesc;
    remote_interceptors_handler_t *interceptorsHandler;
    dyn_interface_type *intfType;
    void *service;
    rsa_rpc_request_sender_t *requestSender;
};

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
    char *invokeRequest = NULL;

    int rc = jsonRpc_prepareInvokeRequest(entry->dynFunc, entry->id, args, &invokeRequest);
    if (rc != 0) {
        *(celix_status_t *)returnVal = CELIX_SERVICE_EXCEPTION;
        return;
    }

    struct iovec replyIovec = {NULL,0};
    celix_properties_t *metadata = NULL;
    bool cont = remoteInterceptorHandler_invokePreProxyCall(proxy->interceptorsHandler,
            proxy->endpointDesc->properties, entry->name, &metadata);
    if (cont) {
        struct iovec requestIovec = {invokeRequest,strlen(invokeRequest) + 1};
        status = rsaRpcRequestSender_send(proxy->requestSender, proxy->endpointDesc,
                metadata, &requestIovec, &replyIovec);
        if (status == CELIX_SUCCESS && dynFunction_hasReturn(entry->dynFunc)) {
            if (replyIovec.iov_base != NULL) {
                int rsErrno = CELIX_SUCCESS;
                int retVal = jsonRpc_handleReply(entry->dynFunc,
                        (const char *)replyIovec.iov_base , args, &rsErrno);
                if(retVal != 0) {
                    status = CELIX_SERVICE_EXCEPTION;
                } else if (rsErrno != CELIX_SUCCESS) {
                    //return the invocation error of remote service function
                    status = rsErrno;
                }
            } else {
                celix_logHelper_error(proxy->logHelper,"Expect service proxy has return, but reply is empty.");
                status = CELIX_ILLEGAL_ARGUMENT;
            }
        } else if (status != CELIX_SUCCESS) {
            celix_logHelper_error(proxy->logHelper,"Service proxy send request failed. %d", status);
        }
        remoteInterceptorHandler_invokePostProxyCall(proxy->interceptorsHandler,
                proxy->endpointDesc->properties, entry->name, metadata);
    } else {
        celix_logHelper_error(proxy->logHelper, "%s has been intercepted.", proxy->endpointDesc->service);
        status = CELIX_INTERCEPTOR_EXCEPTION;
    }

    //free metadata
    if(metadata != NULL) {
        celix_properties_destroy(metadata);
    }

    if (proxy->callsLogFile != NULL) {
        fprintf(proxy->callsLogFile, "PROXY REMOTE CALL:\n\tservice=%s\n\tservice_id=%lu\n\trequest_payload=%s\n\trequest_response=%s\n\tstatus=%i\n",
                proxy->endpointDesc->service, proxy->endpointDesc->serviceId, invokeRequest, (char *)replyIovec.iov_base, status);
        fflush(proxy->callsLogFile);
    }


    free(invokeRequest); //Allocated by json_dumps in jsonRpc_prepareInvokeRequest
    if (replyIovec.iov_base) {
        free(replyIovec.iov_base); //Allocated by json_dumps
    }

    *(celix_status_t *) returnVal = status;

    return;
}

bool rsaJsonRpcProxy_isInitParamInvalid(const struct rsa_json_rpc_proxy_init_param *initParam) {
    if (initParam == NULL) {
        return true;
    }
    if (initParam->ctx == NULL || initParam->logHelper == NULL
            || initParam->interceptorsHandler == NULL || initParam->requestingBundle == NULL
            || initParam->requestSender == NULL) {
        return true;
    }
    const endpoint_description_t *endpointDesc = initParam->endpointDesc;
    if (endpointDesc == NULL || endpointDesc->frameworkUUID == NULL
            || endpointDesc->id == NULL || endpointDesc->service == NULL
            || endpointDesc->properties == NULL || (long)endpointDesc->serviceId < 0) {
        return true;
    }
    return false;
}

celix_status_t rsaJsonRpcProxy_create(const struct rsa_json_rpc_proxy_init_param *initParam,
        rsa_json_rpc_proxy_t **proxyOut) {
    if (rsaJsonRpcProxy_isInitParamInvalid(initParam)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t status = CELIX_SUCCESS;
    dyn_interface_type *intfType = NULL;
    rsa_json_rpc_proxy_t *proxy = calloc(1, sizeof(*proxy));
    assert(proxy != NULL);
    proxy->ctx = initParam->ctx;
    proxy->logHelper = initParam->logHelper;
    proxy->callsLogFile = initParam->logFile;
    proxy->endpointDesc = initParam->endpointDesc;
    proxy->interceptorsHandler = initParam->interceptorsHandler;
    proxy->requestSender = initParam->requestSender;
    status = dfi_findAndParseInterfaceDescriptor(proxy->logHelper,
            proxy->ctx, initParam->requestingBundle, proxy->endpointDesc->service, &intfType);
    if (status != CELIX_SUCCESS) {
        goto intf_descriptor_err;
    }
    proxy->intfType = intfType;

    //Check service version
    const char *providerVerStr = celix_properties_get(proxy->endpointDesc->properties,CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
    if (providerVerStr == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        celix_logHelper_error(proxy->logHelper, "Proxy: Error getting provider service version.");
        goto err_getting_svc_ver;
    }
    version_pt providerVersion;
    status =version_createVersionFromString(providerVerStr,&providerVersion);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(proxy->logHelper, "Proxy: Error converting service version type. %d.", status);
        goto err_creating_provider_ver;
    }
    version_pt consumerVersion = NULL;
    bool isCompatible = false;
    dynInterface_getVersion(intfType,&consumerVersion);
    version_isCompatible(consumerVersion, providerVersion,&isCompatible);
    if(!isCompatible){
        char* consumerVerStr = NULL;
        version_toString(consumerVersion,&consumerVerStr);
        celix_logHelper_error(proxy->logHelper, "Proxy: Service version mismatch, consumer has %s, provider has %s.", consumerVerStr, providerVerStr);
        free(consumerVerStr);
        status = CELIX_SERVICE_EXCEPTION;
        goto svc_ver_mismatch;
    }

    size_t intfMethodNb = dynInterface_nrOfMethods(intfType);
    proxy->service = calloc(1 + intfMethodNb, sizeof(void *));//The interface includes 'void *handle' and its methods
    assert(proxy->service != NULL);
    void **service = (void **)proxy->service;
    service[0] = proxy;
    struct methods_head *list = NULL;
    dynInterface_methods(intfType, &list);
    struct method_entry *entry = NULL;
    void (*fn)(void) = NULL;
    int index = 0;
    TAILQ_FOREACH(entry, list, entries) {
        int rc = dynFunction_createClosure(entry->dynFunc, rsaJsonRpcProxy_serviceFunc, entry, &fn);
        if (rc != 0) {
            status = CELIX_BUNDLE_EXCEPTION;
            goto fn_closure_err;
        }
        service[++index] = fn;
    }

    *proxyOut = proxy;

    version_destroy(providerVersion);

    return CELIX_SUCCESS;

fn_closure_err:
    free(proxy->service);
svc_ver_mismatch:
    version_destroy(providerVersion);
err_creating_provider_ver:
err_getting_svc_ver:
    dynInterface_destroy(intfType);
intf_descriptor_err:
    free(proxy);
    return status;
};

void rsaJsonRpcProxy_destroy(rsa_json_rpc_proxy_t *proxy) {
    if (proxy != NULL) {
        free(proxy->service);
        dynInterface_destroy(proxy->intfType);
        free(proxy);
    }
    return;
}

void *rsaJsonRpcProxy_getService(rsa_json_rpc_proxy_t *proxy) {
    if (proxy == NULL) {
        return NULL;
    }
    return proxy->service;
}

