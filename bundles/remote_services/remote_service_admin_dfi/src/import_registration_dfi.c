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

#include <stdlib.h>
#include <json_rpc.h>
#include <assert.h>
#include "version.h"
#include "dyn_interface.h"
#include "import_registration.h"
#include "import_registration_dfi.h"
#include "remote_service_admin_dfi.h"
#include "remote_interceptors_handler.h"
#include "remote_service_admin_dfi_constants.h"

struct import_registration {
    celix_bundle_context_t *context;
    endpoint_description_t * endpoint; //TODO owner? -> free when destroyed
    const char *classObject; //NOTE owned by endpoint
    version_pt version;

    celix_thread_mutex_t mutex; //protects send & sendhandle
    send_func_type send;
    void *sendHandle;

    service_factory_pt factory;
    service_registration_t *factoryReg;

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

celix_status_t importRegistration_create(celix_bundle_context_t *context, endpoint_description_t *endpoint, const char *classObject, const char* serviceVersion, FILE *logFile, import_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;
    import_registration_t *reg = calloc(1, sizeof(*reg));

    if (reg != NULL) {
        reg->factory = calloc(1, sizeof(*reg->factory));
    }

    if (reg != NULL && reg->factory != NULL) {
        reg->context = context;
        reg->endpoint = endpoint;
        reg->classObject = classObject;
        reg->proxies = hashMap_create(NULL, NULL, NULL, NULL);

        remoteInterceptorsHandler_create(context, &reg->interceptorsHandler);

        celixThreadMutex_create(&reg->mutex, NULL);
        celixThreadMutex_create(&reg->proxiesMutex, NULL);
        status = version_createVersionFromString((char*)serviceVersion,&(reg->version));

        reg->factory->handle = reg;
        reg->factory->getService = (void *)importRegistration_getService;
        reg->factory->ungetService = (void *)importRegistration_ungetService;
        reg->logFile = logFile;
    } else {
        status = CELIX_ENOMEM;
    }


    if (status == CELIX_SUCCESS) {
        //printf("IMPORT REGISTRATION IS %p\n", reg);
        *out = reg;
    } else {
    	importRegistration_destroy(reg);
    }

    return status;
}


celix_status_t importRegistration_setSendFn(import_registration_t *reg,
                                            send_func_type send,
                                            void *handle) {
    celixThreadMutex_lock(&reg->mutex);
    reg->send = send;
    reg->sendHandle = handle;
    celixThreadMutex_unlock(&reg->mutex);

    return CELIX_SUCCESS;
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

void importRegistration_destroy(import_registration_t *import) {
    if (import != NULL) {
        if (import->proxies != NULL) {
            hashMap_destroy(import->proxies, false, false);
            import->proxies = NULL;
        }

        remoteInterceptorsHandler_destroy(import->interceptorsHandler);

        pthread_mutex_destroy(&import->mutex);
        pthread_mutex_destroy(&import->proxiesMutex);

        if (import->factory != NULL) {
            free(import->factory);
        }

        if(import->version!=NULL){
        	version_destroy(import->version);
        }
        free(import);
    }
}

celix_status_t importRegistration_start(import_registration_t *import) {
    celix_status_t  status = CELIX_SUCCESS;
    if (import->factoryReg == NULL && import->factory != NULL) {
        celix_properties_t *props =  celix_properties_copy(import->endpoint->properties);
        status = bundleContext_registerServiceFactory(import->context, (char *)import->classObject, import->factory, props, &import->factoryReg);
    } else {
        status = CELIX_ILLEGAL_STATE;
    }
    return status;
}

celix_status_t importRegistration_stop(import_registration_t *import) {
    celix_status_t status = CELIX_SUCCESS;

    if (import->factoryReg != NULL) {
        serviceRegistration_unregister(import->factoryReg);
        import->factoryReg = NULL;
    }

    importRegistration_clearProxies(import);

    return status;
}


celix_status_t importRegistration_getService(import_registration_t *import, celix_bundle_t *bundle, service_registration_t *registration, void **out) {
    celix_status_t  status = CELIX_SUCCESS;

    /*
    module_pt module = NULL;
    char *name = NULL;
    bundle_getCurrentModule(bundle, &module);
    module_getSymbolicName(module, &name);
    printf("getting service for bundle '%s'\n", name);
     */


    pthread_mutex_lock(&import->proxiesMutex);
    struct service_proxy *proxy = hashMap_get(import->proxies, bundle);
    if (proxy == NULL) {
        status = importRegistration_createProxy(import, bundle, &proxy);
        if (status == CELIX_SUCCESS) {
            hashMap_put(import->proxies, bundle, proxy);
        }
    }

    if (status == CELIX_SUCCESS) {
        proxy->count += 1;
        *out = proxy->service;
    }
    pthread_mutex_unlock(&import->proxiesMutex);

    return status;
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

    status = dfi_findAvprDescriptor(context, bundle, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        *out = dynInterface_parseAvpr(descriptor);
        fclose(descriptor);
        if (*out == NULL) {
            fprintf(stderr, "RSA_AVPR: Cannot parse avpr descriptor for '%s'", name);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    fprintf(stdout, "RSA: Cannot find/open any valid (avpr) descriptor files for '%s'", name);
    return CELIX_BUNDLE_EXCEPTION;
}

static celix_status_t importRegistration_createProxy(import_registration_t *import, celix_bundle_t *bundle, struct service_proxy **out) {
    dyn_interface_type* intf = NULL;
    celix_status_t  status = importRegistration_findAndParseInterfaceDescriptor(import->context, bundle, import->classObject, &intf);

    if (status != CELIX_SUCCESS) {
        return status;
    }

    /* Check if the imported service version is compatible with the one in the consumer descriptor */
    version_pt consumerVersion = NULL;
    bool isCompatible = false;
    dynInterface_getVersion(intf,&consumerVersion);
    version_isCompatible(consumerVersion,import->version,&isCompatible);

    if(!isCompatible){
    	char* cVerString = NULL;
    	char* pVerString = NULL;
    	version_toString(consumerVersion,&cVerString);
    	version_toString(import->version,&pVerString);
    	printf("Service version mismatch: consumer has %s, provider has %s. NOT creating proxy.\n",cVerString,pVerString);
    	dynInterface_destroy(intf);
    	free(cVerString);
    	free(pVerString);
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

        struct methods_head *list = NULL;
        dynInterface_methods(proxy->intf, &list);
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

    if (import == NULL || import->send == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }


    char *invokeRequest = NULL;
    if (status == CELIX_SUCCESS) {
        status = jsonRpc_prepareInvokeRequest(entry->dynFunc, entry->id, args, &invokeRequest);
        //printf("Need to send following json '%s'\n", invokeRequest);
    }


    if (status == CELIX_SUCCESS) {
        char *reply = NULL;
        int rc = 0;
        //printf("sending request\n");
        celix_properties_t *metadata = NULL;
        bool cont = remoteInterceptorHandler_invokePreProxyCall(import->interceptorsHandler, import->endpoint->properties, entry->name, &metadata);
        if (cont) {
            celixThreadMutex_lock(&import->mutex);
            if (import->send != NULL) {
                import->send(import->sendHandle, import->endpoint, invokeRequest, metadata, &reply, &rc);
            }
            celixThreadMutex_unlock(&import->mutex);
            //printf("request sended. got reply '%s' with status %i\n", reply, rc);

            if (rc == 0 && dynFunction_hasReturn(entry->dynFunc)) {
                //fjprintf("Handling reply '%s'\n", reply);
                status = jsonRpc_handleReply(entry->dynFunc, reply, args);
            }

            *(int *) returnVal = rc;

            remoteInterceptorHandler_invokePostProxyCall(import->interceptorsHandler, import->endpoint->properties, entry->name, metadata);
        }

        if (import->logFile != NULL) {
            static int callCount = 0;
            const char *url = importRegistration_getUrl(import);
            const char *svcName = importRegistration_getServiceName(import);
            fprintf(import->logFile, "REMOTE CALL NR %i\n\turl=%s\n\tservice=%s\n\tpayload=%s\n\treturn_code=%i\n\treply=%s\n",
                                       callCount, url, svcName, invokeRequest, rc, reply);
            fflush(import->logFile);
            callCount += 1;
        }
        free(invokeRequest); //Allocated by json_dumps in jsonRpc_prepareInvokeRequest
        free(reply); //Allocated by json_dumps in remoteServiceAdmin_send through curl call
    }

    if (status != CELIX_SUCCESS) {
        //TODO log error
    }
}

celix_status_t importRegistration_ungetService(import_registration_t *import, celix_bundle_t *bundle, service_registration_t *registration, void **out) {
    celix_status_t  status = CELIX_SUCCESS;

    assert(import != NULL);
    assert(import->proxies != NULL);

    pthread_mutex_lock(&import->proxiesMutex);

    struct service_proxy *proxy = hashMap_get(import->proxies, bundle);
    if (proxy != NULL) {
        if (*out == proxy->service) {
            proxy->count -= 1;
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }

        if (proxy->count == 0) {
            hashMap_remove(import->proxies, bundle);
            importRegistration_destroyProxy(proxy);
        }
    }

    pthread_mutex_unlock(&import->proxiesMutex);

    return status;
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


celix_status_t importRegistration_close(import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    importRegistration_stop(registration);
    return status;
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
    return reg->endpoint->service;
}

