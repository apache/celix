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

#include "import_registration_dfi.h"

#include <stdlib.h>
#include <json_rpc.h>
#include <assert.h>

#include "celix_version.h"
#include "celix_stdlib_cleanup.h"
#include "celix_rsa_utils.h"
#include "celix_constants.h"
#include "dyn_interface.h"
#include "import_registration.h"
#include "remote_interceptors_handler.h"
#include "remote_service_admin_dfi.h"
#include "remote_service_admin_dfi_constants.h"

struct import_registration {
    celix_bundle_context_t *context;
    endpoint_description_t * endpoint; //TODO owner? -> free when destroyed
    const char *classObject; //NOTE owned by endpoint
    celix_version_t* version;

    send_func_type send;
    void *sendHandle;

    celix_service_factory_t factory;
    long factorySvcId;

    hash_map_pt proxies; //key -> bundle, value -> service_proxy
    celix_thread_mutex_t proxiesMutex; //protects proxies

    remote_interceptors_handler_t *interceptorsHandler;

    FILE *logFile;
};

struct service_proxy {
    dyn_interface_type *intf;
    void *service;
    size_t count;
};

static celix_status_t importRegistration_findAndParseInterfaceDescriptor(celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out);

static celix_status_t importRegistration_createProxy(import_registration_t *import, celix_bundle_t *bundle,
                                              struct service_proxy **proxy);
static void importRegistration_proxyFunc(void *userData, void *args[], void *returnVal);
static void importRegistration_destroyProxy(struct service_proxy *proxy);
static void importRegistration_clearProxies(import_registration_t *import);
static const char* importRegistration_getUrl(import_registration_t *reg);
static const char* importRegistration_getServiceName(import_registration_t *reg);
static void* importRegistration_getService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
void importRegistration_ungetService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);

celix_status_t importRegistration_create(
        celix_bundle_context_t *context,
        endpoint_description_t *endpoint,
        const char *classObject,
        const char* serviceVersion,
        send_func_type sendFn,
        void* sendFnHandle,
        FILE *logFile,
        import_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;
    import_registration_t *reg = calloc(1, sizeof(*reg));
    reg->context = context;
    reg->endpoint = endpoint;
    reg->classObject = classObject;
    reg->send = sendFn;
    reg->sendHandle = sendFnHandle;
    reg->proxies = hashMap_create(NULL, NULL, NULL, NULL);

    remoteInterceptorsHandler_create(context, &reg->interceptorsHandler);

    celixThreadMutex_create(&reg->proxiesMutex, NULL);
    // serviceVersion == NULL is allowed, check TEST_F(RsaDfiTests, ImportService)
    reg->version = celix_version_createVersionFromString(serviceVersion);
    if (serviceVersion != NULL && reg->version == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    reg->factorySvcId = -1;
    reg->factory.handle = reg;
    reg->factory.getService = importRegistration_getService;
    reg->factory.ungetService = importRegistration_ungetService;
    reg->logFile = logFile;


    if (status == CELIX_SUCCESS) {
        //printf("IMPORT REGISTRATION IS %p\n", reg);
        *out = reg;
    } else {
    	importRegistration_destroy(reg);
    }

    return status;
}

static void importRegistration_clearProxies(import_registration_t *import) {
    if (import != NULL) {
        pthread_mutex_lock(&import->proxiesMutex);
        if (import->proxies != NULL) {
            hash_map_iterator_pt iter = hashMapIterator_create(import->proxies);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt  entry = hashMapIterator_nextEntry(iter);
                struct service_proxy *proxy = hashMapEntry_getValue(entry);
                importRegistration_destroyProxy(proxy);
            }
            hashMapIterator_destroy(iter);
        }
        pthread_mutex_unlock(&import->proxiesMutex);
    }
}

static void importRegistration_destroyCallback(void* data) {
    import_registration_t* import = data;
    importRegistration_clearProxies(import);
    if (import->proxies != NULL) {
        hashMap_destroy(import->proxies, false, false);
        import->proxies = NULL;
    }

    remoteInterceptorsHandler_destroy(import->interceptorsHandler);

    pthread_mutex_destroy(&import->proxiesMutex);

    if (import->version != NULL) {
        celix_version_destroy(import->version);
    }
    free(import);
}

void importRegistration_destroy(import_registration_t *import) {
    if (import != NULL) {
        if (import->factorySvcId >= 0) {
            celix_bundleContext_unregisterServiceAsync(import->context, import->factorySvcId, import, importRegistration_destroyCallback);
        } else {
            importRegistration_destroyCallback(import);
        }
    }
}

