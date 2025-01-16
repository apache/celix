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
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <uuid/uuid.h>
#include <curl/curl.h>
#include <limits.h>
#include <net/if.h>

#include <jansson.h>
#include "json_serializer.h"
#include "utils.h"
#include "celix_utils.h"
#include "celix_ref.h"
#include "hash_map.h"

#include "import_registration_dfi.h"
#include "export_registration_dfi.h"
#include "remote_service_admin_dfi.h"
#include "json_rpc.h"

#include "remote_constants.h"
#include "celix_constants.h"
#include "civetweb.h"

#include "remote_service_admin_dfi_constants.h"
#include "celix_bundle_context.h"
#include "celix_string_hash_map.h"
#include "celix_stdlib_cleanup.h"

// defines how often the webserver is restarted (with an increased port number)
#define MAX_NUMBER_OF_RESTARTS 5


#define RSA_LOG_ERROR(admin, msg, ...) \
    celix_logHelper_log((admin)->loghelper, CELIX_LOG_LEVEL_ERROR, (msg),  ##__VA_ARGS__)

#define RSA_LOG_WARNING(admin, msg, ...) \
    celix_logHelper_log((admin)->loghelper, CELIX_LOG_LEVEL_ERROR, (msg),  ##__VA_ARGS__)

#define RSA_LOG_DEBUG(admin, msg, ...) \
    celix_logHelper_log((admin)->loghelper, CELIX_LOG_LEVEL_ERROR, (msg),  ##__VA_ARGS__)


/**
 * If set to true the rsa will create a thread to handle stopping of service export.
 *
 * This stop thread can be removed when the branch feature/async_svc_registration is merged and
 * celix_bundleContext_stopTrackerAsync is available.
 *
 */
#define CELIX_RSA_USE_STOP_EXPORT_THREAD true


typedef struct celix_imported_endpoint_url_ref {
    struct celix_ref ref;//It must be first
    char url[0];
}celix_imported_endpoint_url_ref_t;

struct remote_service_admin {
    celix_bundle_context_t *context;
    celix_log_helper_t *loghelper;

    celix_thread_rwlock_t exportedServicesLock;
    hash_map_pt exportedServices;

    //NOTE stopExportsMutex, stopExports, stopExportsActive, stopExportsCond and stopExportsThread are only used if CELIX_RSA_USE_STOP_EXPORT_THREAD is set to true
    celix_thread_mutex_t stopExportsMutex;
    celix_array_list_t *stopExports;
    bool stopExportsActive;
    celix_thread_cond_t stopExportsCond;
    celix_thread_t stopExportsThread;

    celix_thread_mutex_t importedServicesLock;
    celix_array_list_t* importedServices;
    celix_string_hash_map_t* importedEndpointUrls;//key: endpointId, value: celix_imported_endpoint_url_ref_t*

    char *port;
    char *ip;
    char *discoveryInterface;
    bool dynamicIpSupport;

    struct mg_context *ctx;

    FILE *logFile;

    bool curlShareEnabled;
    void *curlShare;
    pthread_mutex_t curlMutexConnect;
    pthread_mutex_t curlMutexCookie;
    pthread_mutex_t curlMutexDns;
};

struct celix_post_data {
    const char *readptr;
    size_t size;
    size_t read;
};

struct celix_get_data_reply {
    FILE* stream;
    char* buf;
    size_t size;
};

#define CELIX_RSA_REMOTE_PROXY_FACTORY   "remote_proxy_factory"
#define CELIX_RSA_REMOTE_PROXY_TIMEOUT   "remote_proxy_timeout"

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
static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_t *endpointDescription, char *request, celix_properties_t *metadata, char **reply);
static celix_status_t remoteServiceAdmin_getIpAddress(char* interface, char** ip);
static char* remoteServiceAdmin_getIFNameForIP(const char *ip);
static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp);
static void remoteServiceAdmin_setupStopExportsThread(remote_service_admin_t* admin);
static void remoteServiceAdmin_teardownStopExportsThread(remote_service_admin_t* admin);

static void remoteServiceAdmin_curlshare_lock(CURL *handle, curl_lock_data data, curl_lock_access laccess, void *userptr)
{
    (void)handle;
    (void)data;
    (void)laccess;
    remote_service_admin_t *rsa = userptr;
    switch(data) {
        case CURL_LOCK_DATA_CONNECT:
            pthread_mutex_lock(&rsa->curlMutexConnect);
            break;
        case CURL_LOCK_DATA_COOKIE:
            pthread_mutex_lock(&rsa->curlMutexCookie);
            break;
        case CURL_LOCK_DATA_DNS:
            pthread_mutex_lock(&rsa->curlMutexDns);
            break;
        default:
            break;
    }
}

static void remoteServiceAdmin_curlshare_unlock(CURL *handle, curl_lock_data data, void *userptr)
{
    (void)handle;
    (void)data;
    remote_service_admin_t *rsa = userptr;
    switch(data) {
        case CURL_LOCK_DATA_CONNECT:
            pthread_mutex_unlock(&rsa->curlMutexConnect);
            break;
        case CURL_LOCK_DATA_COOKIE:
            pthread_mutex_unlock(&rsa->curlMutexCookie);
            break;
        case CURL_LOCK_DATA_DNS:
            pthread_mutex_unlock(&rsa->curlMutexDns);
            break;
        default:
            break;
    }
}

celix_status_t remoteServiceAdmin_create(celix_bundle_context_t *context, remote_service_admin_t **admin) {
    celix_status_t status = CELIX_SUCCESS;

    *admin = calloc(1, sizeof(**admin));
    if (!*admin) {
        status = CELIX_ENOMEM;
    } else {
        (*admin)->context = context;
        (*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->importedServices = celix_arrayList_create();

        celixThreadRwlock_create(&(*admin)->exportedServicesLock, NULL);
        celixThreadMutex_create(&(*admin)->importedServicesLock, NULL);

        (*admin)->importedEndpointUrls = celix_stringHashMap_create(); // Ignore ENOMEM for now

        (*admin)->loghelper = celix_logHelper_create(context, "celix_rsa_admin");

        long port = celix_bundleContext_getPropertyAsLong(context, RSA_PORT_KEY, RSA_PORT_DEFAULT);
        const char *ip = celix_bundleContext_getProperty(context, RSA_IP_KEY, RSA_IP_DEFAULT);
        const char *interface = celix_bundleContext_getProperty(context, RSA_INTERFACE_KEY, NULL);
        (*admin)->curlShareEnabled = celix_bundleContext_getPropertyAsBool(context, RSA_DFI_USE_CURL_SHARE_HANDLE, RSA_DFI_USE_CURL_SHARE_HANDLE_DEFAULT);
        (*admin)->dynamicIpSupport = celix_bundleContext_getPropertyAsBool(context, CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT, CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT_DEFAULT);

        char *detectedIp = NULL;
        if ((interface != NULL) && (remoteServiceAdmin_getIpAddress((char*)interface, &detectedIp) != CELIX_SUCCESS)) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_WARNING, "RSA: Could not retrieve IP address for interface %s", interface);
        }
        if (detectedIp != NULL) {
            ip = detectedIp;
            (*admin)->discoveryInterface = celix_utils_strdup(interface);
        } else {
            (*admin)->discoveryInterface = remoteServiceAdmin_getIFNameForIP(ip);
        }

        if (ip != NULL) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_DEBUG, "RSA: Using %s for service annunciation", ip);
            (*admin)->ip = strdup(ip);
        } else {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_WARNING, "RSA: No IP address found, using default: %s", RSA_IP_DEFAULT);
            (*admin)->ip = strdup(RSA_IP_DEFAULT);
        }

        if (detectedIp != NULL) {
            free(detectedIp);
        }

        (*admin)->curlShare = curl_share_init();
        if ((*admin)->curlShare == NULL) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR, "Failed to initialize cURL share handle.");
            status = CELIX_BUNDLE_EXCEPTION;
        } else {
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_USERDATA, *admin);
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_LOCKFUNC, remoteServiceAdmin_curlshare_lock);
            curl_share_setopt((*admin)->curlShare, CURLSHOPT_UNLOCKFUNC, remoteServiceAdmin_curlshare_unlock);
        }

        if (status == CELIX_SUCCESS && pthread_mutex_init(&(*admin)->curlMutexConnect, NULL) != 0) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR, "Could not initialize mutex connect");
            status = CELIX_BUNDLE_EXCEPTION;
        }

        if (status == CELIX_SUCCESS && pthread_mutex_init(&(*admin)->curlMutexCookie, NULL) != 0) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR, "Could not initialize mutex cookie");
            status = CELIX_BUNDLE_EXCEPTION;
        }

        if (status == CELIX_SUCCESS && pthread_mutex_init(&(*admin)->curlMutexDns, NULL) != 0) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR, "Could not initialize mutex dns");
            status = CELIX_BUNDLE_EXCEPTION;
        }

        if (status == CELIX_SUCCESS) {
            remoteServiceAdmin_setupStopExportsThread(*admin);
        }

        struct mg_callbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.begin_request = remoteServiceAdmin_callback;

        char newPort[10];
        snprintf(newPort, 10, "%li", port);

        unsigned int port_counter = 0;
        bool bindToAllInterfaces = celix_bundleContext_getPropertyAsBool(context, CELIX_RSA_BIND_ON_ALL_INTERFACES, CELIX_RSA_BIND_ON_ALL_INTERFACES_DEFAULT);

        do {
            char *listeningPorts = NULL;
            if ((*admin)->dynamicIpSupport || bindToAllInterfaces) {
                asprintf(&listeningPorts, "0.0.0.0:%s", newPort);
            } else {
                asprintf(&listeningPorts, "%s:%s", (*admin)->ip, newPort);
            }

            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_INFO,
                "Binding webserver using IP: %s, Port: %s, Dynamic IP Support: %s, Bind To All Interfaces: %s",
                (*admin)->ip ? (*admin)->ip : "0.0.0.0", newPort,
                (*admin)->dynamicIpSupport ? "Enabled" : "Disabled",
                bindToAllInterfaces ? "Enabled" : "Disabled");

            const char *options[] = { "listening_ports", listeningPorts, "num_threads", "5", NULL };

            (*admin)->ctx = mg_start(&callbacks, (*admin), options);

            if ((*admin)->ctx != NULL) {
                celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_INFO, "RSA: Start webserver: %s", listeningPorts);
                (*admin)->port = strdup(newPort);
            } else {
                celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR, "Error while starting rsa server on port %s - retrying on port %li...", newPort, port + port_counter);
                snprintf(newPort, 10, "%li", port + port_counter++);
            }

            free(listeningPorts);

        } while (((*admin)->ctx == NULL) && (port_counter < MAX_NUMBER_OF_RESTARTS));

        if ((*admin)->ctx == NULL && port_counter >= MAX_NUMBER_OF_RESTARTS) {
            celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_ERROR,
                "Failed to start RSA server after %d retries. Last attempted port: %s", MAX_NUMBER_OF_RESTARTS, newPort);
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    bool logCalls = celix_bundleContext_getPropertyAsBool(context, RSA_LOG_CALLS_KEY, RSA_LOG_CALLS_DEFAULT);
    if (logCalls) {
        const char *f = celix_bundleContext_getProperty(context, RSA_LOG_CALLS_FILE_KEY, RSA_LOG_CALLS_FILE_DEFAULT);
        if (strncmp(f, "stdout", strlen("stdout")) == 0) {
            (*admin)->logFile = stdout;
        } else {
            (*admin)->logFile = fopen(f, "w");
            if ((*admin)->logFile == NULL) {
                celix_logHelper_log((*admin)->loghelper, CELIX_LOG_LEVEL_WARNING, "Error opening file '%s' for logging calls. %s", f, strerror(errno));
            }
        }
    }

    if (status != CELIX_SUCCESS) {
        // Cleanup on failure
        if (*admin) {
            celix_arrayList_destroy((*admin)->importedServices);
            celix_stringHashMap_destroy((*admin)->importedEndpointUrls);
            celix_logHelper_destroy((*admin)->loghelper);
            if ((*admin)->curlShare) {
                curl_share_cleanup((*admin)->curlShare);
            }
            free(*admin);
            *admin = NULL;
        }
    }

    return status;
}


