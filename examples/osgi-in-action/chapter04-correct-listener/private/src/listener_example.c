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
 * listener_example.c
 *
 *  \date       Sep 22, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "bundle_activator.h"
#include "log_service.h"

struct listenerActivator {
    bundle_context_pt context;
    service_listener_pt listener;

    celix_thread_t logger;
    celix_thread_mutex_t logServiceReferencesLock;

    array_list_pt logServiceReferences;

    bool running;
};

void listenerExample_serviceChanged(service_listener_pt listener, service_event_pt event);
celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_pt *service);

static void* listenerExample_logger(void* data);

celix_status_t listenerExample_alternativeLog(struct listenerActivator *activator, char *message);

celix_status_t ref_equals(void *a, void *b, bool *equals) {
    return serviceReference_equals(a, b, equals);
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;

    *userData = calloc(1, sizeof(struct listenerActivator));
    if (!*userData) {
        status = CELIX_ENOMEM;
    } else {
        struct listenerActivator *activator = (*userData);
        activator->context = context;
        activator->listener = NULL;
        activator->logServiceReferences = NULL;
        arrayList_createWithEquals(ref_equals, &activator->logServiceReferences);
        activator->running = false;

        status = celixThreadMutex_create(&activator->logServiceReferencesLock, NULL);
    }

    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct listenerActivator *activator = userData;

    service_listener_pt listener = calloc(1, sizeof(*listener));
    if (!listener) {
        status = CELIX_ENOMEM;
    } else {
        char filter[30];
        array_list_pt logServices = NULL;
        sprintf(filter, "(objectClass=%s)", OSGI_LOGSERVICE_NAME);

        listener->handle = activator;
        listener->serviceChanged = (void *) listenerExample_serviceChanged;
        status = bundleContext_addServiceListener(context, listener, filter);
        if (status == CELIX_SUCCESS) {
            activator->listener = listener;
        }

        status = bundleContext_getServiceReferences(context, NULL, filter, &logServices);
        if (status == CELIX_SUCCESS) {
            int i;
            for (i = 0; i < arrayList_size(logServices); i++) {
                service_reference_pt logService = (service_reference_pt) arrayList_get(logServices, i);
                service_event_pt event = calloc(1, sizeof(*event));
                event->reference = logService;
                event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;

                listenerExample_serviceChanged(listener, event);
                free(event);
            }
            arrayList_destroy(logServices);
        }

        activator->running = true;

        status = celixThread_create(&activator->logger, NULL, listenerExample_logger, activator);
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct listenerActivator *activator = userData;

    activator->running = false;
    celixThread_join(activator->logger, NULL);

    bundleContext_removeServiceListener(context, activator->listener);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct listenerActivator *activator = userData;
    arrayList_destroy(activator->logServiceReferences);
    return status;
}

void listenerExample_serviceChanged(service_listener_pt listener, service_event_pt event) {
    struct listenerActivator *activator = listener->handle;
    celixThreadMutex_lock(&activator->logServiceReferencesLock);

    switch (event->type) {
    case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
        arrayList_add(activator->logServiceReferences, event->reference);
        break;
//	case MODIFIED:
//		// only the service metadata has changed, so no need to do anything here
//		break;
    case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
        arrayList_removeElement(activator->logServiceReferences, event->reference);
        break;
    default:
        break;
    }

    celixThreadMutex_unlock(&activator->logServiceReferencesLock);
}

celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_pt *service) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&activator->logServiceReferencesLock);

    if (arrayList_size(activator->logServiceReferences) > 0) {
        service_reference_pt reference = arrayList_get(activator->logServiceReferences, 0);
        status = bundleContext_getService(activator->context, reference, (void *) service);
    }
    celixThreadMutex_unlock(&activator->logServiceReferencesLock);

    return status;
}

static void* listenerExample_logger(void* data) {
    struct listenerActivator *activator = data;

    while (activator->running) {
        log_service_pt logService = NULL;
        listenerExample_getLogService(activator, &logService);
        if (logService != NULL) {
            (*(logService->log))(logService->logger, OSGI_LOGSERVICE_INFO, "ping");
        } else {
            listenerExample_alternativeLog(activator, "No LogService available. Printing to standard out.");
        }

        sleep(5);
    }

    return NULL;
}

celix_status_t listenerExample_alternativeLog(struct listenerActivator *activator, char *message) {
    celix_status_t status = CELIX_SUCCESS;

    printf("%s\n", message);

    return status;
}

