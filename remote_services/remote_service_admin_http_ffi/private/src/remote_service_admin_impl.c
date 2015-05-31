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
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uuid/uuid.h>

#include <curl/curl.h>

#include <ffi.h>
#include <jansson.h>

#include "export_registration_impl.h"
#include "import_registration_impl.h"
#include "remote_service_admin_impl.h"
#include "remote_constants.h"
#include "constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "bundle.h"
#include "service_reference.h"
#include "service_registration.h"
#include "log_helper.h"
#include "log_service.h"
#include "celix_threads.h"
#include "civetweb.h"
#include "log_helper.h"
#include "endpoint_description.h"

// defines how often the webserver is restarted (with an increased port number)
#define MAX_NUMBER_OF_RESTARTS 	5

typedef void (*GEN_FUNC_TYPE)(void);

struct generic_service_layout {
    void *handle;
    GEN_FUNC_TYPE functions[];
};

struct proxy {
  remote_service_admin_pt admin;
  char *sig;
  int index;
  ffi_cif cif;
  ffi_closure *closure;
  endpoint_description_pt endpointDescription
};


    


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

static celix_status_t remoteServiceAdmin_getIpAdress(char* interface, char** ip);

static size_t remoteServiceAdmin_readCallback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t remoteServiceAdmin_write(void *contents, size_t size, size_t nmemb, void *userp);

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
			exportRegistration_stopTracking(export);
		}
	}
    hashMapIterator_destroy(iter);
    celixThreadMutex_unlock(&admin->exportedServicesLock);

    celixThreadMutex_lock(&admin->importedServicesLock);

    iter = hashMapIterator_create(admin->importedServices);
    while (hashMapIterator_hasNext(iter))
    {
    	hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

    	import_registration_factory_pt importFactory = hashMapEntry_getValue(entry);

        if (importFactory != NULL) {
            int i;
            for (i = 0; i < arrayList_size(importFactory->registrations); i++)
            {
                import_registration_pt importRegistration = arrayList_get(importFactory->registrations, i);

                if (importFactory->trackedFactory != NULL)
                {
                    importFactory->trackedFactory->unregisterProxyService(importFactory->trackedFactory->factory, importRegistration->endpointDescription);
                }
            }

            serviceTracker_close(importFactory->proxyFactoryTracker);
            importRegistrationFactory_close(importFactory);

            hashMapIterator_remove(iter);
            importRegistrationFactory_destroy(&importFactory);
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

static int remoteServiceAdmin_callback(struct mg_connection *conn) {
	int result = 0; // zero means: let civetweb handle it further, any non-zero value means it is handled by us...

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
					if (serviceId == check->endpointDescription->serviceId) {
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

          //FIXME assuming add signature (add(double, double) : double)
          /*TODO
            export->endpoint->handleRequest(export->endpoint->endpoint, data, &response);
            */


          //retreive schema.
          char *schema = NULL;
          schema = properties_get(export->endpointDescription->properties, "protocol.schema");
          printf("RSA: got schema %s\n", schema);


          printf("Parsing data: %s\n", data);
          json_error_t error;
          json_t *js_request = json_loads(data, 0, &error);
          const char *sig;
          if (js_request) {
              if (json_unpack(js_request, "{s:s}", "m", &sig) != 0) {
                  printf("RSA: Got error '%s'\n", error.text);
              }
          } else {
              printf("RSA: got error '%s' for '%s'\n", error.text, data);
              return 0;
          }

          printf("RSA: Looking for method %s\n", sig);

          json_t *js_schema = json_loads(schema, 0, &error);
          int methodIndex = -1;
          if (js_schema) {
              //TODO parse schema and create cif
              size_t index;
              json_t *value;
              json_array_foreach(js_schema, index, value) {
                  if (strcmp(sig, json_string_value(value)) == 0) {
                      //found match
                      methodIndex = index;
                      break;
                  }
              }
          } else {
              printf("RSA: got error '%s' for '%s'\n", error.text, schema);
              return 0;
          }

          if (methodIndex < 0) {
              printf("RSA: cannot find method '%s'\n", sig);
              return 1;
          }

          size_t sigLength = strlen(sig);
          char provArgs[sigLength];
          bool startFound = false;
          int i = 0;
          int argIndex = 0;
          for (; i < sigLength; i += 1) {
              if (!startFound) {
                  if (sig[i] == '(') {
                      startFound = true;
                  }
              } else {
                  if (sig[i] != ')') {
                      provArgs[argIndex++] = sig[i];
                  }
              }
          } 
          provArgs[argIndex] = '\0';
          size_t provArgsLength = strlen(provArgs) -1; 
          printf("method index is %i and args are '%s'\n", methodIndex, provArgs);


          //FFI part
          ffi_cif cif;
          ffi_type *argTypes[provArgsLength + 2]; //e.g. void *handle (extra), double a, double b, double *result (extra)
          void *valuePointers[provArgsLength + 2];
          //note assuming doubles!!
          double values[provArgsLength];
          double result = 0.0;
          double *resultPointer = &result;
          int rvalue = 0;

          argTypes[0] = &ffi_type_pointer;
          argTypes[provArgsLength +1] = &ffi_type_pointer; //last argument is return value, handled as a pointer
          for (i = 0; i < provArgsLength; i += 1) {
              //FIXME for now assuming double as arguments
              argTypes[i+1] = &ffi_type_double;
          }

          valuePointers[0] = NULL;
          valuePointers[provArgsLength+1] = &resultPointer;
          for (i = 0; i < provArgsLength; i += 1) {
              values[i] = 0.0;
              valuePointers[i+1] = &(values[i]);
          }

          json_t *arguments = json_object_get(js_request, "a");
          json_t *value;
          size_t index;
          json_array_foreach(arguments, index, value) {
              values[index] = json_real_value(value); //setting values, again assuming double
          }

          json_decref(js_schema);
          json_decref(js_request);

          if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, provArgsLength + 2,
                      &ffi_type_sint, argTypes) == FFI_OK)
          {
              printf("RSA: FFI PREP OK\n");

              //TRYING TO CALL Calculate service
              void *service = NULL;
              bundleContext_getService(rsa->context, export->reference, &service);     
              if (service == NULL) {
                  printf("RSA: Ouch service is NULL\n");
                  return 0;
              } else {
                  printf("RSA: service ok\n");
              }

              struct generic_service_layout *serv = service;

              printf("RSA:Trying to call service using ffi\n");
              valuePointers[0] = serv->handle;
              ffi_call(&cif, serv->functions[methodIndex], &rvalue, valuePointers);
              printf("RSA: Done calling service through ffi, got value %f\n", result);

              json_t *resultRoot;
              resultRoot = json_pack("{s:f}", "r", result);
              response = json_dumps(resultRoot, 0);
              json_decref(resultRoot);  
          }

          if (response != NULL) {
              mg_write(conn, data_response_headers, strlen(data_response_headers));
//              mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
              mg_write(conn, response, strlen(response));
//              mg_send_data(conn, response, strlen(response));
//              mg_write_data(conn, response, strlen(response));

              free(response);
          } else {
              mg_write(conn, no_content_response_headers, strlen(no_content_response_headers));
          }
          result = 0;

          free(data);
      } else {
          //TODO log warning
      }

      celixThreadMutex_unlock(&rsa->exportedServicesLock);

		}
	}

	return 1;
}

