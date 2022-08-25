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
#include <rsa_shm_impl.h>
#include <rsa_shm_server.h>
#include <rsa_shm_client.h>
#include <rsa_shm_constants.h>
#include <rsa_shm_export_registration.h>
#include <rsa_shm_import_registration.h>
#include <rsa_rpc_factory.h>
#include <endpoint_description.h>
#include <remote_constants.h>
#include <celix_api.h>
#include <celix_long_hash_map.h>
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

    *admin = calloc(1, sizeof(**admin));
    assert(*admin !=  NULL);

    (*admin)->context = context;
    (*admin)->logHelper = logHelper;
    (*admin)->reqSenderSvcId = -1;
    status = celixThreadMutex_create(&(*admin)->exportedServicesLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error((*admin)->logHelper, "Error creating mutex for exported service. %d", status);
        goto exported_svc_lock_err;
    }
    (*admin)->exportedServices = celix_longHashMap_create();
    assert((*admin)->exportedServices);

    status = celixThreadMutex_create(&(*admin)->importedServicesLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error((*admin)->logHelper, "Error creating mutex for imported service. %d", status);
        goto imported_svc_lock_err;
    }
    (*admin)->importedServices = celix_arrayList_create();
    assert((*admin)->importedServices != NULL);

    status = rsaShmClientManager_create(context, (*admin)->logHelper, &(*admin)->shmClientManager);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error((*admin)->logHelper,"Error creating shm client manager. %d", status);
        goto shm_client_manager_err;
    }

    const char *fwUuid = NULL;
    bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUuid);
    if (fwUuid == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
        celix_logHelper_error((*admin)->logHelper,"Error Getting fw uuid for shm rsa admin.");
        goto fw_uuid_err;
    }
    long bundleId = celix_bundleContext_getBundleId(context);
    if (bundleId < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
        celix_logHelper_error((*admin)->logHelper,"Bundle id is invalid.");
        goto bundle_id_err;
    }
    asprintf(&(*admin)->shmServerName, "ShmServ_%s_%ld", fwUuid, bundleId);
    assert((*admin)->shmServerName != NULL);
    status = rsaShmServer_create(context, (*admin)->shmServerName,(*admin)->logHelper,
            rsaShm_receiveMsgCB, *admin, &(*admin)->shmServer);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error((*admin)->logHelper,"Error creating shm server. %d", status);
        goto shm_server_err;
    }

    return CELIX_SUCCESS;
shm_server_err:
    free((*admin)->shmServerName);
bundle_id_err:
fw_uuid_err:
    rsaShmClientManager_destory((*admin)->shmClientManager);
shm_client_manager_err:
    celix_arrayList_destroy((*admin)->importedServices);
    celixThreadMutex_destroy(&(*admin)->importedServicesLock);
imported_svc_lock_err:
    celix_longHashMap_destroy((*admin)->exportedServices);
    celixThreadMutex_destroy(&(*admin)->exportedServicesLock);
exported_svc_lock_err:
    free(*admin);
    return status;
}

void rsaShm_setRequestSenderSvcId(rsa_shm_t *admin, long reqSenderSvcId) {
    admin->reqSenderSvcId = reqSenderSvcId;
    return;
}

