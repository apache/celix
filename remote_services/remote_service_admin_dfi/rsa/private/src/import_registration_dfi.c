#include <stdlib.h>
#include <jansson.h>
#include "json_serializer.h"
#include "dyn_interface.h"
#include "import_registration.h"
#include "import_registration_dfi.h"

struct import_registration {
    bundle_context_pt context;
    endpoint_description_pt  endpoint; //TODO owner? -> free when destroyed
    const char *classObject; //NOTE owned by endpoint
    send_func_type send;
    void *sendHandle;

    service_factory_pt factory;
    service_registration_pt factoryReg;

    hash_map_pt proxies; //key -> bundle, value -> service_proxy
};

struct service_proxy {
    dyn_interface_type *intf;
    void *service;
    int count;
};

static celix_status_t importRegistration_createProxy(import_registration_pt import, bundle_pt bundle,
                                              struct service_proxy **proxy);
static void importRegistration_proxyFunc(void *userData, void *args[], void *returnVal);
static void importRegistration_destroyProxy(struct service_proxy *proxy);

celix_status_t importRegistration_create(bundle_context_pt context, endpoint_description_pt  endpoint, const char *classObject, import_registration_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    import_registration_pt reg = calloc(1, sizeof(*reg));

    if (reg != NULL) {
        reg->factory = calloc(1, sizeof(*reg->factory));
    }

    if (reg != NULL && reg->factory != NULL) {
        reg->context = context;
        reg->endpoint = endpoint;
        reg->classObject = classObject;
        reg->proxies = hashMap_create(NULL, NULL, NULL, NULL);

        reg->factory->factory = reg;
        reg->factory->getService = (void *)importRegistration_getService;
        reg->factory->ungetService = (void *)importRegistration_ungetService;
    } else {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        printf("IMPORT REGISTRATION IS %p\n", reg);
        *out = reg;
    }

    return status;
}


celix_status_t importRegistration_setSendFn(import_registration_pt reg,
                                            send_func_type send,
                                            void *handle) {
    reg->send = send;
    reg->sendHandle = handle;

    return CELIX_SUCCESS;
}

void importRegistration_destroy(import_registration_pt import) {
    if (import != NULL) {
        if (import->proxies != NULL) {
            //TODO destroy proxies
            hashMap_destroy(import->proxies, false, false);
            import->proxies = NULL;
        }
        if (import->factory != NULL) {
            free(import->factory);
        }
        free(import);
    }
}

celix_status_t importRegistration_start(import_registration_pt import) {
    celix_status_t  status = CELIX_SUCCESS;
    if (import->factoryReg == NULL && import->factory != NULL) {
        status = bundleContext_registerServiceFactory(import->context, (char *)import->classObject, import->factory, NULL /*TODO*/, &import->factoryReg);
    } else {
        status = CELIX_ILLEGAL_STATE;
    }
    return status;
}

celix_status_t importRegistration_stop(import_registration_pt import) {
    celix_status_t status = CELIX_SUCCESS;
    if (import->factoryReg != NULL) {
        serviceRegistration_unregister(import->factoryReg);
    }
    //TODO unregister every serv instance?
    return status;
}


celix_status_t importRegistration_getService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **out) {
    celix_status_t  status = CELIX_SUCCESS;

    /*
    module_pt module = NULL;
    char *name = NULL;
    bundle_getCurrentModule(bundle, &module);
    module_getSymbolicName(module, &name);
    printf("getting service for bundle '%s'\n", name);
     */

    struct service_proxy *proxy = hashMap_get(import->proxies, bundle); //TODO lock
    if (proxy == NULL) {
        status = importRegistration_createProxy(import, bundle, &proxy);
        if (status == CELIX_SUCCESS) {
            hashMap_put(import->proxies, bundle, proxy); //TODO lock
        }
    }

    if (status == CELIX_SUCCESS) {
        proxy->count += 1;
        *out = proxy->service;
    }

    return status;
}

static celix_status_t importRegistration_createProxy(import_registration_pt import, bundle_pt bundle, struct service_proxy **out) {
    celix_status_t  status = CELIX_SUCCESS;

    char *descriptorFile = NULL;
    char name[128];
    snprintf(name, 128, "%s.descriptor", import->classObject);
    status = bundle_getEntry(bundle, name, &descriptorFile);
    if (descriptorFile == NULL) {
        printf("Cannot find entry '%s'\n", name);
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        printf("Found descriptor at '%s'\n", descriptorFile);
    }

    struct service_proxy *proxy = NULL;
    if (status == CELIX_SUCCESS) {
        proxy = calloc(1, sizeof(*proxy));
        if (proxy == NULL) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
        FILE *df = fopen(descriptorFile, "r");
        if (df != NULL) {
            int rc = dynInterface_parse(df, &proxy->intf);
            fclose(df);
            if (rc != 0) {
                status = CELIX_BUNDLE_EXCEPTION;
            }
        }
    }


    if (status == CELIX_SUCCESS) {
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
    } else {
        if (proxy->intf != NULL) {
            dynInterface_destroy(proxy->intf);
            proxy->intf = NULL;
        }
        if (proxy->service != NULL) {
            free(proxy->service);
            proxy->service = NULL;
        }
        if (proxy != NULL) {
            free(proxy);
        }
    }

    return status;
}

static void importRegistration_proxyFunc(void *userData, void *args[], void *returnVal) {
    int  status = CELIX_SUCCESS;
    struct method_entry *entry = userData;
    import_registration_pt import = *((void **)args[0]);

    if (import == NULL || import->send == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }


    char *invokeRequest = NULL;
    if (status == CELIX_SUCCESS) {
        status = jsonSerializer_prepareInvokeRequest(entry->dynFunc, entry->id, args, &invokeRequest);
        printf("Need to send following json '%s'\n", invokeRequest);
    }

    if (status == CELIX_SUCCESS) {
        char *reply = NULL;
        int rc = 0;
        printf("sending request\n");
        import->send(import->sendHandle, import->endpoint, invokeRequest, &reply, &rc);
        printf("request sended. got reply '%s' with status %i\n", reply, rc);

        if (rc == 0) {
            printf("handling reply\n");
            status = jsonSerializer_handleReply(entry->dynFunc, reply, args);
        }

        *(int *) returnVal = rc;
    }

    if (status != CELIX_SUCCESS) {
        //TODO log error
    }
}

celix_status_t importRegistration_ungetService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **out) {
    celix_status_t  status = CELIX_SUCCESS;
    return status;

    /* TODO fix. gives segfault in framework shutdown (import->proxies == NULL)
    assert(import != NULL);
    assert(import->proxies != NULL);

    struct service_proxy *proxy = hashMap_get(import->proxies, bundle); //TODO lock
    if (proxy != NULL) {
        if (*out == proxy->service) {
            proxy->count -= 1;
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }

        if (proxy->count == 0) {
            importRegistration_destroyProxy(proxy);
        }
    }
     */

    return status;
}

static void importRegistration_destroyProxy(struct service_proxy *proxy) {
    //TODO
}


celix_status_t importRegistration_close(import_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t importRegistration_getException(import_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t importRegistration_getImportReference(import_registration_pt registration, import_reference_pt *reference) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t importReference_getImportedService(import_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}