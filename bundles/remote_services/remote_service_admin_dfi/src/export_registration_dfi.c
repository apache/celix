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

    FILE *logFile;
};

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

        celixThreadMutex_create(&reg->mutex, NULL);
    }

    const char *exports = NULL;
    CELIX_DO_IF(status, serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports));

    celix_bundle_t *bundle = NULL;
    CELIX_DO_IF(status, serviceReference_getBundle(reference, &bundle));

    FILE *descriptor = NULL;
    if (status == CELIX_SUCCESS) {
        status = dfi_findAvprDescriptor(context, bundle, exports, &descriptor);
    }

    if (status != CELIX_SUCCESS || descriptor == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
        logHelper_log(helper, OSGI_LOGSERVICE_ERROR, "Cannot find/open descriptor for '%s'", exports);
    }

    if (status == CELIX_SUCCESS) {
        reg->intf = dynInterface_parseAvpr(descriptor);

        fclose(descriptor);
        if (!reg->intf) {
            status = CELIX_BUNDLE_EXCEPTION;
            logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA: Error parsing service descriptor for '%s'", exports);
        }
        else {
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

celix_status_t exportRegistration_call(export_registration_t *export, char *data, int datalength, char **responseOut, int *responseLength) {
    int status = CELIX_SUCCESS;

    *responseLength = -1;
    celixThreadMutex_lock(&export->mutex);
    status = jsonRpc_call(export->intf, export->service, data, responseOut);
    celixThreadMutex_unlock(&export->mutex);

    //printf("calling for '%s'\n");
    if (export->logFile != NULL) {
        static int callCount = 0;
        char *name = NULL;
        dynInterface_getName(export->intf, &name);
        fprintf(export->logFile, "REMOTE CALL %i\n\tservice=%s\n\tservice_id=%s\n\trequest_payload=%s\n\tstatus=%i\n", callCount, name, export->servId, data, status);
        fflush(export->logFile);
        callCount += 1;
    }

    return status;
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
