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

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <uuid/uuid.h>
#include <curl/curl.h>

#include <jansson.h>
#include "json_serializer.h"
#include "utils.h"

#include "import_registration_dfi.h"
#include "export_registration_dfi.h"
#include "remote_service_admin_dfi.h"
#include "json_rpc.h"

#include "remote_constants.h"
#include "celix_constants.h"
#include "civetweb.h"

#include "remote_service_admin_dfi_constants.h"
#include "celix_bundle_context.h"

// defines how often the webserver is restarted (with an increased port number)
#define MAX_NUMBER_OF_RESTARTS 5


#define RSA_LOG_ERROR(admin, msg, ...) \
    logHelper_log((admin)->loghelper, OSGI_LOGSERVICE_ERROR, (msg),  ##__VA_ARGS__)

#define RSA_LOG_WARNING(admin, msg, ...) \
    logHelper_log((admin)->loghelper, OSGI_LOGSERVICE_ERROR, (msg),  ##__VA_ARGS__)

#define RSA_LOG_DEBUG(admin, msg, ...) \
    logHelper_log((admin)->loghelper, OSGI_LOGSERVICE_ERROR, (msg),  ##__VA_ARGS__)

struct remote_service_admin {
    celix_bundle_context_t *context;
    log_helper_t *loghelper;

    celix_thread_mutex_t exportedServicesLock;
    hash_map_pt exportedServices;

    celix_thread_mutex_t importedServicesLock;
    array_list_pt importedServices;

    char *port;
    char *ip;

    struct mg_context *ctx;

    FILE *logFile;
};

struct post {
    const char *readptr;
    int size;
};

struct get {
    char *writeptr;
    int size;
};

#define OSGI_RSA_REMOTE_PROXY_FACTORY   "remote_proxy_factory"
#define OSGI_RSA_REMOTE_PROXY_TIMEOUT   "remote_proxy_timeout"

static const char *data_response_headers =
        "HTTP/1.1 200 OK\r\n"
                "Cache: no-cache\r\n"
                "Content-Type: application/json\r\n"
                "\r\n";

static const char *no_content_response_headers =
        "HTTP/1.1 204 OK\r\n";

static const unsigned int DEFAULT_TIMEOUT = 0;

static int remoteServiceAdmin_callback(struct mg_connection *conn);
static celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t *admin, service_reference_pt reference, celix_properties_t *props, char *interface, endpoint_description_t **description);
static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_t *endpointDescription, char *request, char **reply, int* replyStatus);
static celix_status_t remoteServiceAdmin_getIpAddress(char* interface, char** ip);
static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp);
static void remoteServiceAdmin_log(remote_service_admin_t *admin, int level, const char *file, int line, const char *msg, ...);

celix_status_t remoteServiceAdmin_create(celix_bundle_context_t *context, remote_service_admin_t **admin) {
    celix_status_t status = CELIX_SUCCESS;

    *admin = calloc(1, sizeof(**admin));

    if (!*admin) {
        status = CELIX_ENOMEM;
    } else {
        (*admin)->context = context;
        (*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
         arrayList_create(&(*admin)->importedServices);

        celixThreadMutex_create(&(*admin)->exportedServicesLock, NULL);
        celixThreadMutex_create(&(*admin)->importedServicesLock, NULL);

        if (logHelper_create(context, &(*admin)->loghelper) == CELIX_SUCCESS) {
            logHelper_start((*admin)->loghelper);
            dynCommon_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
            dynType_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
            dynFunction_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
            dynInterface_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
            jsonSerializer_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
            jsonRpc_logSetup((void *)remoteServiceAdmin_log, *admin, 1);
        }

        long port = celix_bundleContext_getPropertyAsLong(context, RSA_PORT_KEY, RSA_PORT_DEFAULT);
        const char *ip = celix_bundleContext_getProperty(context, RSA_IP_KEY, RSA_IP_DEFAULT);
        const char *interface = celix_bundleContext_getProperty(context, RSA_INTERFACE_KEY, NULL);

        char *detectedIp = NULL;
        if ((interface != NULL) && (remoteServiceAdmin_getIpAddress((char*)interface, &detectedIp) != CELIX_SUCCESS)) {
            logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: Could not retrieve IP address for interface %s", interface);
        }
        if (detectedIp != NULL) {
            ip = detectedIp;
        }

        if (ip != NULL) {
            logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_DEBUG, "RSA: Using %s for service annunciation", ip);
            (*admin)->ip = strdup(ip);
        }

        if (detectedIp != NULL) {
            free(detectedIp);
        }

        // Prepare callbacks structure. We have only one callback, the rest are NULL.
        struct mg_callbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.begin_request = remoteServiceAdmin_callback;

        char newPort[10];
        snprintf(newPort, 10, "%li", port);

        unsigned int port_counter = 0;
        do {

            const char *options[] = { "listening_ports", newPort, "num_threads", "5", NULL};

            (*admin)->ctx = mg_start(&callbacks, (*admin), options);

            if ((*admin)->ctx != NULL) {
                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Start webserver: %s", newPort);
                (*admin)->port = strdup(newPort);

            } else {
                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_ERROR, "Error while starting rsa server on port %s - retrying on port %li...", newPort, port + port_counter);
                snprintf(newPort, 10,  "%li", port + port_counter++);
            }
        } while (((*admin)->ctx == NULL) && (port_counter < MAX_NUMBER_OF_RESTARTS));

    }

    bool logCalls = celix_bundleContext_getPropertyAsBool(context, RSA_LOG_CALLS_KEY, RSA_LOG_CALLS_DEFAULT);
    if (logCalls) {
        const char *f = celix_bundleContext_getProperty(context, RSA_LOG_CALLS_FILE_KEY, RSA_LOG_CALLS_FILE_DEFAULT);
        if (strncmp(f, "stdout", strlen("stdout")) == 0) {
            (*admin)->logFile = stdout;
        } else {
            (*admin)->logFile = fopen(f, "w");
            if ( (*admin)->logFile == NULL) {
                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "Error opening file '%s' for logging calls. %s", f, strerror(errno));
            }
        }
    }

    return status;
}


