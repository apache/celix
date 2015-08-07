/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <jansson.h>
#include <dyn_interface.h>
#include <json_serializer.h>
#include "export_registration.h"
#include "export_registration_dfi.h"
#include "endpoint_description.h"

struct export_registration {
    endpoint_description_pt endpointDescription;
    service_reference_pt reference;
    dyn_interface_type *intf;
    void *service;
    bundle_pt bundle;

    bool closed;
};

struct export_reference {
    endpoint_description_pt endpoint;
    service_reference_pt reference;
};

typedef void (*GEN_FUNC_TYPE)(void);

struct generic_service_layout {
    void *handle;
    GEN_FUNC_TYPE methods[];
};

celix_status_t exportRegistration_create(log_helper_pt helper, service_reference_pt reference, endpoint_description_pt endpoint, bundle_context_pt context, export_registration_pt *registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_call(export_registration_pt export, char *data, int datalength, char **response, int *responseLength) {
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
    }

    if (method != NULL) {

        int nrOfArgs = dynFunction_nrOfArguments(method->dynFunc);
        void *args[nrOfArgs + 1]; //arg 0 is handle
        dyn_type *returnType = dynFunction_returnType(method->dynFunc);

        json_t *arguments = json_object_get(js_request, "a");
        json_t *value;
        size_t index;
        json_array_foreach(arguments, index, value) {
            dyn_type *argType = dynFunction_argumentTypeForIndex(method->dynFunc, index + 1);
            status = jsonSerializer_deserializeJson(argType, value, &(args[index + 1]));
            index += 1;
            if (status != 0) {
                break;
            }
        }

        json_decref(js_request);

        struct generic_service_layout *serv = export->service;
        args[0] = serv->handle;
        void *returnVal = NULL;
        dynType_alloc(returnType, &returnVal);
        dynFunction_call(method->dynFunc, serv->methods[method->index], returnVal, args);

        status = jsonSerializer_serialize(returnType, returnVal, response);
        if (returnVal != NULL) {
            dynType_free(returnType, returnVal);
        }

        ///TODO add more status checks
    }

    //TODO unlock/sema export
    return status;
}

celix_status_t exportRegistration_destroy(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_start(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_stop(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_close(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_getException(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *reference) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
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



