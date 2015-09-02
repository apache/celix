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
/*
 * remote_service_admin_impl.c
 *
 *  \date       May 21, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
#include "remote_service_admin.h"

#include "import_registration_dfi.h"
#include "export_registration_dfi.h"
#include "remote_service_admin_dfi.h"
#include "dyn_interface.h"

#include "remote_constants.h"
#include "constants.h"
#include "civetweb.h"

// defines how often the webserver is restarted (with an increased port number)
#define MAX_NUMBER_OF_RESTARTS 	5

struct remote_service_admin {
    bundle_context_pt context;
    log_helper_pt loghelper;

    celix_thread_mutex_t exportedServicesLock;
    hash_map_pt exportedServices;

    celix_thread_mutex_t importedServicesLock;
    hash_map_pt importedServices;

    char *port;
    char *ip;

    struct mg_context *ctx;
};

struct post {
    const char *readptr;
    int size;
};

struct get {
    char *writeptr;
    int size;
};

#define OSGI_RSA_REMOTE_PROXY_FACTORY 	"remote_proxy_factory"
#define OSGI_RSA_REMOTE_PROXY_TIMEOUT   "remote_proxy_timeout"

static const char *data_response_headers =
        "HTTP/1.1 200 OK\r\n"
                "Cache: no-cache\r\n"
                "Content-Type: application/json\r\n"
                "\r\n";

static const char *no_content_response_headers =
        "HTTP/1.1 204 OK\r\n";

// TODO do we need to specify a non-Amdatu specific configuration type?!
static const char * const CONFIGURATION_TYPE = "org.amdatu.remote.admin.http";
static const char * const ENDPOINT_URL = "org.amdatu.remote.admin.http.url";

static const char *DEFAULT_PORT = "8888";
static const char *DEFAULT_IP = "127.0.0.1";

static const unsigned int DEFAULT_TIMEOUT = 0;

static int remoteServiceAdmin_callback(struct mg_connection *conn);
static celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, service_reference_pt reference, char *interface, endpoint_description_pt *description);
static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus);
static celix_status_t remoteServiceAdmin_getIpAdress(char* interface, char** ip);
static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp);
static void remoteServiceAdmin_log(remote_service_admin_pt admin, int level, const char *file, int line, const char *msg, ...);

celix_status_t remoteServiceAdmin_create(bundle_context_pt context, remote_service_admin_pt *admin) {
    celix_status_t status = CELIX_SUCCESS;

    *admin = calloc(1, sizeof(**admin));

    if (!*admin) {
        status = CELIX_ENOMEM;
    } else {
        unsigned int port_counter = 0;
        char *port = NULL;
        char *ip = NULL;
        char *detectedIp = NULL;
        (*admin)->context = context;
        (*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);

        celixThreadMutex_create(&(*admin)->exportedServicesLock, NULL);
        celixThreadMutex_create(&(*admin)->importedServicesLock, NULL);

        if (logHelper_create(context, &(*admin)->loghelper) == CELIX_SUCCESS) {
            logHelper_start((*admin)->loghelper);
            dynCommon_logSetup((void *)remoteServiceAdmin_log, *admin, 4);
            dynType_logSetup((void *)remoteServiceAdmin_log, *admin, 4);
            dynFunction_logSetup((void *)remoteServiceAdmin_log, *admin, 4);
            dynInterface_logSetup((void *)remoteServiceAdmin_log, *admin, 4);
            jsonSerializer_logSetup((void *)remoteServiceAdmin_log, *admin, 4);
        }

        bundleContext_getProperty(context, "RSA_PORT", &port);
        if (port == NULL) {
            port = (char *)DEFAULT_PORT;
        }

        bundleContext_getProperty(context, "RSA_IP", &ip);
        if (ip == NULL) {
            char *interface = NULL;

            bundleContext_getProperty(context, "RSA_INTERFACE", &interface);
            if ((interface != NULL) && (remoteServiceAdmin_getIpAdress(interface, &detectedIp) != CELIX_SUCCESS)) {
                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: Could not retrieve IP adress for interface %s", interface);
            }

            if (ip == NULL) {
                remoteServiceAdmin_getIpAdress(NULL, &detectedIp);
            }

            ip = detectedIp;
        }

        if (ip != NULL) {
            logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Using %s for service annunciation", ip);
            (*admin)->ip = strdup(ip);
        }
        else {
            logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: No IP address for service annunciation set. Using %s", DEFAULT_IP);
            (*admin)->ip = (char*) DEFAULT_IP;
        }

        if (detectedIp != NULL) {
            free(detectedIp);
        }

        // Prepare callbacks structure. We have only one callback, the rest are NULL.
        struct mg_callbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.begin_request = remoteServiceAdmin_callback;

        do {
            char newPort[10];
            const char *options[] = { "listening_ports", port, NULL};

            (*admin)->ctx = mg_start(&callbacks, (*admin), options);

            if ((*admin)->ctx != NULL) {
                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Start webserver: %s", port);
                (*admin)->port = strdup(port);

            }
            else {
                char* endptr = port;
                int currentPort = strtol(port, &endptr, 10);

                errno = 0;

                if (*endptr || errno != 0) {
                    currentPort = strtol(DEFAULT_PORT, NULL, 10);
                }

                port_counter++;
                snprintf(&newPort[0], 6,  "%d", (currentPort+1));

                logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_ERROR, "Error while starting rsa server on port %s - retrying on port %s...", port, newPort);
                port = newPort;
            }
        } while(((*admin)->ctx == NULL) && (port_counter < MAX_NUMBER_OF_RESTARTS));

    }
    return status;
}


celix_status_t remoteServiceAdmin_destroy(remote_service_admin_pt *admin)
{
    celix_status_t status = CELIX_SUCCESS;

    free((*admin)->ip);
    free((*admin)->port);
    free(*admin);

    //TODO destroy exports/imports

    *admin = NULL;

    return status;
}


celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&admin->exportedServicesLock);

    hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);
    while (hashMapIterator_hasNext(iter)) {
        array_list_pt exports = hashMapIterator_nextValue(iter);
        int i;
        for (i = 0; i < arrayList_size(exports); i++) {
            export_registration_pt export = arrayList_get(exports, i);
            if (export != NULL) {
                exportRegistration_stop(export);
            }
        }
    }
    hashMapIterator_destroy(iter);
    celixThreadMutex_unlock(&admin->exportedServicesLock);

    celixThreadMutex_lock(&admin->importedServicesLock);

    iter = hashMapIterator_create(admin->importedServices);
    while (hashMapIterator_hasNext(iter))
    {

        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        import_registration_pt import = hashMapEntry_getValue(entry);

        if (import != NULL) {
            importRegistration_stop(import);
        }
    }

    hashMapIterator_destroy(iter);
    celixThreadMutex_unlock(&admin->importedServicesLock);

    if (admin->ctx != NULL) {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Stopping webserver...");
        mg_stop(admin->ctx);
        admin->ctx = NULL;
    }

    hashMap_destroy(admin->exportedServices, false, false);
    hashMap_destroy(admin->importedServices, false, false);

    logHelper_stop(admin->loghelper);
    logHelper_destroy(&admin->loghelper);

    return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
//void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {

celix_status_t importRegistration_getFactory(import_registration_pt import, service_factory_pt *factory);

static int remoteServiceAdmin_callback(struct mg_connection *conn) {
    int result = 1; // zero means: let civetweb handle it further, any non-zero value means it is handled by us...

    const struct mg_request_info *request_info = mg_get_request_info(conn);
    if (request_info->uri != NULL) {
        remote_service_admin_pt rsa = request_info->user_data;


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
            long serviceId = atol(service);

            celixThreadMutex_lock(&rsa->exportedServicesLock);

            //find endpoint
            export_registration_pt export = NULL;
            hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
                array_list_pt exports = hashMapEntry_getValue(entry);
                int expIt = 0;
                for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
                    export_registration_pt check = arrayList_get(exports, expIt);
                    export_reference_pt  ref = NULL;
                    exportRegistration_getExportReference(check, &ref);
                    endpoint_description_pt  checkEndpoint = NULL;
                    exportReference_getExportedEndpoint(ref, &checkEndpoint);
                    if (serviceId == checkEndpoint->serviceId) {
                        export = check;
                        break;
                    }
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
                //TODO check rc

                if (response != NULL) {
                    mg_write(conn, data_response_headers, strlen(data_response_headers));
//              mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
                    printf("writing response '%s'\n", response);
                    mg_write(conn, response, strlen(response));
//              mg_send_data(conn, response, strlen(response));
//              mg_write_data(conn, response, strlen(response));

                    free(response);
                } else {
                    mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
                }
                result = 1;

                free(data);
            } else {
                result = 0;
                //TODO log warning
            }

            celixThreadMutex_unlock(&rsa->exportedServicesLock);

        }
    }

    return result;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_pt admin, char *serviceId, properties_pt properties, array_list_pt *registrations) {
    celix_status_t status = CELIX_SUCCESS;

    arrayList_create(registrations);
    array_list_pt references = NULL;
    service_reference_pt reference = NULL;
    char filter [256];

    snprintf(filter, 256, "(%s=%s)", (char *)OSGI_FRAMEWORK_SERVICE_ID, serviceId);

    status = bundleContext_getServiceReferences(admin->context, NULL, filter, &references);

    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "RSA: exportService called for serviceId %s", serviceId);

    if (status == CELIX_SUCCESS && arrayList_size(references) >= 1) {
        reference = arrayList_get(references, 0);
    }

    if(references != NULL){
        arrayList_destroy(references);
    }

    if (reference == NULL) {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "ERROR: expected a reference for service id %s.", serviceId);
        status = CELIX_ILLEGAL_STATE;
    }

    char *exports = NULL;
    char *provided = NULL;
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
        char *interface = provided;
        endpoint_description_pt endpoint = NULL;
        export_registration_pt registration = NULL;

        remoteServiceAdmin_createEndpointDescription(admin, reference, interface, &endpoint);
        printf("RSA: Creating export registration with endpoint pointer %p\n", endpoint);
        //TOOD precheck if descriptor exists
        status = exportRegistration_create(admin->loghelper, remoteServiceAdmin_removeExportedService, admin, reference, endpoint, admin->context, &registration);
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
    }

    return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(remote_service_admin_pt admin, export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    //TODO
    /*
    remote_service_admin_pt admin = registration->rsa;

    celixThreadMutex_lock(&admin->exportedServicesLock);

    hashMap_remove(admin->exportedServices, registration->reference);
    //TODO stop ?

    celixThreadMutex_unlock(&admin->exportedServicesLock);
    */
    return status;
}

static celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, service_reference_pt reference, char *interface, endpoint_description_pt *endpoint) {
    celix_status_t status = CELIX_SUCCESS;
    properties_pt endpointProperties = properties_create();


    unsigned int size = 0;
    char **keys;

    serviceReference_getPropertyKeys(reference, &keys, &size);
    for (int i = 0; i < size; i++) {
        char *key = keys[i];
        char *value = NULL;

        if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS
            && strcmp(key, (char*) OSGI_RSA_SERVICE_EXPORTED_INTERFACES) != 0
            && strcmp(key, (char*) OSGI_FRAMEWORK_OBJECTCLASS) != 0) {
            properties_set(endpointProperties, key, value);
            printf("Added property '%s' with value '%s'\n", key, value);
        }
    }

    hash_map_entry_pt entry = hashMap_getEntry(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);

    char* key = hashMapEntry_getKey(entry);
    char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
    char *uuid = NULL;

    char buf[512];
    snprintf(buf, 512,  "/service/%s/%s", serviceId, interface);

    char url[1024];
    snprintf(url, 1024, "http://%s:%s%s", admin->ip, admin->port, buf);

    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);

    bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    properties_set(endpointProperties, (char*) OSGI_FRAMEWORK_OBJECTCLASS, interface);
    properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);
    properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
    properties_set(endpointProperties, (char*) OSGI_RSA_SERVICE_IMPORTED, "true");
    properties_set(endpointProperties, (char*) OSGI_RSA_SERVICE_IMPORTED_CONFIGS, (char*) CONFIGURATION_TYPE);
    properties_set(endpointProperties, (char*) ENDPOINT_URL, url);



    *endpoint = calloc(1, sizeof(**endpoint));
    if (!*endpoint) {
        status = CELIX_ENOMEM;
    } else {
        (*endpoint)->id = properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID);
        char *serviceId = NULL;
        serviceReference_getProperty(reference, (char*) OSGI_FRAMEWORK_SERVICE_ID, &serviceId);
        (*endpoint)->serviceId = strtoull(serviceId, NULL, 0);
        (*endpoint)->frameworkUUID = properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID);
        (*endpoint)->service = interface;
        (*endpoint)->properties = endpointProperties;
    }

    free(key);
    free(serviceId);
    free(keys);

    return status;
}

