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

    *responseLength = -1;
    //TODO lock service
    status = jsonSerializer_call(export->intf, export->service, data, responseOut);
    //TODO unlock service

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