celix_status_t remoteServiceAdmin_destroy(remote_service_admin_t **admin) {
    celix_status_t status = CELIX_SUCCESS;

    celix_bundleContext_waitForEvents((*admin)->context);

    if ( (*admin)->logFile != NULL && (*admin)->logFile != stdout) {
        fclose((*admin)->logFile);
    }
    free((*admin)->discoveryInterface);
    free((*admin)->ip);
    free((*admin)->port);
    curl_share_cleanup((*admin)->curlShare);
    pthread_mutex_destroy(&(*admin)->curlMutexConnect);
    pthread_mutex_destroy(&(*admin)->curlMutexCookie);
    pthread_mutex_destroy(&(*admin)->curlMutexDns);
    CELIX_STRING_HASH_MAP_ITERATE((*admin)->importedEndpointUrls, iter) {
        celix_imported_endpoint_url_ref_t* urlRef = iter.value.ptrValue;
        celix_ref_put(&urlRef->ref, (void*)free);
    }
    celix_stringHashMap_destroy((*admin)->importedEndpointUrls);
    celixThreadRwlock_destroy(&(*admin)->exportedServicesLock);
    celixThreadMutex_destroy(&(*admin)->importedServicesLock);
    free(*admin);

    *admin = NULL;

    return status;
}