void rsaShm_destroy(rsa_shm_t *admin) {
    rsaShmServer_destroy(admin->shmServer);
    free(admin->shmServerName);

    rsaShmClientManager_destory(admin->shmClientManager);

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

static celix_status_t rsaShm_receiveMsgCB(void *handle, rsa_shm_server_t *shmServer,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (handle == NULL || shmServer == NULL || request == NULL || response == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    rsa_shm_t *admin = handle;

    long serviceId = celix_properties_getAsLong(metadata, OSGI_RSA_ENDPOINT_SERVICE_ID, -1);
    if (serviceId < 0) {
        celix_logHelper_error(admin->logHelper, "Service id is invalid.");
        status = CELIX_ILLEGAL_ARGUMENT;
        goto err_getting_service_id;
    }

    celixThreadMutex_lock(&admin->exportedServicesLock);

    //find exported registration
    celix_array_list_t *exports = celix_longHashMap_get(admin->exportedServices, serviceId);
    if (exports == NULL || celix_arrayList_size(exports) <= 0) {
        status = CELIX_ILLEGAL_STATE;
        celix_logHelper_error(admin->logHelper, "No export registration found for service id %ld", serviceId);
        goto err_getting_exports;
    }
    export_registration_t *export = celix_arrayList_get(exports, 0);
    if (export == NULL) {
        celix_logHelper_error(admin->logHelper, "Error getting registration for service id %ld", serviceId);
        status = CELIX_ILLEGAL_STATE;
        goto not_found_export;
    }
    // Add a reference to exported registration, avoid it is released by rsaShm_removeExportedService
    exportRegistration_addRef(export);

    celixThreadMutex_unlock(&admin->exportedServicesLock);

    status = exportRegistration_call(export, &metadata, request, response);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(admin->logHelper,"Export registration call service failed, error code is %d", status);
    }

    exportRegistration_release(export);

    return status;

not_found_export:
err_getting_exports:
    celixThreadMutex_unlock(&admin->exportedServicesLock);
err_getting_service_id:
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
        celix_logHelper_error(admin->logHelper,"RSA shm server name of %s is invalid.", endpoint->service);
        status = CELIX_SERVICE_EXCEPTION;
    }
    celix_properties_t * newMetadata = celix_properties_copy(metadata);
    celix_properties_setLong(newMetadata,OSGI_RSA_ENDPOINT_SERVICE_ID, endpoint->serviceId);
    status = rsaShmClientManager_sendMsgTo(admin->shmClientManager, shmServerName,
            (long)endpoint->serviceId, newMetadata, request, response);
    celix_properties_destroy(newMetadata);

    return status;
}

static void rsaShm_overlayProperties(celix_properties_t *additionalProperties, celix_properties_t *serviceProperties) {

    /*The property keys of a service are case-insensitive,while the property keys of the specified additional properties map are case sensitive.
     * A property key in the additional properties map must therefore override any case variant property key in the properties of the specified Service Reference.*/
    const char *additionalPropKey = NULL;
    const char *servicePropKey = NULL;
    PROPERTIES_FOR_EACH(additionalProperties, additionalPropKey) {
        if (strcmp(additionalPropKey,(char*) OSGI_FRAMEWORK_OBJECTCLASS) != 0
                && strcmp(additionalPropKey,(char*) OSGI_FRAMEWORK_SERVICE_ID) != 0) {
            bool propKeyCaseEqual = false;

            PROPERTIES_FOR_EACH(serviceProperties, servicePropKey) {
                if (strcasecmp(additionalPropKey,servicePropKey) == 0) {
                    const char* val = celix_properties_get(additionalProperties,additionalPropKey,NULL);
                    celix_properties_set(serviceProperties,servicePropKey,val);
                    propKeyCaseEqual = true;
                    break;
                }
            }

            if (!propKeyCaseEqual) {
                const char* val = celix_properties_get(additionalProperties,additionalPropKey,NULL);
                celix_properties_set(serviceProperties,additionalPropKey,val);
            }
        }
    }
    return;
}

