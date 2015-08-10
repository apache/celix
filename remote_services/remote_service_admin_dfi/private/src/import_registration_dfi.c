#include <stdlib.h>
#include "dyn_interface.h"
#include "import_registration.h"
#include "import_registration_dfi.h"

struct import_registration {
    bundle_context_pt context;
    endpoint_description_pt  endpoint; //TODO owner? -> free when destroyed
    const char *classObject; //NOTE owned by endpoint

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
        *out = reg;
    }

    return status;
}

void importRegistration_destroy(import_registration_pt import) {
    if (import != NULL) {
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
    if (status != CELIX_SUCCESS) {
        printf("Cannot find entry '%s'\n", name);
    }

    struct service_proxy *proxy = NULL;
    if (status == CELIX_SUCCESS) {
        proxy = calloc(1, sizeof(*proxy));
        if (proxy != NULL) {

        } else {
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

    void **serv = NULL;
    if (status == CELIX_SUCCESS) {
        size_t count = dynInterface_nrOfMethods(proxy->intf);
        serv = calloc(1 + count, sizeof(void *));
        serv[0] = proxy;

        struct methods_head *list = NULL;
        dynInterface_methods(proxy->intf, &list);
        struct method_entry *entry = NULL;
        void (*fn)(void);
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
        proxy->service = serv;
    } else {
        if (serv != NULL) {
            free(serv);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = proxy;
    }

    return status;
}

static void importRegistration_proxyFunc(void *userData, void *args[], void *returnVal) {
    struct method_entry *entry = userData;
    //struct proxy_service *proxy = args[0];

    printf("Calling function '%s'\n", entry->id);

    //TODO
}

celix_status_t importRegistration_ungetService(import_registration_pt import, bundle_pt bundle, service_registration_pt registration, void **out) {
    celix_status_t  status = CELIX_SUCCESS;
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