celix_status_t importRegistration_start(import_registration_t *import) {
    celix_properties_t* svcProperties = NULL;
    celix_status_t status = celix_rsaUtils_createServicePropertiesFromEndpointProperties(import->endpoint->properties, &svcProperties);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    import->factorySvcId = celix_bundleContext_registerServiceFactoryAsync(import->context, &import->factory, import->classObject, svcProperties);
    return import->factorySvcId >= 0 ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
}

static void* importRegistration_getService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties) {
    celix_status_t  status = CELIX_SUCCESS;
    void* svc = NULL;
    import_registration_t* import = handle;

    /*
    module_pt module = NULL;
    char *name = NULL;
    bundle_getCurrentModule(bundle, &module);
    module_getSymbolicName(module, &name);
    printf("getting service for bundle '%s'\n", name);
     */


    pthread_mutex_lock(&import->proxiesMutex);
    struct service_proxy *proxy = hashMap_get(import->proxies, requestingBundle);
    if (proxy == NULL) {
        status = importRegistration_createProxy(import, (celix_bundle_t*)requestingBundle, &proxy);
        if (status == CELIX_SUCCESS) {
            hashMap_put(import->proxies, (void*)requestingBundle, proxy);
        }
    }
    if (status == CELIX_SUCCESS) {
        proxy->count += 1;
        svc = proxy->service;
    }
    pthread_mutex_unlock(&import->proxiesMutex);

    return svc;
}

void importRegistration_ungetService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties) {
    import_registration_t* import = handle;
    assert(import != NULL);
    pthread_mutex_lock(&import->proxiesMutex);
    assert(import->proxies != NULL);
    struct service_proxy *proxy = hashMap_get(import->proxies, requestingBundle);
    if (proxy != NULL) {
        proxy->count -= 1;
        if (proxy->count == 0) {
            hashMap_remove(import->proxies, requestingBundle);
            importRegistration_destroyProxy(proxy);
        }
    }
    pthread_mutex_unlock(&import->proxiesMutex);
}