static bool rsaShm_isConfigTypeMatched(celix_properties_t *properties) {
    bool matched = false;
    /* If OSGI_RSA_SERVICE_EXPORTED_CONFIGS property is not set, then the Remote Service
     * Admin implementation must choose a convenient configuration type.
     */
    const char *exportConfigs = celix_properties_get(properties,
            OSGI_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CONFIGURATION_TYPE);
    if (exportConfigs != NULL) {
        // See if the EXPORT_CONFIGS matches this RSA. If so, try to export.

        char *ecCopy = strndup(exportConfigs, strlen(exportConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(utils_stringTrim(token), RSA_SHM_CONFIGURATION_TYPE, 1024) == 0) {
                matched = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    }
    return matched;
}

celix_status_t rsaShm_exportService(rsa_shm_t *admin, char *serviceId,
        celix_properties_t *properties, celix_array_list_t **registrationsOut) {
    celix_status_t status = CELIX_SUCCESS;

    if (admin == NULL || serviceId == NULL || registrationsOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    array_list_pt references = NULL;
    service_reference_pt reference = NULL;
    char filter[32] = {0};// It is longer than the size of "service.id" + serviceId
    snprintf(filter, sizeof(filter), "(%s=%s)", (char *) OSGI_FRAMEWORK_SERVICE_ID, serviceId);
    status = bundleContext_getServiceReferences(admin->context, NULL, filter, &references);
    if (status != CELIX_SUCCESS) {
       celix_logHelper_error(admin->logHelper,"Error getting reference for service id %s.", serviceId);
       goto references_err;
    }
    //We get reference with serviceId, so the size of references must be less than or equal to 1.
    reference = arrayList_get(references, 0);
    arrayList_destroy(references);
    if (reference == NULL) {
        celix_logHelper_error(admin->logHelper, "Expect a reference for service id %s.", serviceId);
        status = CELIX_ILLEGAL_STATE;
        goto get_reference_failed;
    }

   celix_properties_t *exportedProperties = celix_properties_create();

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

    celix_array_list_t *registrations = NULL;
    if (rsaShm_isConfigTypeMatched(exportedProperties)) {
        const char *exportsProp = celix_properties_get(exportedProperties, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, NULL);
        const char *providedProp = celix_properties_get(exportedProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS, NULL);

        if (exportsProp != NULL && providedProp != NULL) {
            char *exports = strndup(exportsProp, 1024 * 10);
            char *provided = strndup(providedProp, 1024 * 10);
            celix_logHelper_info(admin->logHelper, "Export services (%s)", exports);
            celix_array_list_t *interfaces = celix_arrayList_create();
            assert(interfaces != NULL);
            registrations = celix_arrayList_create();
            assert(registrations != NULL);

            // Parse export interfaces for export service.
            if (strcmp(utils_stringTrim(exports), "*") == 0) {
                char *provided_save_ptr = NULL;
                char *interface = strtok_r(provided, ",", &provided_save_ptr);
                while (interface != NULL) {
                    celix_arrayList_add(interfaces, utils_stringTrim(interface));
                    interface = strtok_r(NULL, ",", &provided_save_ptr);
                }
            } else {
                char *provided_save_ptr = NULL;
                char *pinterface = strtok_r(provided, ",", &provided_save_ptr);
                while (pinterface != NULL) {
                    char *exports_save_ptr = NULL;
                    char *einterface = strtok_r(exports, ",", &exports_save_ptr);
                    while (einterface != NULL) {
                        if (strcmp(einterface, pinterface) == 0) {
                            celix_arrayList_add(interfaces, einterface);
                        }
                        einterface = strtok_r(NULL, ",", &exports_save_ptr);
                    }
                    pinterface = strtok_r(NULL, ",", &provided_save_ptr);
                }
            }

            // Create export registrations for its interfaces.
            size_t interfaceNum = arrayList_size(interfaces);
            for (int iter = 0; iter < interfaceNum; iter++) {
                char *interface = arrayList_get(interfaces, iter);
                endpoint_description_t *endpointDescription = NULL;
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
                //We have did copy assignment for endpointDescription in exportRegistration_create
                endpointDescription_destroy(endpointDescription);
            }

            if (celix_arrayList_size(registrations) > 0) {
                celixThreadMutex_lock(&admin->exportedServicesLock);
                celix_longHashMap_put(admin->exportedServices, atol(serviceId), registrations);
                celixThreadMutex_unlock(&admin->exportedServicesLock);
            } else {
                celix_arrayList_destroy(registrations);
                registrations = NULL;
            }

            celix_arrayList_destroy(interfaces);
            free(exports);
            free(provided);
        } else {
            celix_logHelper_warning(admin->logHelper, "No Services to export.");
        }
    }

    //We return a empty list of registrations if Remote Service Admin does not recognize any of the configuration types.
    celix_array_list_t *newRegistrations = celix_arrayList_create();
    if (registrations != NULL) {
        //clone registrations
        int regSize = celix_arrayList_size(registrations);
        for (int i = 0; i < regSize; ++i) {
            celix_arrayList_add(newRegistrations, celix_arrayList_get(registrations, i));
        }
    }
    *registrationsOut = newRegistrations;

    celix_properties_destroy(exportedProperties);

    //We have retained the service refrence in exportRegistration_create
    bundleContext_ungetServiceReference(admin->context, reference);

    return CELIX_SUCCESS;

get_reference_failed:
references_err:
    return status;
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
        celix_logHelper_info(admin->logHelper, "Remove exported service %s", endpoint->service);
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

static char *rsaShm_getRpcTypeWithDefault(rsa_shm_t *admin, const char *serviceExportedConfigs, const char *defaultVal) {
    char *rpcType = NULL;
    if (serviceExportedConfigs != NULL) {
        char *ecCopy = strdup(serviceExportedConfigs);
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(utils_stringTrim(token), RSA_RPC_TYPE_PREFIX, sizeof(RSA_RPC_TYPE_PREFIX) - 1) == 0) {
                rpcType = strdup(token);
                assert(rpcType != NULL);
                break;//TODO Multiple RPC type values may be supported in the future.
            }
            token = strtok_r(NULL, delimiter, &savePtr);
        }
        free(ecCopy);
    }
    //if rpc type is not exist, then set a default value.
    if (rpcType == NULL && defaultVal != NULL) {
        rpcType = strdup(defaultVal);
        assert(rpcType != NULL);
    }

    return rpcType;
}

static celix_status_t rsaShm_createEndpointDescription(rsa_shm_t *admin,
        celix_properties_t *exportedProperties, char *interface, endpoint_description_t **description) {
    assert(admin != NULL);
    assert(exportedProperties != NULL);
    assert(interface != NULL);
    assert(description != NULL);

    celix_properties_t *endpointProperties = celix_properties_copy(exportedProperties);
    celix_properties_unset(endpointProperties,OSGI_FRAMEWORK_OBJECTCLASS);
    celix_properties_unset(endpointProperties,OSGI_RSA_SERVICE_EXPORTED_INTERFACES);
    celix_properties_unset(endpointProperties,OSGI_RSA_SERVICE_EXPORTED_CONFIGS);
    celix_properties_unset(endpointProperties, OSGI_FRAMEWORK_SERVICE_ID);

    long serviceId = celix_properties_getAsLong(exportedProperties, OSGI_FRAMEWORK_SERVICE_ID, -1);
    assert(serviceId > 0);

    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);

    const char *uuid = celix_bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    assert(uuid != NULL);
    celix_properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    celix_properties_set(endpointProperties, (char*) OSGI_FRAMEWORK_OBJECTCLASS, interface);
    celix_properties_setLong(endpointProperties, (char*) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);
    celix_properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
    celix_properties_set(endpointProperties, (char*) OSGI_RSA_SERVICE_IMPORTED, "true");

    char *rpcType = rsaShm_getRpcTypeWithDefault(admin, celix_properties_get(exportedProperties,OSGI_RSA_SERVICE_EXPORTED_CONFIGS, NULL),
            RSA_SHM_RPC_TYPE_DEFAULT);
    assert(rpcType != NULL);
    char *importedConfigs = NULL;
    (void)asprintf(&importedConfigs, "%s,%s",RSA_SHM_CONFIGURATION_TYPE, rpcType);
    free(rpcType);
    celix_properties_setWithoutCopy(endpointProperties, strdup(OSGI_RSA_SERVICE_IMPORTED_CONFIGS), importedConfigs);
    celix_properties_set(endpointProperties, (char *) RSA_SHM_SERVER_NAME_KEY, admin->shmServerName);

    *description = calloc(1, sizeof(**description));
    assert(*description != NULL);

    (*description)->properties = endpointProperties;
    (*description)->frameworkUUID = (char*)celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    (*description)->serviceId = serviceId;
    (*description)->id = (char*)celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, NULL);
    (*description)->service = strndup(interface, 1024*10);

    return CELIX_SUCCESS;
}

