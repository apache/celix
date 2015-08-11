/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <jansson.h>
#include <dyn_interface.h>
#include <json_serializer.h>
#include <remote_constants.h>
#include "export_registration.h"
#include "export_registration_dfi.h"

struct export_reference {
    endpoint_description_pt endpoint; //owner
    service_reference_pt reference;
};

struct export_registration {
    bundle_context_pt  context;
    struct export_reference exportReference;
    void *service;
    dyn_interface_type *intf; //owner

    //TODO add tracker and lock
    bool closed;
};

typedef void (*gen_func_type)(void);

struct generic_service_layout {
    void *handle;
    gen_func_type methods[];
};

celix_status_t exportRegistration_create(log_helper_pt helper, service_reference_pt reference, endpoint_description_pt endpoint, bundle_context_pt context, export_registration_pt *out) {
    celix_status_t status = CELIX_SUCCESS;

    export_registration_pt reg = calloc(1, sizeof(*reg));

    if (reg == NULL) {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        reg->context = context;
        reg->exportReference.endpoint = endpoint;
        reg->exportReference.reference = reference;
        reg->closed = false;
    }

    char *exports = NULL;
    CELIX_DO_IF(status, serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports));

    bundle_pt bundle = NULL;
    CELIX_DO_IF(status, serviceReference_getBundle(reference, &bundle));


    char *descriptorFile = NULL;
    if (status == CELIX_SUCCESS) {
        char name[128];
        snprintf(name, 128, "%s.descriptor", exports);
        status = bundle_getEntry(bundle, name, &descriptorFile);
        logHelper_log(helper, OSGI_LOGSERVICE_DEBUG, "RSA: Found descriptor '%s' for %'s'.", descriptorFile, exports);
    }

    if (descriptorFile == NULL) {
        logHelper_log(helper, OSGI_LOGSERVICE_ERROR, "RSA: Cannot find descrriptor in bundle for service '%s'", exports);
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        FILE *df = fopen(descriptorFile, "r");
        if (df != NULL) {
            int rc = dynInterface_parse(df, &reg->intf);
            fclose(df);
            if (rc != 0) {
                status = CELIX_BUNDLE_EXCEPTION;
                logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA: Error parsing service descriptor.");
            }
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            logHelper_log(helper, OSGI_LOGSERVICE_ERROR, "Cannot open descriptor '%s'", descriptorFile);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = reg;
    } else {
        logHelper_log(helper, OSGI_LOGSERVICE_ERROR, "Error creating export registration");
        exportRegistration_destroy(reg);
    }

    return status;
}