static celix_status_t remoteServiceAdmin_getIpAdress(char* interface, char** ip) {
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


celix_status_t remoteServiceAdmin_destroyEndpointDescription(endpoint_description_pt *description)
{
    celix_status_t status = CELIX_SUCCESS;

    properties_destroy((*description)->properties);
    free(*description);

    return status;
}


celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_pt admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_pt admin, array_list_pt *services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_pt admin, endpoint_description_pt endpointDescription, import_registration_pt *out) {
    celix_status_t status = CELIX_SUCCESS;

    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Import service %s", endpointDescription->service);

    const char *objectClass = properties_get(endpointDescription->properties, "objectClass");
    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "Registering service factory (proxy) for service '%s'\n", objectClass);


    import_registration_pt import = NULL;
    if (objectClass != NULL) {
        status = importRegistration_create(admin->context, NULL, NULL, endpointDescription, objectClass, &import);
    }
    if (status == CELIX_SUCCESS) {
        importRegistration_setSendFn(import, remoteServiceAdmin_send, admin);
    }

    if (status == CELIX_SUCCESS) {
        status = importRegistration_start(import);
    }

    //celixThreadMutex_lock(&admin->importedServicesLock);
    //TODO add to list
    //celixThreadMutex_unlock(&admin->importedServicesLock);

    if (status == CELIX_SUCCESS) {
        *out = import;
    }

    return status;
}


celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_pt admin, import_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
    /*

      endpoint_description_pt endpointDescription = (endpoint_description_pt) registration->endpointDescription;
      import_registration_factory_pt registration_factory = NULL;

      celixThreadMutex_lock(&admin->importedServicesLock);

      registration_factory = (import_registration_factory_pt) hashMap_get(admin->importedServices, endpointDescription->service);

      // factory available
      if ((registration_factory == NULL) || (registration_factory->trackedFactory == NULL))
      {
          logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "RSA: Error while retrieving registration factory for imported service %s", endpointDescription->service);
      }
      else
      {
          registration_factory->trackedFactory->unregisterProxyService(registration_factory->trackedFactory->factory, endpointDescription);
          arrayList_removeElement(registration_factory->registrations, registration);
          importRegistration_destroy(registration);

          if (arrayList_isEmpty(registration_factory->registrations))
          {
              logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: closing proxy.");

              serviceTracker_close(registration_factory->proxyFactoryTracker);
              importRegistrationFactory_close(registration_factory);

              hashMap_remove(admin->importedServices, endpointDescription->service);

              importRegistrationFactory_destroy(&registration_factory);
          }
      }

      celixThreadMutex_unlock(&admin->importedServicesLock);

      return status;
    */
}


static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus) {
    remote_service_admin_pt  rsa = handle;
    struct post post;
    post.readptr = request;
    post.size = strlen(request);

    struct get get;
    get.size = 0;
    get.writeptr = malloc(1);

    char *serviceUrl = properties_get(endpointDescription->properties, (char*) ENDPOINT_URL);
    char url[256];
    snprintf(url, 256, "%s", serviceUrl);

    // assume the default timeout
    int timeout = DEFAULT_TIMEOUT;

    char *timeoutStr = NULL;
    // Check if the endpoint has a timeout, if so, use it.
    timeoutStr = properties_get(endpointDescription->properties, (char*) OSGI_RSA_REMOTE_PROXY_TIMEOUT);
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_URL, &url[0]);
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


static void remoteServiceAdmin_log(remote_service_admin_pt admin, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    int levels[5] = {0, OSGI_LOGSERVICE_ERROR, OSGI_LOGSERVICE_WARNING, OSGI_LOGSERVICE_INFO, OSGI_LOGSERVICE_DEBUG};

    char buf1[256];
    snprintf(buf1, 256, "FILE:%s, LINE:%i, MSG:", file, line);

    char buf2[256];
    vsnprintf(buf2, 256, msg, ap);
    logHelper_log(admin->loghelper, levels[level], "%s%s", buf1, buf2);
}