celix_status_t rsaShm_getExportedServices(rsa_shm_t *admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t rsaShm_getImportedEndpoints(rsa_shm_t *admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    //It is stub and will not be called at present.
    return status;
}

celix_status_t rsaShm_importService(rsa_shm_t *admin, endpoint_description_t *endpointDesc,
        import_registration_t **registration) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || endpointDesc == NULL || registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_logHelper_info(admin->logHelper, "Import service %s", endpointDesc->service);

    bool importService = false;
    const char *importConfigs = celix_properties_get(endpointDesc->properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (importConfigs != NULL) {
        // Check whether this RSA must be imported
        char *ecCopy = strndup(importConfigs, strlen(importConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(utils_stringTrim(token), RSA_SHM_CONFIGURATION_TYPE, 1024) == 0) {
                importService = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    } else {
        celix_logHelper_warning(admin->logHelper, "Mandatory %s element missing from endpoint description", OSGI_RSA_SERVICE_IMPORTED_CONFIGS);
    }

    import_registration_t *import = NULL;
    const char *shmServerName = NULL;
    if (importService) {

        shmServerName = celix_properties_get(endpointDesc->properties, RSA_SHM_SERVER_NAME_KEY, NULL);
        if (shmServerName == NULL) {
            status = CELIX_ILLEGAL_ARGUMENT;
            celix_logHelper_error(admin->logHelper, "Get shm server name for service %s failed", endpointDesc->service);
            goto shm_server_name_err;
        }

        status = rsaShmClientManager_createOrAttachClient(admin->shmClientManager,
                shmServerName, (long)endpointDesc->serviceId);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(admin->logHelper, "Error Creating shm client for service %s. %d", endpointDesc->service, status);
            goto shm_client_err;
        }

        status = importRegistration_create(admin->context, admin->logHelper,
                endpointDesc, admin->reqSenderSvcId, &import);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(admin->logHelper, "Error Creating import registration for service %s. %d", endpointDesc->service, status);
            goto registration_create_failed;
        }

        celixThreadMutex_lock(&admin->importedServicesLock);
        celix_arrayList_add(admin->importedServices, import);
        celixThreadMutex_unlock(&admin->importedServicesLock);

        *registration = import;
    }

    return CELIX_SUCCESS;

registration_create_failed:
    rsaShmClientManager_destroyOrDetachClient(admin->shmClientManager, shmServerName,
            (long)endpointDesc->serviceId);
shm_client_err:
shm_server_name_err:
    return status;
}

celix_status_t rsaShm_removeImportedService(rsa_shm_t *admin, import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    if (admin == NULL || registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    endpoint_description_t *endpoint = NULL;// Its owner is registration.
    status = importRegistration_getImportedEndpoint(registration, &endpoint);
    if (status == CELIX_SUCCESS) {
        celix_logHelper_info(admin->logHelper, "Remove imported service %s", endpoint->service);
        const char *shmServerName = celix_properties_get(endpoint->properties,
                RSA_SHM_SERVER_NAME_KEY, NULL);
        if (shmServerName != NULL) {
            rsaShmClientManager_destroyOrDetachClient(admin->shmClientManager,
                    shmServerName, (long)endpoint->serviceId);
        } else {
            celix_logHelper_error(admin->logHelper, "Error getting shm server name for service %s. It maybe cause resource leaks.", endpoint->service);
        }
    } else {
        celix_logHelper_error(admin->logHelper, "Error getting endpoint from imported registration for service %s. It maybe cause resource leaks.", endpoint->service);
    }

    celixThreadMutex_lock(&admin->importedServicesLock);
    celix_arrayList_remove(admin->importedServices, registration);
    importRegistration_destroy(registration);
    celixThreadMutex_unlock(&admin->importedServicesLock);

    return CELIX_SUCCESS;
}