void* remoteServiceAdmin_stopExportsThread(void *data) {
    remote_service_admin_t* admin = data;
    bool active = true;

    while (active) {
        celixThreadMutex_lock(&admin->stopExportsMutex);
        if (admin->stopExportsActive && celix_arrayList_size(admin->stopExports) == 0) {
            celixThreadCondition_timedwaitRelative(&admin->stopExportsCond, &admin->stopExportsMutex, 1, 0);
        }
        for (int i = 0; i < celix_arrayList_size(admin->stopExports); ++i) {
            export_registration_t *export = celix_arrayList_get(admin->stopExports, i);
            exportRegistration_destroy(export);
        }
        celix_arrayList_clear(admin->stopExports);
        active = admin->stopExportsActive;
        celixThreadMutex_unlock(&admin->stopExportsMutex);
    }

    return NULL;
}

static void remoteServiceAdmin_setupStopExportsThread(remote_service_admin_t* admin) {
    if (CELIX_RSA_USE_STOP_EXPORT_THREAD) {
        //setup exports stop thread
        celixThreadMutex_create(&admin->stopExportsMutex, NULL);
        admin->stopExports = celix_arrayList_create();
        celixThreadCondition_init(&admin->stopExportsCond, NULL);
        admin->stopExportsActive = true;
        celixThread_create(&admin->stopExportsThread, NULL, remoteServiceAdmin_stopExportsThread, admin);
    }
}

