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

#include <jansson.h>
#include <dyn_interface.h>
#include <json_serializer.h>
#include <remote_constants.h>
#include <remote_service_admin.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>
#include <json_rpc.h>
#include "celix_constants.h"
#include "export_registration_dfi.h"
#include "dfi_utils.h"
#include "remote_interceptors_handler.h"

struct export_reference {
    endpoint_description_t *endpoint; //owner
    service_reference_pt reference;
};

struct export_registration {
    celix_bundle_context_t * context;
    struct export_reference exportReference;
    char *servId;
    dyn_interface_type *intf; //owner
    service_tracker_t *tracker;

    celix_thread_mutex_t mutex;
    void *service; //protected by mutex

    //TODO add tracker and lock
    bool closed;

    remote_interceptors_handler_t *interceptorsHandler;

    FILE *logFile;
};

static celix_status_t exportRegistration_findAndParseInterfaceDescriptor(log_helper_t *helper, celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out);
static void exportRegistration_addServ(export_registration_t *reg, service_reference_pt ref, void *service);
static void exportRegistration_removeServ(export_registration_t *reg, service_reference_pt ref, void *service);

celix_status_t exportRegistration_create(log_helper_t *helper, service_reference_pt reference, endpoint_description_t *endpoint, celix_bundle_context_t *context, FILE *logFile, export_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;

    const char *servId = NULL;
    status = serviceReference_getProperty(reference, "service.id", &servId);
    if (status != CELIX_SUCCESS) {
        logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "Cannot find service.id for ref");
    }

    export_registration_t *reg = NULL;
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
        reg->logFile = logFile;
        reg->servId = strndup(servId, 1024);

        remoteInterceptorsHandler_create(context, &reg->interceptorsHandler);

        celixThreadMutex_create(&reg->mutex, NULL);
    }

    const char *exports = NULL;
    CELIX_DO_IF(status, serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports));

    celix_bundle_t *bundle = NULL;
    CELIX_DO_IF(status, serviceReference_getBundle(reference, &bundle));

    if (status == CELIX_SUCCESS) {
        status = exportRegistration_findAndParseInterfaceDescriptor(helper, context, bundle, exports, &reg->intf);
    }

    if (status == CELIX_SUCCESS) {
        /* Add the interface version as a property in the properties_map */
        char* intfVersion = NULL;
        dynInterface_getVersionString(reg->intf, &intfVersion);
        const char *serviceVersion = celix_properties_get(endpoint->properties,(char*) CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
        if (serviceVersion != NULL) {
            if(strcmp(serviceVersion,intfVersion)!=0){
                logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "Service version (%s) and interface version from the descriptor (%s) are not the same!",serviceVersion,intfVersion);
            }
        }
        else{
            celix_properties_set(endpoint->properties, (char*) CELIX_FRAMEWORK_SERVICE_VERSION, intfVersion);
        }
    }

    if (status == CELIX_SUCCESS) {
        service_tracker_customizer_t *cust = NULL;
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

celix_status_t exportRegistration_call(export_registration_t *export, char *data, int datalength, celix_properties_t *metadata, char **responseOut, int *responseLength) {
    int status = CELIX_SUCCESS;

    *responseLength = -1;
    json_error_t error;
    json_t *js_request = json_loads(data, 0, &error);
    const char *sig;
    if (js_request) {
        if (json_unpack(js_request, "{s:s}", "m", &sig) == 0) {
            bool cont = remoteInterceptorHandler_invokePreExportCall(export->interceptorsHandler, "TODO", export->exportReference.endpoint->properties, sig, &metadata);
            if (cont) {
                celixThreadMutex_lock(&export->mutex);
                status = jsonRpc_call(export->intf, export->service, data, responseOut);
                celixThreadMutex_unlock(&export->mutex);

                remoteInterceptorHandler_invokePostExportCall(export->interceptorsHandler, "TODO", export->exportReference.endpoint->properties, sig, metadata);
            }

            //printf("calling for '%s'\n");
            if (export->logFile != NULL) {
                static int callCount = 0;
                char *name = NULL;
                dynInterface_getName(export->intf, &name);
                fprintf(export->logFile, "REMOTE CALL %i\n\tservice=%s\n\tservice_id=%s\n\trequest_payload=%s\n\tstatus=%i\n", callCount, name, export->servId, data, status);
                fflush(export->logFile);
                callCount += 1;
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

static celix_status_t exportRegistration_findAndParseInterfaceDescriptor(log_helper_t *helper, celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out) {
    FILE* descriptor = NULL;

    celix_status_t status = dfi_findDescriptor(context, bundle, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        int rc = dynInterface_parse(descriptor, out);
        fclose(descriptor);
        if (rc != 0) {
            logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA_DFI: Error parsing service descriptor for \"%s\", return code is %d.", name, rc);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    status = dfi_findAvprDescriptor(context, bundle, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        *out = dynInterface_parseAvpr(descriptor);
        if (*out == NULL) {
            logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA_AVPR: Error parsing avpr service descriptor for '%s'", name);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA: Error finding service descriptor for '%s'", name);
    return CELIX_BUNDLE_EXCEPTION;
}

void exportRegistration_destroy(export_registration_t *reg) {
    if (reg != NULL) {
        if (reg->intf != NULL) {
            dyn_interface_type *intf = reg->intf;
            reg->intf = NULL;
            dynInterface_destroy(intf);
        }

        if (reg->exportReference.endpoint != NULL) {
            endpoint_description_t *ep = reg->exportReference.endpoint;
            reg->exportReference.endpoint = NULL;
            endpointDescription_destroy(ep);
        }
        if (reg->tracker != NULL) {
            serviceTracker_destroy(reg->tracker);
        }
        if (reg->servId != NULL) {
            free(reg->servId);
        }

        remoteInterceptorsHandler_destroy(reg->interceptorsHandler);

        celixThreadMutex_destroy(&reg->mutex);

        free(reg);
    }
}

celix_status_t exportRegistration_start(export_registration_t *reg) {
    celix_status_t status = CELIX_SUCCESS;

    serviceTracker_open(reg->tracker);
    return status;
}


celix_status_t exportRegistration_stop(export_registration_t *reg) {
    celix_status_t status = CELIX_SUCCESS;
    if (status == CELIX_SUCCESS) {
        status = bundleContext_ungetServiceReference(reg->context, reg->exportReference.reference);
        serviceTracker_close(reg->tracker);
    }
    return status;
}

static void exportRegistration_addServ(export_registration_t *reg, service_reference_pt ref, void *service) {
    celixThreadMutex_lock(&reg->mutex);
    reg->service = service;
    celixThreadMutex_unlock(&reg->mutex);
}

static void exportRegistration_removeServ(export_registration_t *reg, service_reference_pt ref, void *service) {
    celixThreadMutex_lock(&reg->mutex);
    if (reg->service == service) {
        reg->service = NULL;
    }
    celixThreadMutex_unlock(&reg->mutex);
}


celix_status_t exportRegistration_close(export_registration_t *reg) {
    celix_status_t status = CELIX_SUCCESS;
    exportRegistration_stop(reg);
    return status;
}


celix_status_t exportRegistration_getException(export_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    return status;
}

celix_status_t exportRegistration_getExportReference(export_registration_t *registration, export_reference_t **out) {
    celix_status_t status = CELIX_SUCCESS;
    export_reference_t *ref = calloc(1, sizeof(*ref));
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

celix_status_t exportReference_getExportedEndpoint(export_reference_t *reference, endpoint_description_t **endpoint) {
    celix_status_t status = CELIX_SUCCESS;
    *endpoint = reference->endpoint;
    return status;
}

celix_status_t exportReference_getExportedService(export_reference_t *reference, service_reference_pt *ref) {
    celix_status_t status = CELIX_SUCCESS;
    *ref = reference->reference;
    return status;
}
