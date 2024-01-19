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
#include "rsa_shm_impl.h"
#include "rsa_shm_server.h"
#include "rsa_shm_client.h"
#include "rsa_shm_constants.h"
#include "rsa_shm_export_registration.h"
#include "rsa_shm_import_registration.h"
#include "rsa_rpc_factory.h"
#include "rsa_request_sender_service.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_api.h"
#include "celix_long_hash_map.h"
#include "celix_array_list.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <assert.h>

struct rsa_shm {
    celix_bundle_context_t *context;
    celix_log_helper_t *logHelper;
    celix_thread_mutex_t exportedServicesLock;//It protects exportedServices
    celix_long_hash_map_t *exportedServices;// Key is service id, value is the list of exported registration.
    celix_thread_mutex_t importedServicesLock;// It protects importedServices
    celix_array_list_t *importedServices;
    rsa_shm_client_manager_t *shmClientManager;
    rsa_shm_server_t *shmServer;
    char *shmServerName;
    rsa_request_sender_service_t reqSenderService;
    long reqSenderSvcId;
};


static celix_status_t rsaShm_receiveMsgCB(void *handle, rsa_shm_server_t *shmServer,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

static celix_status_t rsaShm_createEndpointDescription(rsa_shm_t *admin,
        celix_properties_t *exportedProperties, char *interface, endpoint_description_t **description);

celix_status_t rsaShm_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper,
        rsa_shm_t **admin) {
    celix_status_t status = CELIX_SUCCESS;
    if (context == NULL ||  admin == NULL || logHelper == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree rsa_shm_t* ad = calloc(1, sizeof(*ad));
    if (ad == NULL) {
        return CELIX_ENOMEM;
    }

    ad->context = context;
    ad->logHelper = logHelper;
    ad->reqSenderSvcId = -1;
    status = celixThreadMutex_create(&ad->exportedServicesLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating mutex for exported service. %d", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) exportedServicesLock = &ad->exportedServicesLock;
    celix_autoptr(celix_long_hash_map_t) exportedServices = ad->exportedServices = celix_longHashMap_create();
    assert(ad->exportedServices);

    status = celixThreadMutex_create(&ad->importedServicesLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating mutex for imported service. %d", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) importedServicesLock = &ad->importedServicesLock;
    celix_autoptr(celix_array_list_t) importedServices = ad->importedServices = celix_arrayList_create();
    assert(ad->importedServices != NULL);

    status = rsaShmClientManager_create(context, logHelper, &ad->shmClientManager);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"Error creating shm client manager. %d", status);
        return status;
    }
    celix_autoptr(rsa_shm_client_manager_t) shmClientManager = ad->shmClientManager;

    ad->reqSenderService.handle = ad;
    ad->reqSenderService.sendRequest = (void*)rsaShm_send;
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_RSA_REQUEST_SENDER_SERVICE_NAME;
    opts.serviceVersion = CELIX_RSA_REQUEST_SENDER_SERVICE_VERSION;
    opts.svc = &ad->reqSenderService;
    ad->reqSenderSvcId = celix_bundleContext_registerServiceWithOptionsAsync(context, &opts);
    if (ad->reqSenderSvcId < 0) {
        celix_logHelper_error(logHelper,"Error registering request sender service.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_auto(celix_service_registration_guard_t) reg =
        celix_serviceRegistrationGuard_init(context, ad->reqSenderSvcId);

    const char *fwUuid = celix_bundleContext_getProperty(context, CELIX_FRAMEWORK_UUID, NULL);
    if (fwUuid == NULL) {
        celix_logHelper_error(logHelper,"Error Getting cfw uuid for shm rsa admin.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    long bundleId = celix_bundleContext_getBundleId(context);
    if (bundleId < 0) {
        celix_logHelper_error(logHelper,"Bundle id is invalid.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    int bytes = asprintf(&ad->shmServerName, "ShmServ_%s_%ld", fwUuid, bundleId);
    if (bytes < 0) {
        celix_logHelper_error(logHelper,"Failed to alloc memory for shm server name.");
        return CELIX_ENOMEM;
    }
    celix_autofree char* shmServerName = ad->shmServerName;
    status = rsaShmServer_create(context, ad->shmServerName,logHelper,
            rsaShm_receiveMsgCB, ad, &ad->shmServer);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper,"Error creating shm server. %d", status);
    } else {
        celix_steal_ptr(shmServerName);
        reg.svcId = -1;
        celix_steal_ptr(shmClientManager);
        celix_steal_ptr(importedServices);
        celix_steal_ptr(importedServicesLock);
        celix_steal_ptr(exportedServices);
        celix_steal_ptr(exportedServicesLock);
        *admin = celix_steal_ptr(ad);
    }
    return status;
}

void rsaShm_destroy(rsa_shm_t *admin) {
    rsaShmServer_destroy(admin->shmServer);
    free(admin->shmServerName);

    celix_bundleContext_unregisterService(admin->context, admin->reqSenderSvcId);

    rsaShmClientManager_destroy(admin->shmClientManager);

    // exported/imported services must be clear by topology manager before RSA destroy
    assert(celix_arrayList_size(admin->importedServices) == 0);
    celix_arrayList_destroy(admin->importedServices);
    celixThreadMutex_destroy(&admin->importedServicesLock);
    assert(celix_longHashMap_size(admin->exportedServices) == 0);
    celix_longHashMap_destroy(admin->exportedServices);
    celixThreadMutex_destroy(&admin->exportedServicesLock);
    free(admin);

    return;
}

static export_registration_t* rsaShm_getExportService(rsa_shm_t *admin, long serviceId) {
    celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&admin->exportedServicesLock);
    export_registration_t *export = NULL;
    celix_array_list_t *exports = celix_longHashMap_get(admin->exportedServices, serviceId);
    if (exports != NULL && celix_arrayList_size(exports) > 0) {
        export = celix_arrayList_get(exports, 0);
        if (export) {
            exportRegistration_addRef(export);
        }
    }
    return export;
}

static celix_status_t rsaShm_receiveMsgCB(void *handle, rsa_shm_server_t *shmServer,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (handle == NULL || shmServer == NULL || metadata == NULL || request == NULL || response == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    rsa_shm_t *admin = handle;

    long serviceId = celix_properties_getAsLong(metadata, CELIX_RSA_ENDPOINT_SERVICE_ID, -1);
    if (serviceId < 0) {
        celix_logHelper_error(admin->logHelper, "Service id is invalid.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autoptr(export_registration_t) export = rsaShm_getExportService(admin, serviceId);
    if (export == NULL) {
        celix_logHelper_error(admin->logHelper, "No export registration found for service id %ld", serviceId);
        return CELIX_ILLEGAL_STATE;
    }

    status = exportRegistration_call(export, metadata, request, response);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(admin->logHelper,"Export registration call service failed, error code is %d", status);
    }
    return status;
}

celix_status_t rsaShm_send(rsa_shm_t *admin, endpoint_description_t *endpoint,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || endpoint == NULL || request == NULL || response == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    const char *shmServerName = celix_properties_get(endpoint->properties, RSA_SHM_SERVER_NAME_KEY, NULL);
    if (shmServerName == NULL) {
        celix_logHelper_error(admin->logHelper,"RSA shm server name of %s is invalid.", endpoint->serviceName);
        return CELIX_SERVICE_EXCEPTION;
    }
    celix_autoptr(celix_properties_t) newMetadata = celix_properties_copy(metadata);
    celix_properties_setLong(newMetadata, CELIX_RSA_ENDPOINT_SERVICE_ID, endpoint->serviceId);
    status = rsaShmClientManager_sendMsgTo(admin->shmClientManager, shmServerName,
            (long)endpoint->serviceId, newMetadata, request, response);

    return status;
}

static void rsaShm_overlayProperties(celix_properties_t *additionalProperties, celix_properties_t *serviceProperties) {

    /*The property keys of a service are case-insensitive,while the property keys of the specified additional properties map are case sensitive.
     * A property key in the additional properties map must therefore override any case variant property key in the properties of the specified Service Reference.*/
    CELIX_PROPERTIES_ITERATE(additionalProperties, additionalPropIter) {
        if (strcmp(additionalPropIter.key,(char*) CELIX_FRAMEWORK_SERVICE_NAME) != 0
                && strcmp(additionalPropIter.key,(char*) CELIX_FRAMEWORK_SERVICE_ID) != 0) {
            bool propKeyCaseEqual = false;

            CELIX_PROPERTIES_ITERATE(serviceProperties, servicePropIter) {
                if (strcasecmp(additionalPropIter.key,servicePropIter.key) == 0) {
                    const char* val = additionalPropIter.entry.value;
                    celix_properties_set(serviceProperties,servicePropIter.key,val);
                    propKeyCaseEqual = true;
                    break;
                }
            }

            if (!propKeyCaseEqual) {
                const char* val = additionalPropIter.entry.value;
                celix_properties_set(serviceProperties,additionalPropIter.key,val);
            }
        }
    }
}

static bool rsaShm_isConfigTypeMatched(celix_properties_t *properties) {
    bool matched = false;
    /* If OSGI_RSA_SERVICE_EXPORTED_CONFIGS property is not set, then the Remote Service
     * Admin implementation must choose a convenient configuration type.
     */
    const char *exportConfigs = celix_properties_get(properties,
                                                     CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CONFIGURATION_TYPE);
    if (exportConfigs != NULL) {
        // See if the EXPORT_CONFIGS matches this RSA. If so, try to export.

        celix_autofree char *ecCopy = strndup(exportConfigs, strlen(exportConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            char *configType = celix_utils_trimInPlace(token);
            if (strncmp(configType, RSA_SHM_CONFIGURATION_TYPE, 1024) == 0) {
                matched = true;
                break;
            }
            token = strtok_r(NULL, delimiter, &savePtr);
        }
    }
    return matched;
}

celix_status_t rsaShm_exportService(rsa_shm_t *admin, char *serviceId,
        celix_properties_t *properties, celix_array_list_t **registrationsOut) {
    celix_status_t status = CELIX_SUCCESS;

    if (admin == NULL || serviceId == NULL || registrationsOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_array_list_t *references = NULL;
    service_reference_pt reference = NULL;
    char filter[32] = {0};// It is longer than the size of "service.id" + serviceId
    snprintf(filter, sizeof(filter), "(%s=%s)", (char *) CELIX_FRAMEWORK_SERVICE_ID, serviceId);
    status = bundleContext_getServiceReferences(admin->context, NULL, filter, &references);
    if (status != CELIX_SUCCESS) {
       celix_logHelper_error(admin->logHelper,"Error getting reference for service id %s.", serviceId);
       return status;
    }
    //We get reference with serviceId, so the size of references must be less than or equal to 1.
    reference = celix_arrayList_get(references, 0);
    celix_arrayList_destroy(references);
    if (reference == NULL) {
        celix_logHelper_error(admin->logHelper, "Expect a reference for service id %s.", serviceId);
        return CELIX_ILLEGAL_STATE;
    }
    celix_auto(celix_service_ref_guard_t) ref = celix_ServiceRefGuard_init(admin->context, reference);

    celix_autoptr(celix_properties_t) exportedProperties = celix_properties_create();
    if (exportedProperties == NULL) {
        celix_logHelper_error(admin->logHelper, "Error creating exported properties.");
        return CELIX_ENOMEM;
    }
    unsigned int propertySize = 0;
    char **keys = NULL;
    serviceReference_getPropertyKeys(reference, &keys, &propertySize);
    for (int i = 0; i < propertySize; i++) {
        char *key = keys[i];
        const char *value = NULL;
        if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS) {
            celix_properties_set(exportedProperties, key, value);
        }
    }
    free(keys);

    //Property in the additional properties overrides the Service Reference properties
    if (properties != NULL) {
        rsaShm_overlayProperties(properties,exportedProperties);
    }

    celix_autoptr(celix_array_list_t) registrations = NULL;
    if (rsaShm_isConfigTypeMatched(exportedProperties)) {
        const char *exportsProp = celix_properties_get(exportedProperties, (char *) CELIX_RSA_SERVICE_EXPORTED_INTERFACES, NULL);
        const char *providedProp = celix_properties_get(exportedProperties, (char *) CELIX_FRAMEWORK_SERVICE_NAME, NULL);
        if (exportsProp == NULL  || providedProp == NULL) {
            celix_logHelper_error(admin->logHelper, "Error exporting service %s. Missing property %s or %s.", serviceId, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, CELIX_FRAMEWORK_SERVICE_NAME);
            return CELIX_ILLEGAL_STATE;
        }
        celix_autofree char *exports = celix_utils_trim(exportsProp);
        celix_autofree char *provided = celix_utils_trim(providedProp);
        if (exports == NULL || provided == NULL) {
            celix_logHelper_error(admin->logHelper, "Failed to trim exported interface.");
            return CELIX_ENOMEM;
        }

        celix_logHelper_info(admin->logHelper, "Export services (%s)", exports);

        celix_autoptr(celix_array_list_t) interfaces = celix_arrayList_create();
        celix_autoptr(celix_array_list_t) proInterfaces = celix_arrayList_create();
        registrations = celix_arrayList_create();

        // Parse export interfaces for export service.
        if (strcmp(exports, "*") == 0) {
            char *provided_save_ptr = NULL;
            char *interface = strtok_r(provided, ",", &provided_save_ptr);
            while (interface != NULL) {
                celix_arrayList_add(interfaces, celix_utils_trimInPlace(interface));
                interface = strtok_r(NULL, ",", &provided_save_ptr);
            }
        } else {
            char *provided_save_ptr = NULL;
            char *pinterface = strtok_r(provided, ",", &provided_save_ptr);
            while (pinterface != NULL) {
                celix_arrayList_add(proInterfaces, celix_utils_trimInPlace(pinterface));
                pinterface = strtok_r(NULL, ",", &provided_save_ptr);
            }

            char *exports_save_ptr = NULL;
            char *einterface = strtok_r(exports, ",", &exports_save_ptr);
            while (einterface != NULL) {
                einterface = celix_utils_trimInPlace(einterface);
                for (int i = 0; i < celix_arrayList_size(proInterfaces); ++i) {
                    if (strcmp(celix_arrayList_get(proInterfaces, i), einterface) == 0) {
                        celix_arrayList_add(interfaces, einterface);
                        break;
                    }
                }
                einterface = strtok_r(NULL, ",", &exports_save_ptr);
            }
        }

        // Create export registrations for its interfaces.
        size_t interfaceNum = arrayList_size(interfaces);
        for (int iter = 0; iter < interfaceNum; iter++) {
            char *interface = arrayList_get(interfaces, iter);
            celix_autoptr(endpoint_description_t) endpointDescription = NULL;
            export_registration_t *registration = NULL;
            int ret = CELIX_SUCCESS;
            ret = rsaShm_createEndpointDescription(admin,
                    exportedProperties, interface, &endpointDescription);
            if (ret != CELIX_SUCCESS) {
                continue;
            }

            ret = exportRegistration_create(admin->context, admin->logHelper,
                    reference, endpointDescription, &registration);
            if (ret == CELIX_SUCCESS) {
                celix_arrayList_add(registrations, registration);
            }
        }
    }

    //We return a empty list of registrations if Remote Service Admin does not recognize any of the configuration types.
    celix_array_list_t *newRegistrations = celix_arrayList_create();
    int regSize = registrations != NULL ? celix_arrayList_size(registrations) : 0;
    if (regSize > 0) {
        for (int i = 0; i < regSize; ++i) {
            celix_arrayList_add(newRegistrations, celix_arrayList_get(registrations, i));
        }
        celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&admin->exportedServicesLock);
        celix_longHashMap_put(admin->exportedServices, atol(serviceId), celix_steal_ptr(registrations));
    }
    *registrationsOut = newRegistrations;

    return CELIX_SUCCESS;
}

celix_status_t rsaShm_removeExportedService(rsa_shm_t *admin, export_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    export_reference_t *ref = NULL;
    status = exportRegistration_getExportReference(registration, &ref);

    if (status == CELIX_SUCCESS) {
        endpoint_description_t *endpoint = NULL;
        celixThreadMutex_lock(&admin->exportedServicesLock);
        status = exportReference_getExportedEndpoint(ref, &endpoint);
        assert(status == CELIX_SUCCESS);
        celix_logHelper_info(admin->logHelper, "Remove exported service %s", endpoint->serviceName);
        celix_array_list_t *registrations = (celix_array_list_t *)celix_longHashMap_get(admin->exportedServices,
                (long)endpoint->serviceId);
        if (registrations != NULL) {
            celix_arrayList_remove(registrations, registration);
            if (celix_arrayList_size(registrations) == 0) {
                (void)celix_longHashMap_remove(admin->exportedServices, endpoint->serviceId);
                celix_arrayList_destroy(registrations);
            }
        }

        exportRegistration_release(registration);
        celixThreadMutex_unlock(&admin->exportedServicesLock);
    } else {
        celix_logHelper_error(admin->logHelper,"SHM RSA cannot find reference for registration.");
    }

    if(ref!=NULL){
        free(ref);
    }

    return status;
}

static celix_status_t rsaShm_createEndpointDescription(rsa_shm_t *admin,
        celix_properties_t *exportedProperties, char *interface, endpoint_description_t **description) {
    celix_status_t status = CELIX_SUCCESS;
    assert(admin != NULL);
    assert(exportedProperties != NULL);
    assert(interface != NULL);
    assert(description != NULL);

    celix_autoptr(celix_properties_t) endpointProperties = celix_properties_copy(exportedProperties);
    celix_properties_unset(endpointProperties,CELIX_FRAMEWORK_SERVICE_NAME);
    celix_properties_unset(endpointProperties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES);
    celix_properties_unset(endpointProperties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS);
    celix_properties_unset(endpointProperties, CELIX_FRAMEWORK_SERVICE_ID);

    long serviceId = celix_properties_getAsLong(exportedProperties, CELIX_FRAMEWORK_SERVICE_ID, -1);

    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);

    const char *uuid = celix_bundleContext_getProperty(admin->context, CELIX_FRAMEWORK_UUID, NULL);
    if (uuid == NULL) {
        celix_logHelper_error(admin->logHelper, "Cannot get framework uuid");
        return CELIX_FRAMEWORK_EXCEPTION;
    }
    celix_properties_set(endpointProperties, (char*) CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    celix_properties_set(endpointProperties, (char*) CELIX_FRAMEWORK_SERVICE_NAME, interface);
    celix_properties_setLong(endpointProperties, (char*) CELIX_RSA_ENDPOINT_SERVICE_ID, serviceId);
    celix_properties_set(endpointProperties, (char*) CELIX_RSA_ENDPOINT_ID, endpoint_uuid);
    celix_properties_set(endpointProperties, (char*) CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(endpointProperties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, RSA_SHM_CONFIGURATION_TYPE);
    //If the rpc type of RSA_SHM_CONFIGURATION_TYPE is not set, then set a default value.
    if (celix_properties_get(endpointProperties, RSA_SHM_RPC_TYPE_KEY, NULL) == NULL) {
        celix_properties_set(endpointProperties, RSA_SHM_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);
    }
    celix_properties_set(endpointProperties, (char *) RSA_SHM_SERVER_NAME_KEY, admin->shmServerName);
    status = endpointDescription_create(endpointProperties, description);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(admin->logHelper, "Cannot create endpoint description for service %s", interface);
        return status;
    }
    celix_steal_ptr(endpointProperties);
    return CELIX_SUCCESS;
}

//LCOV_EXCL_START
celix_status_t rsaShm_getExportedServices(rsa_shm_t *admin CELIX_UNUSED, array_list_pt *services CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t rsaShm_getImportedEndpoints(rsa_shm_t *admin CELIX_UNUSED, array_list_pt *services CELIX_UNUSED) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}
//LCOV_EXCL_STOP

celix_status_t rsaShm_importService(rsa_shm_t *admin, endpoint_description_t *endpointDesc,
        import_registration_t **registration) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || endpointDesc == NULL || registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    *registration = NULL;
    celix_logHelper_info(admin->logHelper, "Import service %s", endpointDesc->serviceName);

    bool importService = false;
    const char *importConfigs = celix_properties_get(endpointDesc->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (importConfigs != NULL) {
        // Check whether this RSA must be imported
        celix_autofree char *ecCopy = strndup(importConfigs, strlen(importConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            char *trimmedToken = celix_utils_trimInPlace(token);
            if (strcmp(trimmedToken, RSA_SHM_CONFIGURATION_TYPE) == 0) {
                importService = true;
                break;
            }
            token = strtok_r(NULL, delimiter, &savePtr);
        }
    } else {
        celix_logHelper_warning(admin->logHelper, "Mandatory %s element missing from endpoint description", CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
    }
    if (!importService) {
        return CELIX_SUCCESS;
    }

    import_registration_t *import = NULL;
    const char *shmServerName = NULL;

    shmServerName = celix_properties_get(endpointDesc->properties, RSA_SHM_SERVER_NAME_KEY, NULL);
    if (shmServerName == NULL) {
        celix_logHelper_error(admin->logHelper, "Get shm server name for service %s failed", endpointDesc->serviceName);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    status = rsaShmClientManager_createOrAttachClient(admin->shmClientManager,
                                                      shmServerName, (long)endpointDesc->serviceId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(admin->logHelper, "Error Creating shm client for service %s. %d", endpointDesc->serviceName, status);
        return status;
    }

    status = importRegistration_create(admin->context, admin->logHelper,
                                       endpointDesc, admin->reqSenderSvcId, &import);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(admin->logHelper, "Error Creating import registration for service %s. %d", endpointDesc->serviceName, status);
        rsaShmClientManager_destroyOrDetachClient(admin->shmClientManager, shmServerName,
                                                  (long)endpointDesc->serviceId);
        return status;
    }

    celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&admin->importedServicesLock);
    celix_arrayList_add(admin->importedServices, import);
    *registration = import;
    return CELIX_SUCCESS;
}

celix_status_t rsaShm_removeImportedService(rsa_shm_t *admin, import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    endpoint_description_t *endpoint = NULL;// Its owner is registration.
    status = importRegistration_getImportedEndpoint(registration, &endpoint);
    if (status == CELIX_SUCCESS) {
        celix_logHelper_info(admin->logHelper, "Remove imported service %s", endpoint->serviceName);
        const char *shmServerName = celix_properties_get(endpoint->properties,
                RSA_SHM_SERVER_NAME_KEY, NULL);
        if (shmServerName != NULL) {
            rsaShmClientManager_destroyOrDetachClient(admin->shmClientManager,
                    shmServerName, (long)endpoint->serviceId);
        } else {
            celix_logHelper_error(admin->logHelper, "Error getting shm server name for service %s. It maybe cause resource leaks.", endpoint->serviceName);
        }
    } else {
        celix_logHelper_error(admin->logHelper, "Error getting endpoint from imported registration for service %s. It maybe cause resource leaks.", endpoint->serviceName);
    }

    celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&admin->importedServicesLock);
    celix_arrayList_remove(admin->importedServices, registration);
    importRegistration_destroy(registration);

    return CELIX_SUCCESS;
}