static void remoteServiceAdmin_teardownStopExportsThread(remote_service_admin_t* admin) {
    if (CELIX_RSA_USE_STOP_EXPORT_THREAD) {
        celixThreadMutex_lock(&admin->stopExportsMutex);
        admin->stopExportsActive = false;
        celixThreadCondition_broadcast(&admin->stopExportsCond);
        celixThreadMutex_unlock(&admin->stopExportsMutex);
        celixThread_join(admin->stopExportsThread, NULL);
        celix_arrayList_destroy(admin->stopExports);
        celixThreadMutex_destroy(&admin->stopExportsMutex);
        celixThreadCondition_destroy(&admin->stopExportsCond);
    }
}

static void remoteServiceAdmin_stopExport(remote_service_admin_t *admin, export_registration_t* export) {
    if (export != NULL) {
        if (CELIX_RSA_USE_STOP_EXPORT_THREAD) {
            celixThreadMutex_lock(&admin->stopExportsMutex);
            exportRegistration_setActive(export, false);
            celix_arrayList_add(admin->stopExports, export);
            celixThreadCondition_broadcast(&admin->stopExportsCond);
            celixThreadMutex_unlock(&admin->stopExportsMutex);
        } else {
            exportRegistration_waitTillNotUsed(export);
            exportRegistration_destroy(export);
        }
    }
}

celix_status_t remoteServiceAdmin_stop(remote_service_admin_t *admin) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_writeLock(&admin->exportedServicesLock);

    hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);
    while (hashMapIterator_hasNext(iter)) {
        celix_array_list_t *exports = hashMapIterator_nextValue(iter);
        int i;
        for (i = 0; i < celix_arrayList_size(exports); i++) {
            export_registration_t *export = celix_arrayList_get(exports, i);
            remoteServiceAdmin_stopExport(admin, export);
        }
        celix_arrayList_destroy(exports);
    }
    hashMapIterator_destroy(iter);
    celixThreadRwlock_unlock(&admin->exportedServicesLock);

    remoteServiceAdmin_teardownStopExportsThread(admin);

    celixThreadMutex_lock(&admin->importedServicesLock);
    int i;
    int size = celix_arrayList_size(admin->importedServices);
    for (i = 0; i < size ; i += 1) {
        import_registration_t *import = celix_arrayList_get(admin->importedServices, i);
        if (import != NULL) {
            importRegistration_destroy(import);
        }
    }
    celixThreadMutex_unlock(&admin->importedServicesLock);

    if (admin->ctx != NULL) {
        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA: Stopping webserver...");
        mg_stop(admin->ctx);
        admin->ctx = NULL;
    }

    hashMap_destroy(admin->exportedServices, false, false);
    celix_arrayList_destroy(admin->importedServices);

    celix_logHelper_destroy(admin->loghelper);

    return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
//void *remoteServiceAdmin_callback(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) {

celix_status_t importRegistration_getFactory(import_registration_t *import, service_factory_pt *factory);