celix_status_t exportRegistration_call(export_registration_pt export, char *data, int datalength, char **responseOut, int *responseLength) {
    int status = CELIX_SUCCESS;
    //TODO lock/sema export

    printf("Parsing data: %s\n", data);
    json_error_t error;
    json_t *js_request = json_loads(data, 0, &error);
    const char *sig;
    if (js_request) {
        if (json_unpack(js_request, "{s:s}", "m", &sig) != 0) {
            printf("RSA: Got error '%s'\n", error.text);
        }
    } else {
        printf("RSA: got error '%s' for '%s'\n", error.text, data);
        return 0;
    }

    printf("RSA: Looking for method %s\n", sig);

    struct methods_head *methods = NULL;
    dynInterface_methods(export->intf, &methods);
    struct method_entry *entry = NULL;
    struct method_entry *method = NULL;
    TAILQ_FOREACH(entry, methods, entries) {
        if (strcmp(sig, entry->id) == 0) {
            method = entry;
            break;
        }
    }

    if (method == NULL) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        printf("RSA: found method '%s'\n", entry->id);
    }

    if (method != NULL) {

        int nrOfArgs = dynFunction_nrOfArguments(method->dynFunc);
        void *args[nrOfArgs]; //arg 0 is handle

        json_t *arguments = json_object_get(js_request, "a");
        json_t *value = NULL;
        int index = -1;
        json_array_foreach(arguments, index, value) {
            int argNr = index + 1;
            if (argNr < nrOfArgs -1 ) { //note skip last argument. this is the output
                dyn_type *argType = dynFunction_argumentTypeForIndex(method->dynFunc, argNr);
                status = jsonSerializer_deserializeJson(argType, value, &(args[argNr]));
            } else {
                status = CELIX_ILLEGAL_ARGUMENT;
            }
            if (status != 0) {
                break;
            }
        }

        json_decref(js_request);


        struct generic_service_layout *serv = export->service;
        args[0] = &serv->handle;

        //TODO assert last is output pointer (e.g. double pointer)
        dyn_type *lastTypePtr = dynFunction_argumentTypeForIndex(method->dynFunc, nrOfArgs-1);
        dyn_type *lastType = NULL;
        dynType_typedPointer_getTypedType(lastTypePtr, &lastType);


        void *out = NULL;
        dynType_alloc(lastType, &out); //TODO, NOTE only for simple types or single pointer types.. TODO check

        //NOTE !! Need to access out, else it is a dummy pointer which will result in errors.
        char b;
        memcpy(&b, out, 1);

        dynType_alloc(lastType, &out); //TODO, NOTE only for simple types or single pointer types.. TODO check
        printf("out ptr is %p value is %f\n", out, *(double *)out);
        args[nrOfArgs-1] = &out; //NOTE for simple type no double

        printf("args is %p %p %p\n", args[0] , args[1], args[2]);
        printf("args derefs is %p %p %p\n", *(void **)args[0], *(void **)args[1], *(void **)args[2]);

        //TODO assert return type is native int
        int returnVal = 0;
        //printf("calling function '%s', with index %i, nrOfArgs %i and at loc %p\n", method->id, method->index, nrOfArgs, serv->methods[method->index]);
        dynFunction_call(method->dynFunc, serv->methods[method->index], (void *)&returnVal, args);
        //printf("done calling\n");
        //printf("args is %p %p %p\n", args[0] , args[1], args[2]);
        //printf("args derefs is %p %p %p\n", *(void **)args[0], *(void **)args[1], *(void **)args[2]);
        //printf("out is %p and val is %f\n", out, *(double *)out);

        json_t *responseJson = NULL;
        //double r = 2.0;
        //status = jsonSerializer_serializeJson(lastOutputType, &r /*out*/, &responseJson);
        printf("out ptr is %p, value is %f\n", out, *(double *)out);
        status = jsonSerializer_serializeJson(lastType, out, &responseJson);

        json_t *payload = json_object();
        json_object_set_new(payload, "r", responseJson);

        char *response = json_dumps(payload, JSON_DECODE_ANY);
        json_decref(payload);


        *responseOut = response;
        *responseLength = -1;

        //TODO free args (created by jsonSerializer and dynType_alloc) (dynType_free)

        ///TODO add more status checks
    }

    //TODO unlock/sema export
    printf("done export reg call\n");
    return status;
}

void exportRegistration_destroy(export_registration_pt reg) {
    if (reg != NULL) {
        if (reg->intf != NULL) {
            dyn_interface_type *intf = reg->intf;
            reg->intf = NULL;
            dynInterface_destroy(intf);
        }

        if (reg->exportReference.endpoint != NULL) {
            endpoint_description_pt  ep = reg->exportReference.endpoint;
            reg->exportReference.endpoint = NULL;
            endpointDescription_destroy(ep);
        }

        free(reg);
    }
}

celix_status_t exportRegistration_start(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    status = bundleContext_getService(reg->context, reg->exportReference.reference, &reg->service); //TODO use tracker
    return status;
}

celix_status_t exportRegistration_stop(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    status = bundleContext_ungetService(reg->context, reg->exportReference.reference, NULL);
    return status;
}

celix_status_t exportRegistration_close(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    exportRegistration_stop(reg);
    //TODO callback to rsa to remove from list
    return status;
}

celix_status_t exportRegistration_getException(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    export_reference_pt ref = calloc(1, sizeof(*ref));
    if (ref != NULL) {
        ref->endpoint = registration->exportReference.endpoint;
        ref->reference = registration->exportReference.reference;
    } else {
        status = CELIX_ENOMEM;
        //TODO log
    }

    if (status == CELIX_SUCCESS) {
        *out = ref;
    }

    return status;
}

celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint) {
    celix_status_t status = CELIX_SUCCESS;
    *endpoint = reference->endpoint;
    return status;
}

celix_status_t exportReference_getExportedService(export_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}