celix_status_t remoteServiceAdmin_destroy(remote_service_admin_t **admin)
{
    celix_status_t status = CELIX_SUCCESS;

    if ( (*admin)->logFile != NULL && (*admin)->logFile != stdout) {
        fclose((*admin)->logFile);
    }

    free((*admin)->ip);
    free((*admin)->port);
    free(*admin);

    *admin = NULL;

    return status;
}


celix_status_t remoteServiceAdmin_stop(remote_service_admin_t *admin) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&admin->exportedServicesLock);

    hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);
    while (hashMapIterator_hasNext(iter)) {
        array_list_pt exports = hashMapIterator_nextValue(iter);
        int i;
        for (i = 0; i < arrayList_size(exports); i++) {
            export_registration_t *export = arrayList_get(exports, i);
            if (export != NULL) {
                exportRegistration_stop(export);
                exportRegistration_destroy(export);
            }
        }
        arrayList_destroy(exports);
    }
    hashMapIterator_destroy(iter);
    celixThreadMutex_unlock(&admin->exportedServicesLock);

    celixThreadMutex_lock(&admin->importedServicesLock);
    int i;
    int size = arrayList_size(admin->importedServices);
    for (i = 0; i < size ; i += 1) {
        import_registration_t *import = arrayList_get(admin->importedServices, i);
        if (import != NULL) {
            importRegistration_stop(import);
            importRegistration_destroy(import);
        }
    }
    celixThreadMutex_unlock(&admin->importedServicesLock);

    if (admin->ctx != NULL) {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Stopping webserver...");
        mg_stop(admin->ctx);
        admin->ctx = NULL;
    }

    hashMap_destroy(admin->exportedServices, false, false);
    arrayList_destroy(admin->importedServices);

    logHelper_stop(admin->loghelper);
    logHelper_destroy(&admin->loghelper);

    return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
//void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {

celix_status_t importRegistration_getFactory(import_registration_t *import, service_factory_pt *factory);