static int remoteServiceAdmin_callback(struct mg_connection *conn) {
    int result = 1; // zero means: let civetweb handle it further, any non-zero value means it is handled by us...
    export_registration_t *export = NULL;
    celix_properties_t *metadata = NULL;

    const struct mg_request_info *request_info = mg_get_request_info(conn);
    if (request_info->request_uri != NULL) {
        remote_service_admin_t *rsa = request_info->user_data;


        if (strncmp(request_info->request_uri, "/service/", 9) == 0 && strcmp("POST", request_info->request_method) == 0) {

            // uri = /services/myservice/call
            const char *uri = request_info->request_uri;
            // rest = myservice/call

            const char *rest = uri+9;
            char *interfaceStart = strchr(rest, '/');
            int pos = interfaceStart - rest;
            char service[pos+1];
            strncpy(service, rest, pos);
            service[pos] = '\0';
            unsigned long serviceId = strtoul(service,NULL,10);

            for (int i = 0; i < request_info->num_headers; i++) {
                struct mg_header header = request_info->http_headers[i];
                if (strncmp(header.name, "X-RSA-Metadata-", 15) == 0) {
                    if (metadata == NULL) {
                        metadata = celix_properties_create();
                    }
                    celix_properties_set(metadata, header.name + 15, header.value);
                }
            }

            celixThreadRwlock_readLock(&rsa->exportedServicesLock);

            //find endpoint
            hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
                celix_array_list_t *exports = hashMapEntry_getValue(entry);
                int expIt = 0;
                for (expIt = 0; expIt < celix_arrayList_size(exports); expIt++) {
                    export_registration_t *check = celix_arrayList_get(exports, expIt);
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
                exportRegistration_increaseUsage(export);
            } else {
                result = 0;
                RSA_LOG_WARNING(rsa, "No export registration found for service id %lu", serviceId);
            }
            celixThreadRwlock_unlock(&rsa->exportedServicesLock);
        }


        if (export != NULL) {
            uint64_t datalength = request_info->content_length;
            char* data = malloc(datalength + 1);
            mg_read(conn, data, datalength);
            data[datalength] = '\0';

            char *response = NULL;
            int responceLength = 0;
            int rc = exportRegistration_call(export, data, -1, &metadata, &response, &responceLength);
            if (rc != CELIX_SUCCESS) {
                RSA_LOG_ERROR(rsa, "Error trying to invoke remove service, got error %i\n", rc);
            }

            if (rc == CELIX_SUCCESS && response != NULL) {
                mg_write(conn, data_response_headers, strlen(data_response_headers));

                char *bufLoc = response;
                size_t bytesLeft = strlen(response);
                if (bytesLeft > INT_MAX) {
                    //NOTE arcording to civetweb mg_write, there is a limit on mg_write for INT_MAX.
                    RSA_LOG_WARNING(rsa, "nr of bytes to send for a remote call is > INT_MAX, this can lead to issues\n");
                }
                while (bytesLeft > 0) {
                    int send = mg_write(conn, bufLoc, strlen(bufLoc));
                    if (send > 0) {
                        bytesLeft -= send;
                        bufLoc += send;
                    } else {
                        RSA_LOG_ERROR(rsa, "Error sending response: %s", strerror(errno));
                        break;
                    }
                }

                free(response);
            } else {
                mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
            }
            result = 1;

            free(data);
            exportRegistration_decreaseUsage(export);
        }
    }

    //free metadata
    if(metadata != NULL) {
        celix_properties_destroy(metadata);
    }

    return result;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t *admin, char *serviceId, celix_properties_t *properties, celix_array_list_t** registrationsOut) {
    celix_status_t status = CELIX_SUCCESS;

    bool export = false;
    const char *exportConfigs = celix_properties_get(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_DFI_CONFIGURATION_TYPE);
    if (exportConfigs != NULL) {
        // See if the EXPORT_CONFIGS matches this RSA. If so, try to export.

        char *ecCopy = strndup(exportConfigs, strlen(exportConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(celix_utils_trimInPlace(token), RSA_DFI_CONFIGURATION_TYPE, 1024) == 0) {
                export = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    } else {
        export = true;
    }

    celix_array_list_t *registrations = NULL;
    if (export) {
        registrations = celix_arrayList_create();
        celix_array_list_t* references = NULL;
        service_reference_pt reference = NULL;
        char filter[256];

        snprintf(filter, 256, "(%s=%s)", (char *) CELIX_FRAMEWORK_SERVICE_ID, serviceId);

        status = bundleContext_getServiceReferences(admin->context, NULL, filter, &references);

        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "RSA: exportService called for serviceId %s", serviceId);

        int i;
        int size = celix_arrayList_size(references);
        for (i = 0; i < size; i += 1) {
            if (i == 0) {
                reference = celix_arrayList_get(references, i);
            } else {
                bundleContext_ungetServiceReference(admin->context, celix_arrayList_get(references, i));
            }
        }
        celix_arrayList_destroy(references);

        if (reference == NULL) {
            celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "ERROR: expected a reference for service id %s.",
                          serviceId);
            status = CELIX_ILLEGAL_STATE;
        }

        const char *exports = NULL;
        const char *provided = NULL;
        if (status == CELIX_SUCCESS) {
            serviceReference_getProperty(reference, (char *) CELIX_RSA_SERVICE_EXPORTED_INTERFACES, &exports);
            serviceReference_getProperty(reference, (char *) CELIX_FRAMEWORK_SERVICE_NAME, &provided);

            if (exports == NULL || provided == NULL || strcmp(exports, provided) != 0) {
                celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "RSA: No Services to export.");
                status = CELIX_ILLEGAL_STATE;
            } else {
                celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA: Export service (%s)", provided);
            }
        }

        if (status == CELIX_SUCCESS) {
            const char *interface = provided;
            endpoint_description_t *endpoint = NULL;
            export_registration_t *registration = NULL;

            remoteServiceAdmin_createEndpointDescription(admin, reference, properties, (char *) interface, &endpoint);
            status = exportRegistration_create(admin->loghelper, reference, endpoint, admin->context, admin->logFile,
                                               &registration);
            if (status == CELIX_SUCCESS) {
                status = exportRegistration_start(registration);
                if (status == CELIX_SUCCESS) {
                    celix_arrayList_add(registrations, registration);
                }
            }
        }


        if (status == CELIX_SUCCESS && celix_arrayList_size(registrations) > 0) {
            celixThreadRwlock_writeLock(&admin->exportedServicesLock);
            hashMap_put(admin->exportedServices, reference, registrations);
            celixThreadRwlock_unlock(&admin->exportedServicesLock);
        } else {
            celix_arrayList_destroy(registrations);
            registrations = NULL;
        }
    }

    if (status == CELIX_SUCCESS) {
        //We return a empty list of registrations if Remote Service Admin does not recognize any of the configuration types.
    	celix_array_list_t *newRegistrations = celix_arrayList_create();
        if (registrations != NULL) {
            int regSize = celix_arrayList_size(registrations);
            for (int i = 0; i < regSize; ++i) {
                celix_arrayList_add(newRegistrations, celix_arrayList_get(registrations, i));
            }
        }
        *registrationsOut = newRegistrations;
    }

    return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(remote_service_admin_t *admin, export_registration_t *registration) {
    celix_status_t status;

    celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA_DFI: Removing exported service");

    export_reference_t * ref = NULL;
    status = exportRegistration_getExportReference(registration, &ref);

    if (status == CELIX_SUCCESS && ref != NULL) {
        service_reference_pt servRef;
        celixThreadRwlock_writeLock(&admin->exportedServicesLock);
        exportReference_getExportedService(ref, &servRef);

        celix_array_list_t *exports = (celix_array_list_t *)hashMap_get(admin->exportedServices, servRef);
        if (exports != NULL) {
            celix_arrayList_remove(exports, registration);
            if (celix_arrayList_size(exports) == 0) {
                hashMap_remove(admin->exportedServices, servRef);
                celix_arrayList_destroy(exports);
            }
        }

        remoteServiceAdmin_stopExport(admin, registration);
        celixThreadRwlock_unlock(&admin->exportedServicesLock);

        free(ref);

    } else {
        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Cannot find reference for registration");
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
            && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_INTERFACES) != 0
            && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_CONFIGS) != 0
            && strcmp(key, (char*) CELIX_FRAMEWORK_SERVICE_NAME) != 0) {
            celix_properties_set(endpointProperties, key, value);
        }
    }

    const char* serviceId = celix_properties_get(endpointProperties, CELIX_FRAMEWORK_SERVICE_ID, "-1");
    const char *uuid = NULL;

    char buf[512];
    snprintf(buf, 512,  "/service/%s/%s", serviceId, interface);

    char url[1024];
    snprintf(url, 1024, "http://%s:%s%s", admin->ip, admin->port, buf);

    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);

    bundleContext_getProperty(admin->context, CELIX_FRAMEWORK_UUID, &uuid);
    celix_properties_set(endpointProperties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    celix_properties_set(endpointProperties, CELIX_FRAMEWORK_SERVICE_NAME, interface);
    celix_properties_set(endpointProperties, CELIX_RSA_ENDPOINT_SERVICE_ID, serviceId);
    celix_properties_set(endpointProperties, CELIX_RSA_ENDPOINT_ID, endpoint_uuid);
    celix_properties_set(endpointProperties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(endpointProperties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, RSA_DFI_CONFIGURATION_TYPE);
    celix_properties_set(endpointProperties, RSA_DFI_ENDPOINT_URL, url);
    if (admin->discoveryInterface != NULL) {
        celix_properties_set(endpointProperties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, admin->discoveryInterface);
    }
    if (admin->dynamicIpSupport) {
        celix_properties_set(endpointProperties, RSA_DFI_ENDPOINT_URL, buf);
        celix_properties_set(endpointProperties, CELIX_RSA_PORT, admin->port);
    }

    if (props != NULL) {
        CELIX_PROPERTIES_ITERATE(props, iter) {
            celix_properties_set(endpointProperties, iter.key, iter.entry.value);
        }
    }

    *endpoint = calloc(1, sizeof(**endpoint));
    if (!*endpoint) {
        status = CELIX_ENOMEM;
    } else {
        (*endpoint)->id = (char*) celix_properties_get(endpointProperties, (char*) CELIX_RSA_ENDPOINT_ID, NULL);
        (*endpoint)->serviceId = serviceReference_getServiceId(reference);
        (*endpoint)->frameworkUUID = (char*) celix_properties_get(endpointProperties, (char*) CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
        (*endpoint)->serviceName = strndup(interface, 1024*10);
        (*endpoint)->properties = endpointProperties;
    }

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

static char* remoteServiceAdmin_getIFNameForIP(const char *ip) {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    char *ifName = NULL;
    if (ip != NULL && getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0)) {
                if (strcmp(host, ip) == 0) {
                    ifName = celix_utils_strdup(ifa->ifa_name);
                    break;
                }
            }
        }

        freeifaddrs(ifaddr);
    }
    return ifName;
}

