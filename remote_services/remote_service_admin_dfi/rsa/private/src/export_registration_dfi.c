/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <jansson.h>
#include <dyn_interface.h>
#include <json_serializer.h>
#include <remote_constants.h>
#include <remote_service_admin.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>
#include <json_rpc.h>
#include "export_registration_dfi.h"

struct export_reference {
    endpoint_description_pt endpoint; //owner
    service_reference_pt reference;
};

struct export_registration {
    bundle_context_pt  context;
    struct export_reference exportReference;
    char *servId;
    dyn_interface_type *intf; //owner
    service_tracker_pt tracker;

    celix_thread_mutex_t mutex;
    void *service; //protected by mutex

    //TODO add tracker and lock
    bool closed;
};

static void exportRegistration_addServ(export_registration_pt reg, service_reference_pt ref, void *service);
static void exportRegistration_removeServ(export_registration_pt reg, service_reference_pt ref, void *service);

celix_status_t exportRegistration_create(log_helper_pt helper, service_reference_pt reference, endpoint_description_pt endpoint, bundle_context_pt context, export_registration_pt *out) {
    celix_status_t status = CELIX_SUCCESS;

    char *servId = NULL;
    status = serviceReference_getProperty(reference, "service.id", &servId);
    if (status != CELIX_SUCCESS) {
        logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "Cannot find service.id for ref");
    }

    export_registration_pt reg = NULL;
    if (status == CELIX_SUCCESS) {
        reg = calloc(1, sizeof(*reg));
        if (reg == NULL) {
            status = CELIX_ENOMEM;
        }
    }


    if (status == CELIX_SUCCESS) {
        reg->context = context;
        reg->exportReference.endpoint = endpoint;
        reg->exportReference.reference = reference;
        reg->closed = false;

        celixThreadMutex_create(&reg->mutex, NULL);
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
        service_tracker_customizer_pt cust = NULL;
        status = serviceTrackerCustomizer_create(reg, NULL, (void *) exportRegistration_addServ, NULL,
                                                 (void *) exportRegistration_removeServ, &cust);
        if (status == CELIX_SUCCESS) {
            char filter[32];
            snprintf(filter, 32, "(service.id=%s)", servId);
            status = serviceTracker_createWithFilter(reg->context, filter, cust, &reg->tracker);
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

    //printf("calling for '%s'\n");

    *responseLength = -1;
    celixThreadMutex_lock(&export->mutex);
    status = jsonRpc_call(export->intf, export->service, data, responseOut);
    celixThreadMutex_unlock(&export->mutex);

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
        if (reg->tracker != NULL) {
            serviceTracker_destroy(reg->tracker);
        }
        celixThreadMutex_destroy(&reg->mutex);

        free(reg);
    }
}

celix_status_t exportRegistration_start(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;

    serviceTracker_open(reg->tracker);
    return status;
}

static void exportRegistration_addServ(export_registration_pt reg, service_reference_pt ref, void *service) {
    celixThreadMutex_lock(&reg->mutex);
    reg->service = service;
    celixThreadMutex_unlock(&reg->mutex);
}

static void exportRegistration_removeServ(export_registration_pt reg, service_reference_pt ref, void *service) {
    celixThreadMutex_lock(&reg->mutex);
    if (reg->service == service) {
        reg->service = NULL;
    }
    celixThreadMutex_unlock(&reg->mutex);
}

celix_status_t exportRegistration_stop(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    status = bundleContext_ungetService(reg->context, reg->exportReference.reference, NULL);
    return status;
}

celix_status_t exportRegistration_close(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    exportRegistration_stop(reg);
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



