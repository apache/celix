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
#include <remote_constants.h>
#include <remote_service_admin.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>
#include <json_rpc.h>
#include "celix_constants.h"
#include "export_registration_dfi.h"
#include "dfi_utils.h"
#include "remote_interceptors_handler.h"

#include <string.h>

struct export_reference {
    endpoint_description_t *endpoint; //owner
    service_reference_pt reference;
};

struct export_registration {
    celix_log_helper_t* helper;
    celix_bundle_context_t * context;
    struct export_reference exportReference;
    char *servId;
    dyn_interface_type *intf; //owner
    char filter[32];


    celix_thread_mutex_t mutex;
    celix_thread_cond_t  cond;
    bool active; //protected by mutex
    void *service; //protected by mutex
    long trackerId; //protected by mutex
    int useCount; //protected by mutex

    //TODO add tracker and lock
    bool closed;

    remote_interceptors_handler_t *interceptorsHandler;

    FILE *logFile;
};

static celix_status_t exportRegistration_findAndParseInterfaceDescriptor(celix_log_helper_t *helper, celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out);
static void exportRegistration_addServ(void *data, void *service);
static void exportRegistration_removeServ(void *data, void *service);

celix_status_t exportRegistration_create(celix_log_helper_t *helper, service_reference_pt reference, endpoint_description_t *endpoint, celix_bundle_context_t *context, FILE *logFile, export_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;

    const char *servId = NULL;
    status = serviceReference_getProperty(reference, "service.id", &servId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_log(helper, CELIX_LOG_LEVEL_WARNING, "Cannot find service.id for ref");
    }

    export_registration_t *reg = NULL;
    if (status == CELIX_SUCCESS) {
        reg = calloc(1, sizeof(*reg));
        if (reg == NULL) {
            status = CELIX_ENOMEM;
        }
    }


    if (status == CELIX_SUCCESS) {
        reg->helper = helper;
        reg->context = context;
        reg->exportReference.endpoint = endpoint;
        reg->exportReference.reference = reference;
        reg->closed = false;
        reg->logFile = logFile;
        reg->servId = strndup(servId, 1024);
        reg->trackerId = -1L;
        reg->active = true;

        remoteInterceptorsHandler_create(context, &reg->interceptorsHandler);

        celixThreadMutex_create(&reg->mutex, NULL);
        celixThreadCondition_init(&reg->cond, NULL);
    }

    const char *exports = NULL;
    CELIX_DO_IF(status, serviceReference_getProperty(reference, (char *) CELIX_RSA_SERVICE_EXPORTED_INTERFACES, &exports));

    celix_bundle_t *bundle = NULL;
    CELIX_DO_IF(status, serviceReference_getBundle(reference, &bundle));

    if (status == CELIX_SUCCESS) {
        status = exportRegistration_findAndParseInterfaceDescriptor(helper, context, bundle, exports, &reg->intf);
    }

    if (status == CELIX_SUCCESS) {
        /* Add the interface version as a property in the properties_map */
        const char* intfVersion = dynInterface_getVersionString(reg->intf);
        const char *serviceVersion = celix_properties_get(endpoint->properties,(char*) CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
        if (serviceVersion != NULL) {
            if(strcmp(serviceVersion,intfVersion)!=0){
                celix_logHelper_log(helper, CELIX_LOG_LEVEL_WARNING, "Service version (%s) and interface version from the descriptor (%s) are not the same!",serviceVersion,intfVersion);
            }
        }
        else{
            celix_properties_set(endpoint->properties, (char*) CELIX_FRAMEWORK_SERVICE_VERSION, intfVersion);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = reg;
    } else {
        celix_logHelper_log(helper, CELIX_LOG_LEVEL_ERROR, "Error creating export registration");
        exportRegistration_destroy(reg);
    }

    return status;
}


void exportRegistration_increaseUsage(export_registration_t *export) {
    celixThreadMutex_lock(&export->mutex);
    export->useCount += 1;
    celixThreadMutex_unlock(&export->mutex);
}

void exportRegistration_decreaseUsage(export_registration_t *export) {
    celixThreadMutex_lock(&export->mutex);
    export->useCount -= 1;
    celixThreadCondition_broadcast(&export->cond);
    celixThreadMutex_unlock(&export->mutex);
}

void exportRegistration_waitTillNotUsed(export_registration_t *export) {
    celixThreadMutex_lock(&export->mutex);
    while (export->useCount > 0) {
        celixThreadCondition_wait(&export->cond, &export->mutex);
    }
    celixThreadMutex_unlock(&export->mutex);
}

celix_status_t exportRegistration_call(export_registration_t *export, char *data, int datalength, celix_properties_t **metadata, char **responseOut, int *responseLength) {
    int status = CELIX_SUCCESS;

    char* response = NULL;
    *responseLength = -1;
    json_error_t error;
    json_t *js_request = json_loads(data, 0, &error);
    const char *sig;
    if (js_request) {
        if (json_unpack(js_request, "{s:s}", "m", &sig) == 0) {
            bool cont = remoteInterceptorHandler_invokePreExportCall(export->interceptorsHandler, export->exportReference.endpoint->properties, sig, metadata);
            if (cont) {
                celixThreadMutex_lock(&export->mutex);
                if (export->active && export->service != NULL) {
                    int rc = jsonRpc_call(export->intf, export->service, data, &response);
                    status = (rc != 0) ? CELIX_SERVICE_EXCEPTION : CELIX_SUCCESS;
                    if (rc != 0) {
                        celix_logHelper_logTssErrors(export->helper, CELIX_LOG_LEVEL_ERROR);
                        celix_logHelper_error(export->helper, "Error calling remote service. Got error code %d", rc);
                    }
                } else if (!export->active) {
                    status = CELIX_ILLEGAL_STATE;
                    celix_logHelper_warning(export->helper, "Cannot call an inactive service export");
                } else {
                    status = CELIX_ILLEGAL_STATE;
                    celix_logHelper_error(export->helper, "export service pointer is NULL");
                }
                celixThreadMutex_unlock(&export->mutex);

                remoteInterceptorHandler_invokePostExportCall(export->interceptorsHandler, export->exportReference.endpoint->properties, sig, *metadata);
            }
            *responseOut = response;

            //printf("calling for '%s'\n");
            if (export->logFile != NULL) {
                static int callCount = 0;
                const char *name = dynInterface_getName(export->intf);
                fprintf(export->logFile, "REMOTE CALL %i\n\tservice=%s\n\tservice_id=%s\n\trequest_payload=%s\n\trequest_response=%s\n\tstatus=%i\n", callCount, name, export->servId, data, response, status);
                fflush(export->logFile);
                callCount += 1;
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    json_decref(js_request);

    return status;
}

static celix_status_t exportRegistration_findAndParseInterfaceDescriptor(celix_log_helper_t *helper, celix_bundle_context_t * const context, celix_bundle_t * const bundle, char const * const name, dyn_interface_type **out) {
    FILE* descriptor = NULL;

    celix_status_t status = dfi_findDescriptor(context, bundle, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        int rc = dynInterface_parse(descriptor, out);
        fclose(descriptor);
        if (rc != 0) {
            celix_logHelper_logTssErrors(helper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(helper, "RSA_DFI: Error parsing service descriptor for \"%s\", return code is %d.", name, rc);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    celix_logHelper_log(helper, CELIX_LOG_LEVEL_WARNING, "RSA: Error finding service descriptor for '%s'", name);
    return CELIX_BUNDLE_EXCEPTION;
}

static void exportRegistration_destroyCallback(void* data) {
    export_registration_t* reg = data;
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
    if (reg->servId != NULL) {
        free(reg->servId);
    }
    remoteInterceptorsHandler_destroy(reg->interceptorsHandler);
    bundleContext_ungetServiceReference(reg->context, reg->exportReference.reference);

    celixThreadMutex_destroy(&reg->mutex);
    celixThreadCondition_destroy(&reg->cond);
    free(reg);
}

void exportRegistration_destroy(export_registration_t *reg) {
    if (reg != NULL) {
        if (reg->trackerId >= 0) {
            celix_bundleContext_stopTrackerAsync(reg->context, reg->trackerId, reg, exportRegistration_destroyCallback);
        } else {
            exportRegistration_destroyCallback(reg);
        }
    }
}

celix_status_t exportRegistration_start(export_registration_t *reg) {
    celix_status_t status = CELIX_SUCCESS;

    snprintf(reg->filter, 32, "(service.id=%s)", reg->servId);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.filter = reg->filter;
    opts.filter.serviceName = "*";
    opts.callbackHandle = reg;
    opts.add = exportRegistration_addServ;
    opts.remove = exportRegistration_removeServ;
    long newTrkId = celix_bundleContext_trackServicesWithOptionsAsync(reg->context, &opts);

    celixThreadMutex_lock(&reg->mutex);
    long prevTrkId = reg->trackerId;
    reg->trackerId = newTrkId;
    celixThreadMutex_unlock(&reg->mutex);

    if (prevTrkId >= 0) {
        celix_logHelper_error(reg->helper, "Error starting export registration. The export registration already had an active service tracker");
        celix_bundleContext_stopTrackerAsync(reg->context, prevTrkId, NULL, NULL);
    }

    return status;
}

void exportRegistration_setActive(export_registration_t *reg, bool active) {
    celixThreadMutex_lock(&reg->mutex);
    reg->active = active;
    celixThreadMutex_unlock(&reg->mutex);
}

static void exportRegistration_addServ(void *data, void *service) {
    export_registration_t *reg = data;
    celixThreadMutex_lock(&reg->mutex);
    reg->service = service;
    celixThreadMutex_unlock(&reg->mutex);
}

static void exportRegistration_removeServ(void *data, void *service) {
    export_registration_t *reg = data;
    celixThreadMutex_lock(&reg->mutex);
    if (reg->service == service) {
        reg->service = NULL;
    }
    celixThreadMutex_unlock(&reg->mutex);
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