celix_status_t remoteServiceAdmin_destroyEndpointDescription(endpoint_description_t **description)
{
    celix_status_t status = CELIX_SUCCESS;

    celix_properties_destroy((*description)->properties);
    free((*description)->serviceName);
    free(*description);

    return status;
}


celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t *admin, celix_array_list_t** services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t *admin, celix_array_list_t** services) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, import_registration_t **out) {
    celix_status_t status = CELIX_SUCCESS;

    bool importService = false;
    const char *importConfigs = celix_properties_get(endpointDescription->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (importConfigs != NULL) {
        // Check whether this RSA must be imported
        char *ecCopy = strndup(importConfigs, strlen(importConfigs));
        const char delimiter[2] = ",";
        char *token, *savePtr;

        token = strtok_r(ecCopy, delimiter, &savePtr);
        while (token != NULL) {
            if (strncmp(celix_utils_trimInPlace(token), RSA_DFI_CONFIGURATION_TYPE, 1024) == 0) {
                importService = true;
                break;
            }

            token = strtok_r(NULL, delimiter, &savePtr);
        }

        free(ecCopy);
    } else {
        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "Mandatory %s element missing from endpoint description",
                            CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
    }

    if (importService) {
        import_registration_t *import = NULL;

        const char *objectClass = celix_properties_get(endpointDescription->properties, "objectClass", NULL);
        const char *serviceVersion = celix_properties_get(endpointDescription->properties, CELIX_FRAMEWORK_SERVICE_VERSION, NULL);

        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA: Import service %s", endpointDescription->serviceName);
        celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "Registering service factory (proxy) for service '%s'\n",
                      objectClass);

        if (objectClass != NULL) {
            status = importRegistration_create(admin->context, endpointDescription, objectClass, serviceVersion,
                                               (send_func_type )remoteServiceAdmin_send, admin,
                                               admin->logFile,
                                               &import);
        }

        if (status == CELIX_SUCCESS && import != NULL) {
            status = importRegistration_start(import);
        }

        celixThreadMutex_lock(&admin->importedServicesLock);
        celix_arrayList_add(admin->importedServices, import);
        celixThreadMutex_unlock(&admin->importedServicesLock);

        if (status == CELIX_SUCCESS) {
            *out = import;
        }
    }

    return status;
}