celix_status_t remoteServiceAdmin_handleRequest(remote_service_admin_pt rsa, char *service, char *data, char **reply) {
	celixThreadMutex_lock(&rsa->exportedServicesLock);

	hash_map_iterator_pt iter = hashMapIterator_create(rsa->exportedServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		array_list_pt exports = hashMapEntry_getValue(entry);
		int expIt = 0;
		for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
			export_registration_pt export = arrayList_get(exports, expIt);
			if (strcmp(service, export->endpointDescription->service) == 0) {
				export->endpoint->handleRequest(export->endpoint->endpoint, data, reply);
			}
		}
	}
    hashMapIterator_destroy(iter);

	celixThreadMutex_unlock(&rsa->exportedServicesLock);

	return CELIX_SUCCESS;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_pt admin, char *serviceId, properties_pt properties, array_list_pt *registrations) {
    celix_status_t status = CELIX_SUCCESS;
    arrayList_create(registrations);
    array_list_pt references = NULL;
    service_reference_pt reference = NULL;
    char filter [256];

    snprintf(filter, 256, "(%s=%s)", (char *)OSGI_FRAMEWORK_SERVICE_ID, serviceId);

    bundleContext_getServiceReferences(admin->context, NULL, filter, &references); //FIXME not safe

    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "RSA: exportService called for serviceId %s", serviceId);

    if (arrayList_size(references) >= 1) {
        reference = arrayList_get(references, 0);
    }

    if(references!=NULL){
        arrayList_destroy(references);
    }

    if (reference == NULL) {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_ERROR, "ERROR: expected a reference for service id %s.", serviceId);
        return CELIX_ILLEGAL_STATE;
    }

    char *exports = NULL;
    char *provided = NULL;
    char *schema = NULL;
    serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exports);
    serviceReference_getProperty(reference, (char *) OSGI_FRAMEWORK_OBJECTCLASS, &provided);
    serviceReference_getProperty(reference, (char *) "protocol.schema", &schema);

    if (exports == NULL || provided == NULL) {
        logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: No Services to export.");
            status = CELIX_ILLEGAL_STATE;
    } else {
        if (schema == NULL) {
            logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: no protocol.schema found.");
            status = CELIX_ILLEGAL_STATE;
        } else {
            logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Export service (%s)", provided);

            char *interface = provided;
            endpoint_description_pt endpoint = NULL;
            export_registration_pt registration = NULL;

            remoteServiceAdmin_createEndpointDescription(admin, reference, interface, &endpoint);
            printf("RSA: Creating export registration with endpoint pointer %p\n", endpoint);
            exportRegistration_create(admin->loghelper, reference, endpoint, admin, admin->context, &registration);
            arrayList_add(*registrations, registration);

            exportRegistration_open(registration);
            exportRegistration_startTracking(registration);

            celixThreadMutex_lock(&admin->exportedServicesLock);
            hashMap_put(admin->exportedServices, reference, *registrations);
            celixThreadMutex_unlock(&admin->exportedServicesLock);
        }
    }
    return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(export_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;
    remote_service_admin_pt admin = registration->rsa;

    celixThreadMutex_lock(&admin->exportedServicesLock);

    hashMap_remove(admin->exportedServices, registration->reference);

    celixThreadMutex_unlock(&admin->exportedServicesLock);

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


static int remoteServiceAdmin_remoteFunctionProxy(ffi_cif *cif, int *ret, void* args[], void *userData); //TODO MOVE
static int remoteServiceAdmin_remoteFunctionProxy(ffi_cif *cif, int *ret, void* args[], void *userData) {
   void **handle = args[0];
   remote_service_admin_pt admin = (*handle);

   struct proxy *proxy = userData;

   printf("ffi closure got called for method sig %s with index %i\n", proxy->sig, proxy->index);

   json_t *root = json_object();
   json_t *sig = json_string(proxy->sig);
   json_object_set(root, "m", sig);

   json_t *arguments = json_array();
   int i;
   for (i = 1; i < proxy->cif.nargs -1; i += 1) {
       //NOTE assuming double
       double *dval = args[i];
       json_t *value = json_real(*dval);
       json_array_append(arguments, value);
   }
   json_object_set(root, "a", arguments);

   char *data = json_dumps(root, 0);
   char *reply = NULL;
   int replyStatus;

   remoteServiceAdmin_send(proxy->admin, proxy->endpointDescription, data, &reply, &replyStatus);
   (*ret) = replyStatus;
   printf("got reply from server '%s'\n", reply);

   json_error_t error;
   json_t *js_reply = json_loads(reply, JSON_DECODE_ANY, &error);
   if (js_reply) {
       //note assuming double
       double **result = args[proxy->cif.nargs - 1];
       json_unpack(js_reply, "{s:f}", "r", *result);
       json_decref(js_reply);
   } else {
       printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
   }

   json_decref(root);

   free(data);
   free(reply);

   return 0;
}


celix_status_t remoteServiceAdmin_importService(remote_service_admin_pt admin, endpoint_description_pt endpointDescription, import_registration_pt *registration) {
	celix_status_t status = CELIX_SUCCESS;

	logHelper_log(admin->loghelper, OSGI_LOGSERVICE_INFO, "RSA: Import service %s", endpointDescription->service);

  char *schema = properties_get(endpointDescription->properties, "protocol.schema");
  if (schema == NULL) {
    logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: protocol schema not found for endpoint\n");
    return CELIX_SUCCESS;
  }

  printf("protocol.schema is '%s'\n", schema);
  json_error_t error;
  json_t *js_schema = json_loads(schema, 0, &error);
  if (js_schema) {
      //TODO parse schema and create cif
      size_t numOfMethods = json_array_size(js_schema);
      printf("RSA: num of method for proxy is %i\n", (int) numOfMethods);

      struct generic_service_layout *service = calloc(1, sizeof(*service) + (numOfMethods-1) * sizeof(GEN_FUNC_TYPE)); 
      service->handle = admin;

        //struct generic_service_layout {
        //    void *handle;
        //    GEN_FUNC_TYPE functions[];
        //};

      size_t index;
      json_t *value;
      json_array_foreach(js_schema, index, value) {
          //create closure

          char *sig = json_string_value(value);
          size_t sigLength = strlen(sig);
          char provArgs[sigLength];
          bool startFound = false;
          int i = 0;
          int argIndex = 0;
          for (; i < sigLength; i += 1) {
              if (!startFound) {
                  if (sig[i] == '(') {
                      startFound = true;
                  }
              } else {
                  if (sig[i] != ')') {
                      provArgs[argIndex++] = sig[i];
                  }
              }
          } 
          provArgs[argIndex] = '\0';
          printf("method index is %i and args are '%s'\n", index, provArgs);

          int provArgLength = strlen(provArgs) -1;

          struct proxy *proxy = malloc(sizeof(*proxy));
          proxy->admin = admin;
          proxy->sig = strdup(sig);
          proxy->index = index;
          proxy->endpointDescription = endpointDescription;
          
          ffi_type **args = calloc(provArgLength +2, sizeof(ffi_type)); //TODO test if this can be on the stack

          void (*function)(void);
          proxy->closure = ffi_closure_alloc(sizeof(ffi_closure), &function);
          service->functions[index] = function;

          /* Initialize the argument info vectors */
          args[0] = &ffi_type_pointer;
          args[provArgLength + 1] = &ffi_type_pointer;
          for (i = 0; i < provArgLength; i += 1) {
              args[i+1] = &ffi_type_double;
          }

          if (proxy->closure) {
            if (ffi_prep_cif(&proxy->cif, FFI_DEFAULT_ABI, provArgLength + 2, &ffi_type_sint, args) == FFI_OK) {
                if (ffi_prep_closure_loc(proxy->closure, &proxy->cif, remoteServiceAdmin_remoteFunctionProxy, proxy, function) == FFI_OK) {
                    printf("RSA: created closure for method '%s'\n", sig);
                }
            }
          }
      }
                   
      //TODO register with imported properties 
      char *objectClass = properties_get(endpointDescription->properties, "objectClass");
      service_registration_pt reg = NULL;
      bundleContext_registerService(admin->context, objectClass, service, NULL, &reg);
      printf("registered proxy service with objectclass '%s'\n", objectClass);

  } else {
      printf("RSA: got error '%s' for '%s'\n", error.text, schema);
      return CELIX_ILLEGAL_STATE;
  }


  //celixThreadMutex_lock(&admin->importedServicesLock);
  //celixThreadMutex_unlock(&admin->importedServicesLock);


  return status;
  /*

	celixThreadMutex_lock(&admin->importedServicesLock);

   import_registration_factory_pt registration_factory = (import_registration_factory_pt) hashMap_get(admin->importedServices, endpointDescription->service);

	// check whether we already have a registration_factory registered in the hashmap
	if (registration_factory == NULL)
	{
		status = importRegistrationFactory_install(admin->loghelper, endpointDescription->service, admin->context, &registration_factory);
		if (status == CELIX_SUCCESS) {
		    hashMap_put(admin->importedServices, endpointDescription->service, registration_factory);
		}
	}

	 // factory available
	if (status != CELIX_SUCCESS || (registration_factory->trackedFactory == NULL))
	{
		logHelper_log(admin->loghelper, OSGI_LOGSERVICE_WARNING, "RSA: no proxyFactory available.");
		if (status == CELIX_SUCCESS) {
			status = CELIX_SERVICE_EXCEPTION;
		}
	}
	else
	{
		// we create an importRegistration per imported service
		importRegistration_create(endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send, admin->context, registration);
		registration_factory->trackedFactory->registerProxyService(registration_factory->trackedFactory->factory,  endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send);

		arrayList_add(registration_factory->registrations, *registration);
	}

    celixThreadMutex_unlock(&admin->importedServicesLock);


	return status;
  */
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


celix_status_t remoteServiceAdmin_send(remote_service_admin_pt rsa, endpoint_description_pt endpointDescription, char *request, char **reply, int* replyStatus) {

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
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        *reply = get.writeptr;
        *replyStatus = res;
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


celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	*endpoint = reference->endpoint;

	return status;
}

celix_status_t exportReference_getExportedService(export_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t importReference_getImportedService(import_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}