static int remoteServiceAdmin_callback(struct mg_connection *conn) {
    int result = 1; // zero means: let civetweb handle it further, any non-zero value means it is handled by us...

    const struct mg_request_info *request_info = mg_get_request_info(conn);
    if (request_info->uri != NULL) {
        remote_service_admin_t *rsa = request_info->user_data;


        if (strncmp(request_info->uri, "/service/", 9) == 0 && strcmp("POST", request_info->request_method) == 0) {

            // uri = /services/myservice/call
            const char *uri = request_info->uri;
            // rest = myservice/call

            const char *rest = uri+9;
            char *interfaceStart = strchr(rest, '/');
            int pos = interfaceStart - rest;
            char service[pos+1];
            strncpy(service, rest, pos);
            service[pos] = '\0';
            unsigned long serviceId = strtoul(service,NULL,10);

            celixThreadMutex_lock(&rsa->exportedServicesLock);

            //find endpoint
            export_registration_t *export = NULL;
            hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
                array_list_pt exports = hashMapEntry_getValue(entry);
                int expIt = 0;
                for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
                    export_registration_t *check = arrayList_get(exports, expIt);
                    export_reference_t * ref = NULL;
                    exportRegistration_getExportReference(check, &ref);
                    endpoint_description_t * checkEndpoint = NULL;
                    exportReference_getExportedEndpoint(ref, &checkEndpoint);
                    if (serviceId == checkEndpoint->serviceId) {
                        export = check;
                        free(ref);
                        break;
                    }
                    free(ref);
                }
            }
            hashMapIterator_destroy(iter);

            if (export != NULL) {

                uint64_t datalength = request_info->content_length;
                char* data = malloc(datalength + 1);
                mg_read(conn, data, datalength);
                data[datalength] = '\0';

                char *response = NULL;
                int responceLength = 0;
                int rc = exportRegistration_call(export, data, -1, &response, &responceLength);
                if (rc != CELIX_SUCCESS) {
                    RSA_LOG_ERROR(rsa, "Error trying to invoke remove service, got error %i\n", rc);
                }

                if (rc == CELIX_SUCCESS && response != NULL) {
                    mg_write(conn, data_response_headers, strlen(data_response_headers));
                    mg_write(conn, response, strlen(response));
                    free(response);
                } else {
                    mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
                }
                result = 1;

                free(data);
            } else {
                result = 0;
                RSA_LOG_WARNING(rsa, "No export registration found for service id %lu", serviceId);
            }

            celixThreadMutex_unlock(&rsa->exportedServicesLock);

        }
    }

    return result;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t *admin, char *serviceId, celix_properties_t *properties, array_list_pt *registrations) {
    celix_status_t status = CELIX_SUCCESS;

    bool export = false;
    const char *exportConfigs = celix_properties_get(properties, OSGI_RSA_SERVICE_EXPORTED_CONFIGS, RSA_DFI_CONFIGURATION_TYPE);
    if (exportConfigs != NULL) {
        // See if the EXPORT_CONFIGS matches this RSA. If so, try to export.

        char *ecCopy = strndup(exportConfigs, strlen(exportConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(utils_stringTrim(token), RSA_DFI_CONFIGURATION_TYPE, 1024) == 0) {
                export = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    } else {
        export = true;
    }

    if (export) {
        arrayList_create(registrations);
        array_list_pt references = NULL;
        service_reference_pt reference = NULL;
        char filter[256];

        snprintf(filter, 256, "(%s=%s)", (char *) OSGI_FRAMEWORK_SERVICE_ID, serviceId);

        status = bundleContext_getServiceReferences(admin->context, NULL, filter, &references);

        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_DEBUG, "RSA: exportService called for serviceId %s", serviceId);

        int i;
        int size = arrayList_size(references);
        for (i = 0; i < size; i += 1) {
            if (i == 0) {
                reference = arrayList_get(references, i);
            } else {
                bundleContext_ungetServiceReference(admin->context, arrayList_get(references, i));
            }
        }
        arrayList_destroy(references);

        if (reference == NULL) {
            logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "ERROR: expected a reference for service id %s.",
                          serviceId);
            status = CELIX_ILLEGAL_STATE;
        }

        const char *exports = NULL;
        const char *provided = NULL;
        if (status == CELIX_SUCCESS) {
            serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports);
            serviceReference_getProperty(reference, (char *) OSGI_FRAMEWORK_OBJECTCLASS, &provided);

            if (exports == NULL || provided == NULL || strcmp(exports, provided) != 0) {
                logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: No Services to export.");
                status = CELIX_ILLEGAL_STATE;
            } else {
                logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Export service (%s)", provided);
            }
        }

        if (status == CELIX_SUCCESS) {
            const char *interface = provided;
            endpoint_description_t *endpoint = NULL;
            export_registration_t *registration = NULL;

            remoteServiceAdmin_createEndpointDescription(admin, reference, properties, (char *) interface, &endpoint);
            //TODO precheck if descriptor exists
            status = exportRegistration_create(admin->loghelper, reference, endpoint, admin->context, admin->logFile,
                                               &registration);
            if (status == CELIX_SUCCESS) {
                status = exportRegistration_start(registration);
                if (status == CELIX_SUCCESS) {
                    arrayList_add(*registrations, registration);
                }
            }
        }


        if (status == CELIX_SUCCESS) {
            celixThreadMutex_lock(&admin->exportedServicesLock);
            hashMap_put(admin->exportedServices, reference, *registrations);
            celixThreadMutex_unlock(&admin->exportedServicesLock);
        } else {
            arrayList_destroy(*registrations);
            *registrations = NULL;
        }
    }

    return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(remote_service_admin_t *admin, export_registration_t *registration) {
    celix_status_t status;

    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA_DFI: Removing exported service");

    export_reference_t * ref = NULL;
    status = exportRegistration_getExportReference(registration, &ref);

    if (status == CELIX_SUCCESS && ref != NULL) {
        service_reference_pt servRef;
        celixThreadMutex_lock(&admin->exportedServicesLock);
        exportReference_getExportedService(ref, &servRef);

        array_list_pt exports = (array_list_pt)hashMap_remove(admin->exportedServices, servRef);
        if(exports!=NULL){
            arrayList_destroy(exports);
        }

        exportRegistration_close(registration);
        exportRegistration_destroy(registration);

        celixThreadMutex_unlock(&admin->exportedServicesLock);

        free(ref);

    } else {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "Cannot find reference for registration");
    }

    return status;
}

static celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t *admin, service_reference_pt reference, celix_properties_t *props, char *interface, endpoint_description_t **endpoint) {

    celix_status_t status = CELIX_SUCCESS;
    celix_properties_t *endpointProperties = celix_properties_create();

    unsigned int size = 0;
    char **keys;

    serviceReference_getPropertyKeys(reference, &keys, &size);
    for (int i = 0; i < size; i++) {
        char *key = keys[i];
        const char *value = NULL;

        if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS
            && strcmp(key, (char*) OSGI_RSA_SERVICE_EXPORTED_INTERFACES) != 0
            && strcmp(key, (char*) OSGI_RSA_SERVICE_EXPORTED_CONFIGS) != 0
            && strcmp(key, (char*) OSGI_FRAMEWORK_OBJECTCLASS) != 0) {
            celix_properties_set(endpointProperties, key, value);
        }
    }

    hash_map_entry_pt entry = hashMap_getEntry(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);

    char* key = hashMapEntry_getKey(entry);
    char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
    const char *uuid = NULL;

    char buf[512];
    snprintf(buf, 512,  "/service/%s/%s", serviceId, interface);

    char url[1024];
    snprintf(url, 1024, "http://%s:%s%s", admin->ip, admin->port, buf);

    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);

    bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    celix_properties_set(endpointProperties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    celix_properties_set(endpointProperties, OSGI_FRAMEWORK_OBJECTCLASS, interface);
    celix_properties_set(endpointProperties, OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);
    celix_properties_set(endpointProperties, OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
    celix_properties_set(endpointProperties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(endpointProperties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, (char*) RSA_DFI_CONFIGURATION_TYPE);
    celix_properties_set(endpointProperties, RSA_DFI_ENDPOINT_URL, url);

    if (props != NULL) {
        hash_map_iterator_pt propIter = hashMapIterator_create(props);
        while (hashMapIterator_hasNext(propIter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(propIter);
            celix_properties_set(endpointProperties, (char*)hashMapEntry_getKey(entry), (char*)hashMapEntry_getValue(entry));
        }
        hashMapIterator_destroy(propIter);
    }

    *endpoint = calloc(1, sizeof(**endpoint));
    if (!*endpoint) {
        status = CELIX_ENOMEM;
    } else {
        (*endpoint)->id = (char*) celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, NULL);
        const char *serviceId = NULL;
        serviceReference_getProperty(reference, (char*) OSGI_FRAMEWORK_SERVICE_ID, &serviceId);
        (*endpoint)->serviceId = strtoull(serviceId, NULL, 0);
        (*endpoint)->frameworkUUID = (char*) celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
        (*endpoint)->service = strndup(interface, 1024*10);
        (*endpoint)->properties = endpointProperties;
    }

    free(key);
    free(serviceId);
    free(keys);

    return status;
}

static celix_status_t remoteServiceAdmin_getIpAddress(char* interface, char** ip) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != NULL && status != CELIX_SUCCESS; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
                if (interface == NULL) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
                else if (strcmp(ifa->ifa_name, interface) == 0) {
                    *ip = strdup(host);
                    status = CELIX_SUCCESS;
                }
            }
        }

        freeifaddrs(ifaddr);
    }

    return status;
}


celix_status_t remoteServiceAdmin_destroyEndpointDescription(endpoint_description_t **description)
{
    celix_status_t status = CELIX_SUCCESS;

    celix_properties_destroy((*description)->properties);
    free((*description)->service);
    free(*description);

    return status;
}


celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t *admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t *admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, import_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;

    bool importService = false;
    const char *importConfigs = celix_properties_get(endpointDescription->properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (importConfigs != NULL) {
        // Check whether this RSA must be imported
        char *ecCopy = strndup(importConfigs, strlen(importConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(utils_stringTrim(token), RSA_DFI_CONFIGURATION_TYPE, 1024) == 0) {
                importService = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    } else {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "Mandatory %s element missing from endpoint description",
                OSGI_RSA_SERVICE_IMPORTED_CONFIGS);
    }

    if (importService) {
        import_registration_t *import = NULL;

        const char *objectClass = celix_properties_get(endpointDescription->properties, "objectClass", NULL);
        const char *serviceVersion = celix_properties_get(endpointDescription->properties, CELIX_FRAMEWORK_SERVICE_VERSION, NULL);

        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Import service %s", endpointDescription->service);
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "Registering service factory (proxy) for service '%s'\n",
                      objectClass);

        if (objectClass != NULL) {
            status = importRegistration_create(admin->context, endpointDescription, objectClass, serviceVersion,
                                               admin->logFile, &import);
        }
        if (status == CELIX_SUCCESS && import != NULL) {
            importRegistration_setSendFn(import, (send_func_type) remoteServiceAdmin_send, admin);
        }

        if (status == CELIX_SUCCESS && import != NULL) {
            status = importRegistration_start(import);
        }

        celixThreadMutex_lock(&admin->importedServicesLock);
        arrayList_add(admin->importedServices, import);
        celixThreadMutex_unlock(&admin->importedServicesLock);

        if (status == CELIX_SUCCESS) {
            *out = import;
        }
    }

    return status;
}


celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_t *admin, import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA_DFI: Removing imported service");

    celixThreadMutex_lock(&admin->importedServicesLock);
    int i;
    int size = arrayList_size(admin->importedServices);
    import_registration_t * current  = NULL;
    for (i = 0; i < size; i += 1) {
        current = arrayList_get(admin->importedServices, i);
        if (current == registration) {
            arrayList_remove(admin->importedServices, i);
            importRegistration_close(current);
            importRegistration_destroy(current);
            break;
        }
    }
    celixThreadMutex_unlock(&admin->importedServicesLock);

    return status;
}


static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_t *endpointDescription, char *request, char **reply, int* replyStatus) {
    remote_service_admin_t * rsa = handle;
    struct post post;
    post.readptr = request;
    post.size = strlen(request);

    struct get get;
    get.size = 0;
    get.writeptr = malloc(1);

    const char *serviceUrl = celix_properties_get(endpointDescription->properties, (char*) RSA_DFI_ENDPOINT_URL, NULL);
    char url[256];
    snprintf(url, 256, "%s", serviceUrl);

    // assume the default timeout
    int timeout = DEFAULT_TIMEOUT;

    const char *timeoutStr = NULL;
    // Check if the endpoint has a timeout, if so, use it.
    timeoutStr = (char*) celix_properties_get(endpointDescription->properties, (char*) OSGI_RSA_REMOTE_PROXY_TIMEOUT, NULL);
    if (timeoutStr == NULL) {
        // If not, get the global variable and use that one.
        bundleContext_getProperty(rsa->context, (char*) OSGI_RSA_REMOTE_PROXY_TIMEOUT, &timeoutStr);
    }

    // Update timeout if a property is used to set it.
    if (timeoutStr != NULL) {
        timeout = atoi(timeoutStr);
    }

    celix_status_t status = CELIX_SUCCESS;
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(!curl) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, remoteServiceAdmin_readCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &post);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, remoteServiceAdmin_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&get);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)post.size);
        logHelper_log(rsa->loghelper, OSGI_LOGSERVICE_DEBUG, "RSA: Performing curl post\n");
        res = curl_easy_perform(curl);

        *reply = get.writeptr;
        *replyStatus = res;

        curl_easy_cleanup(curl);
    }

    return status;
}

static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct post *post = userp;

    if (post->size) {
        *(char *) ptr = post->readptr[0];
        post->readptr++;
        post->size--;
        return 1;
    }

    return 0;
}

static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct get *mem = (struct get *)userp;

    mem->writeptr = realloc(mem->writeptr, mem->size + realsize + 1);
    if (mem->writeptr == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->writeptr[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->writeptr[mem->size] = 0;

    return realsize;
}


static void remoteServiceAdmin_log(remote_service_admin_t *admin, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    int levels[5] = {0, OSGI_LOGSERVICE_ERROR, OSGI_LOGSERVICE_WARNING, OSGI_LOGSERVICE_INFO, OSGI_LOGSERVICE_DEBUG};

    char buf1[256];
    snprintf(buf1, 256, "FILE:%s, LINE:%i, MSG:", file, line);

    char buf2[256];
    vsnprintf(buf2, 256, msg, ap);
    logHelper_log(admin->loghelper, levels[level], "%s%s", buf1, buf2);
    va_end(ap);
}