static void remoteServiceAdmin_removeDynamicIpUrlOfImportedRegistration(remote_service_admin_t* admin, import_registration_t *registration) {
    endpoint_description_t* endpoint = importRegistration_getEndpointDescription(registration);
    assert(endpoint != NULL);
    celix_imported_endpoint_url_ref_t* urlRef = celix_stringHashMap_get(admin->importedEndpointUrls, endpoint->id);
    if (urlRef != NULL) {
        celix_ref_put(&urlRef->ref, (void*)free);
        celix_stringHashMap_remove(admin->importedEndpointUrls, endpoint->id);
    }
    return;
}

celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_t *admin, import_registration_t *registration) {
    celix_status_t status = CELIX_SUCCESS;
    celix_logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA_DFI: Removing imported service");

    celixThreadMutex_lock(&admin->importedServicesLock);
    int i;
    int size = celix_arrayList_size(admin->importedServices);
    import_registration_t * current  = NULL;
    for (i = 0; i < size; i += 1) {
        current = celix_arrayList_get(admin->importedServices, i);
        if (current == registration) {
            celix_arrayList_removeAt(admin->importedServices, i);
            remoteServiceAdmin_removeDynamicIpUrlOfImportedRegistration(admin, registration);
            importRegistration_destroy(current);
            break;
        }
    }
    celixThreadMutex_unlock(&admin->importedServicesLock);

    return status;
}

static bool shouldTryUseOtherDynamicIp(celix_status_t useUrlStatus) {
    switch (useUrlStatus) {
        case CELIX_SUCCESS:
        case CELIX_ERROR_MAKE(CELIX_FACILITY_CURL,CURLE_OUT_OF_MEMORY):
            return false;
        default:
            return true;
    }
}

static celix_status_t remoteServiceAdmin_useDynamicIpUrlsForEndpoint(remote_service_admin_t* rsa, endpoint_description_t* endpointDescription,
celix_status_t (*useUrl)(void* handle, const char* url), void* handle) {
    celix_status_t status = CELIX_ERROR_MAKE(CELIX_FACILITY_CURL,CURLE_COULDNT_CONNECT);
    celixThreadMutex_lock(&rsa->importedServicesLock);
    celix_imported_endpoint_url_ref_t* urlRef = celix_stringHashMap_get(rsa->importedEndpointUrls, endpointDescription->id);
    if (urlRef != NULL) {
        celix_ref_get(&urlRef->ref);
        celixThreadMutex_unlock(&rsa->importedServicesLock);
        status = useUrl(handle, urlRef->url);
        celix_ref_put(&urlRef->ref, (void*)free);
    } else {
        celixThreadMutex_unlock(&rsa->importedServicesLock);
    }
    if (!shouldTryUseOtherDynamicIp(status)) {
        return status;
    }

    const char *serviceUrl = celix_properties_get(endpointDescription->properties, (char*) RSA_DFI_ENDPOINT_URL, "");
    const char *ipaddresses = celix_properties_get(endpointDescription->properties, CELIX_RSA_IP_ADDRESSES, NULL);
    int port = (int)celix_properties_getAsLong(endpointDescription->properties, CELIX_RSA_PORT, RSA_PORT_DEFAULT);
    celix_autofree char *ipStrListCopy = NULL;
    ipStrListCopy = celix_utils_strdup(ipaddresses);
    if (ipStrListCopy == NULL) {
        celix_logHelper_error(rsa->loghelper, "Error duplicating ip addresses");
        return CELIX_ENOMEM;
    }
    char url[256] = {0};
    char *savePtr = NULL;
    char *token = strtok_r(ipStrListCopy, ",", &savePtr);
    while (token != NULL) {
        char *ip = celix_utils_trimInPlace(token);
        struct sockaddr_in sa;
        if (inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1) {
            snprintf(url, sizeof(url), "http://%s:%d%s", ip, port, serviceUrl);
        } else {
            snprintf(url, sizeof(url), "http://[%s]:%d%s", ip, port, serviceUrl);
        }
        status = useUrl(handle, url);
        if (!shouldTryUseOtherDynamicIp(status)) {
            celix_imported_endpoint_url_ref_t* urlRefNew = calloc(1, sizeof(*urlRefNew) + strlen(url) + 1);
            if (urlRefNew == NULL) {
                celix_logHelper_error(rsa->loghelper, "Error allocating memory for imported endpoint url reference");
                return CELIX_ENOMEM;
            }
            celix_ref_init(&urlRefNew->ref);
            strcpy(urlRefNew->url, url);

            celixThreadMutex_lock(&rsa->importedServicesLock);
            if (celix_stringHashMap_put(rsa->importedEndpointUrls, endpointDescription->id, urlRefNew) != CELIX_SUCCESS) {
                celix_logHelper_logTssErrors(rsa->loghelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(rsa->loghelper, "Error storing imported endpoint url reference for endpoint %s", endpointDescription->id);
                celix_ref_put(&urlRefNew->ref, (void*)free);
            }else if (urlRef != NULL) {
                celix_ref_put(&urlRef->ref, (void*)free);
            }
            celixThreadMutex_unlock(&rsa->importedServicesLock);
            break;
        }
        token = strtok_r(NULL, ",", &savePtr);
    }
    return status;
}

static celix_status_t onUseUrlCallback(void* handle, const char* url) {
    CURL *curl = handle;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    CURLcode ret = curl_easy_perform(curl);
    if (ret != CURLE_OK) {
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CURL, ret);
    }
    return CELIX_SUCCESS;
}