static celix_status_t importRegistration_findAndParseInterfaceDescriptor(celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out) {
    celix_status_t status = CELIX_SUCCESS;
    FILE* descriptor = NULL;
    status = dfi_findDescriptor(context, bundle, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        int rc = dynInterface_parse(descriptor, out);
        fclose(descriptor);
        if (rc != 0) {
            fprintf(stderr, "RSA_DFI: Cannot parse dfi descriptor for '%s'", name);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    fprintf(stdout, "RSA: Cannot find/open any valid descriptor files for '%s'", name);
    return CELIX_BUNDLE_EXCEPTION;
}

static celix_status_t importRegistration_createProxy(import_registration_t *import, celix_bundle_t *bundle, struct service_proxy **out) {
    dyn_interface_type* intf = NULL;
    celix_status_t  status = importRegistration_findAndParseInterfaceDescriptor(import->context, bundle, import->classObject, &intf);

    if (status != CELIX_SUCCESS) {
        return status;
    }

    /* Check if the imported service version is compatible with the one in the consumer descriptor */
    const celix_version_t* consumerVersion = dynInterface_getVersion(intf);
    bool isCompatible = false;
    isCompatible = celix_version_isCompatible(consumerVersion,import->version);

    if(!isCompatible){
        celix_autofree char* cVerString = NULL;
        celix_autofree char* pVerString = NULL;
        cVerString = celix_version_toString(consumerVersion);
        pVerString = import->version != NULL ? celix_version_toString(import->version) : NULL;
        printf("Service version mismatch: consumer has %s, provider has %s. NOT creating proxy.\n",
               cVerString,pVerString != NULL ? pVerString : "NA");
        dynInterface_destroy(intf);
        status = CELIX_SERVICE_EXCEPTION;
    }

    struct service_proxy *proxy = NULL;
    if (status == CELIX_SUCCESS) {
        proxy = calloc(1, sizeof(*proxy));
        if (proxy == NULL) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
    	proxy->intf = intf;
        size_t count = dynInterface_nrOfMethods(proxy->intf);
        proxy->service = calloc(1 + count, sizeof(void *));
        if (proxy->service == NULL) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
        void **serv = proxy->service;
        serv[0] = import;

        const struct methods_head* list = dynInterface_methods(proxy->intf);
        struct method_entry *entry = NULL;
        void (*fn)(void) = NULL;
        int index = 0;
        TAILQ_FOREACH(entry, list, entries) {
            int rc = dynFunction_createClosure(entry->dynFunc, importRegistration_proxyFunc, entry, &fn);
            serv[index + 1] = fn;
            index += 1;

            if (rc != 0) {
                status = CELIX_BUNDLE_EXCEPTION;
                break;
            }
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = proxy;
    } else if (proxy != NULL) {
        if (proxy->intf != NULL) {
            dynInterface_destroy(proxy->intf);
            proxy->intf = NULL;
        }
        free(proxy->service);
        free(proxy);
    }

    return status;
}

static void importRegistration_proxyFunc(void *userData, void *args[], void *returnVal) {
    int  status = CELIX_SUCCESS;
    struct method_entry *entry = userData;
    import_registration_t *import = *((void **)args[0]);

    *(int *) returnVal = CELIX_SUCCESS;

    if (import == NULL || import->send == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }


    char *invokeRequest = NULL;
    if (status == CELIX_SUCCESS) {
        int rc = jsonRpc_prepareInvokeRequest(entry->dynFunc, entry->id, args, &invokeRequest);
        status = (rc != 0) ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
        //printf("Need to send following json '%s'\n", invokeRequest);
    }


    if (status == CELIX_SUCCESS) {
        char *reply = NULL;
        //printf("sending request\n");
        celix_properties_t *metadata = NULL;
        bool cont = remoteInterceptorHandler_invokePreProxyCall(import->interceptorsHandler, import->endpoint->properties,
                                                                dynFunction_getName(entry->dynFunc), &metadata);
        if (cont) {
            status = import->send(import->sendHandle, import->endpoint, invokeRequest, metadata, &reply);
            //printf("request sended. got reply '%s' with status %i\n", reply, rc);

            if (status == CELIX_SUCCESS && dynFunction_hasReturn(entry->dynFunc)) {
                //fjprintf("Handling reply '%s'\n", reply);
                int rsErrno = CELIX_SUCCESS;
                int retVal = jsonRpc_handleReply(entry->dynFunc, reply, args, &rsErrno);
                if(retVal != 0) {
                    status = CELIX_BUNDLE_EXCEPTION;
                } else if (rsErrno != CELIX_SUCCESS) {
                    //return the invocation error of remote service function
                    *(int *) returnVal = rsErrno;
                }
            }

            remoteInterceptorHandler_invokePostProxyCall(import->interceptorsHandler, import->endpoint->properties,
                                                         dynFunction_getName(entry->dynFunc), metadata);
        } else {
            *(int *) returnVal = CELIX_INTERCEPTOR_EXCEPTION;
        }

        //free metadata
        if(metadata != NULL) {
            celix_properties_destroy(metadata);
        }

        if (import->logFile != NULL) {
            static int callCount = 0;
            const char *url = importRegistration_getUrl(import);
            const char *svcName = importRegistration_getServiceName(import);
            fprintf(import->logFile, "REMOTE CALL NR %i\n\turl=%s\n\tservice=%s\n\tpayload=%s\n\treturn_code=%i\n\treply=%s\n",
                                       callCount, url, svcName, invokeRequest, status, reply == NULL ? "null" : reply);
            fflush(import->logFile);
            callCount += 1;
        }
        free(invokeRequest); //Allocated by json_dumps in jsonRpc_prepareInvokeRequest
        free(reply); //Allocated by json_dumps in remoteServiceAdmin_send through curl call
    }

    if (status != CELIX_SUCCESS) {
        //TODO log error
        *(int *) returnVal = status;
    }
}

static void importRegistration_destroyProxy(struct service_proxy *proxy) {
    if (proxy != NULL) {
        if (proxy->intf != NULL) {
            dynInterface_destroy(proxy->intf);
        }
        if (proxy->service != NULL) {
            free(proxy->service);
        }
        free(proxy);
    }
}

celix_status_t importRegistration_getException(import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t importRegistration_getImportReference(import_registration_t *registration, import_reference_t **reference) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

endpoint_description_t* importRegistration_getEndpointDescription(import_registration_t *registration) {
    return registration->endpoint;
}

celix_status_t importReference_getImportedEndpoint(import_reference_t *reference) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t importReference_getImportedService(import_reference_t *reference) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

static const char* importRegistration_getUrl(import_registration_t *reg) {
    return celix_properties_get(reg->endpoint->properties, RSA_DFI_ENDPOINT_URL, "!Error!");
}

static const char* importRegistration_getServiceName(import_registration_t *reg) {
    return reg->endpoint->serviceName;
}

