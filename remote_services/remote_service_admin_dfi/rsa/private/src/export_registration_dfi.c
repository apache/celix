/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <jansson.h>
#include <dyn_interface.h>
#include <json_serializer.h>
#include <remote_constants.h>
#include <remote_service_admin.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>
#include <json_rpc.h>
#include "constants.h"
#include "export_registration_dfi.h"
#include "dfi_utils.h"

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

    const char *servId = NULL;
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

    const char *exports = NULL;
    CELIX_DO_IF(status, serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports));

    bundle_pt bundle = NULL;
    CELIX_DO_IF(status, serviceReference_getBundle(reference, &bundle));

    FILE *descriptor = NULL;
    if (status == CELIX_SUCCESS) {
        status = dfi_findDescriptor(context, bundle, exports, &descriptor);
    }

    if (status != CELIX_SUCCESS || descriptor == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
        logHelper_log(helper, OSGI_LOGSERVICE_ERROR, "Cannot find/open descriptor for '%s'", exports);
    }

    if (status == CELIX_SUCCESS) {
        int rc = dynInterface_parse(descriptor, &reg->intf);
        fclose(descriptor);
        if (rc != 0) {
            status = CELIX_BUNDLE_EXCEPTION;
            logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "RSA: Error parsing service descriptor.");
        }
        else{
            /* Add the interface version as a property in the properties_map */
            char* intfVersion = NULL;
            dynInterface_getVersionString(reg->intf, &intfVersion);
            const char *serviceVersion = properties_get(endpoint->properties,(char*) CELIX_FRAMEWORK_SERVICE_VERSION);
            if(serviceVersion!=NULL){
            	if(strcmp(serviceVersion,intfVersion)!=0){
            		logHelper_log(helper, OSGI_LOGSERVICE_WARNING, "Service version (%s) and interface version from the descriptor (%s) are not the same!",serviceVersion,intfVersion);
            	}
            }
            else{
            	properties_set(endpoint->properties, (char*) CELIX_FRAMEWORK_SERVICE_VERSION, intfVersion);
            }
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
            endpoint_description_pt ep = reg->exportReference.endpoint;
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


celix_status_t exportRegistration_stop(export_registration_pt reg) {
    celix_status_t status = CELIX_SUCCESS;
    if (status == CELIX_SUCCESS) {
        status = bundleContext_ungetServiceReference(reg->context, reg->exportReference.reference);
        serviceTracker_close(reg->tracker);
    }
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

celix_status_t exportReference_getExportedService(export_reference_pt reference, service_reference_pt *ref) {
    celix_status_t status = CELIX_SUCCESS;
    *ref = reference->reference;
    return status;
}