static celix_status_t remoteServiceAdmin_send(void *handle, endpoint_description_t *endpointDescription, char *request, celix_properties_t *metadata, char **reply) {
    remote_service_admin_t * rsa = handle;
    struct celix_post_data post;
    post.readptr = request;
    post.size = strlen(request);
    post.read = 0;

    struct celix_get_data_reply get;
    get.buf = NULL;
    get.size = 0;
    get.stream = open_memstream(&get.buf, &get.size);

    const char *serviceUrl = celix_properties_get(endpointDescription->properties, (char*) RSA_DFI_ENDPOINT_URL, NULL);
    char url[256];
    snprintf(url, 256, "%s", serviceUrl);

    // assume the default timeout
    int timeout = DEFAULT_TIMEOUT;

    const char *timeoutStr = NULL;
    // Check if the endpoint has a timeout, if so, use it.
    timeoutStr = (char*) celix_properties_get(endpointDescription->properties, (char*) CELIX_RSA_REMOTE_PROXY_TIMEOUT, NULL);
    if (timeoutStr == NULL) {
        // If not, get the global variable and use that one.
        bundleContext_getProperty(rsa->context, (char*) CELIX_RSA_REMOTE_PROXY_TIMEOUT, &timeoutStr);
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
        fclose(get.stream);
        free(get.buf);
        status = CELIX_ILLEGAL_STATE;
    } else {
        struct curl_slist *metadataHeader = NULL;
        if (metadata != NULL && celix_properties_size(metadata) > 0) {
            CELIX_PROPERTIES_ITERATE(metadata, iter) {
                size_t length = strlen(iter.key) + strlen(iter.entry.value) + 18; // "X-RSA-Metadata-key: val\0"
                char header[length];
                snprintf(header, length, "X-RSA-Metadata-%s: %s", iter.key, iter.entry.value);
                metadataHeader = curl_slist_append(metadataHeader, header);
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, metadataHeader);
        }

        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, remoteServiceAdmin_readCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &post);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, remoteServiceAdmin_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&get);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)post.size);
        if (rsa->curlShareEnabled) {
            curl_easy_setopt(curl, CURLOPT_SHARE, rsa->curlShare);
        }

        const char *ipaddresses = celix_properties_get(endpointDescription->properties, CELIX_RSA_IP_ADDRESSES, NULL);
        if (ipaddresses == NULL) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            res = curl_easy_perform(curl);
            status = (res == CURLE_OK) ? CELIX_SUCCESS:CELIX_ERROR_MAKE(CELIX_FACILITY_CURL,res);
        } else {
            status = remoteServiceAdmin_useDynamicIpUrlsForEndpoint(rsa, endpointDescription, onUseUrlCallback, curl);
        }

        fputc('\0', get.stream);
        fclose(get.stream);
        *reply = get.buf;
        if (status == CELIX_SUCCESS) {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            status = (httpCode == 200/*HTTP OK*/) ? CELIX_SUCCESS : CELIX_ERROR_MAKE(CELIX_FACILITY_HTTP,httpCode);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(metadataHeader);
    }

    return status;
}

static size_t remoteServiceAdmin_readCallback(void *voidBuffer, size_t size, size_t nmemb, void *userp) {
    struct celix_post_data *post = userp;
    size_t buffSize = size * nmemb;
    size_t readSize = post->size - post->read;
    if (readSize > buffSize) {
        readSize = buffSize;
    }
    void *startRead = (void*)(post->readptr + post->read);
    memcpy(voidBuffer, startRead, readSize);
    post->read += readSize;
    return readSize;
}

static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp) {
    struct celix_get_data_reply *get = userp;
    fwrite(contents, size, nmemb, get->stream);
    return size * nmemb;
}

