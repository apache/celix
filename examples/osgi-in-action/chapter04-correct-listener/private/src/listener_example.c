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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "log_service.h"

struct listenerActivator {
	apr_pool_t *pool;

	bundle_context_pt context;
	service_listener_pt listener;

	apr_thread_mutex_t *logServiceReferencesLock;
	array_list_pt logServiceReferences;

	bool running;
	apr_thread_t *logger;
};

void listenerExample_serviceChanged(service_listener_pt listener, service_event_pt event);
celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_pt *service);

static void *APR_THREAD_FUNC listenerExample_logger(apr_thread_t *thd, void *activator);

celix_status_t listenerExample_alternativeLog(struct listenerActivator *activator, char *message);

celix_status_t ref_equals(void *a, void *b, bool *equals) {
	return serviceReference_equals(a, b, equals);
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool;
	apr_pool_t *subpool;

	status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		if (apr_pool_create(&subpool, pool) == APR_SUCCESS) {
			*userData = apr_palloc(subpool, sizeof(struct listenerActivator));
			if (!userData) {
				status = CELIX_ENOMEM;
			} else {
				struct listenerActivator *activator = (*userData);
				activator->pool = subpool;
				activator->context = context;
				activator->listener = NULL;
				apr_thread_mutex_create(&activator->logServiceReferencesLock, 0, subpool);
				activator->logServiceReferences = NULL;
				arrayList_createWithEquals(ref_equals, &activator->logServiceReferences);
				activator->running = false;
				activator->logger = NULL;
			}
		} else {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct listenerActivator *activator = userData;

	service_listener_pt listener = apr_palloc(activator->pool, sizeof(struct listenerActivator));
	if (!listener) {
		status = CELIX_ENOMEM;
	} else {
		char filter[30];
		array_list_pt logServices = NULL;
		apr_pool_t *pool;
		sprintf(filter, "(objectClass=%s)", OSGI_LOGSERVICE_NAME);

		bundleContext_getMemoryPool(context, &pool);

		listener->handle = activator;
		listener->serviceChanged = (void *) listenerExample_serviceChanged;
		listener->pool = pool;
		status = bundleContext_addServiceListener(context, listener, filter);
		if (status == CELIX_SUCCESS) {
			activator->listener = listener;
		}

		status = bundleContext_getServiceReferences(context, NULL, filter, &logServices);
		if (status == CELIX_SUCCESS) {
			int i;
			for (i = 0; i < arrayList_size(logServices); i++) {
				service_reference_pt logService = (service_reference_pt) arrayList_get(logServices, i);
				service_event_pt event = apr_palloc(activator->pool, sizeof(*event));
				event->reference = logService;
				event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;

				listenerExample_serviceChanged(listener, event);
			}
		}

		activator->running = true;
		apr_thread_create(&activator->logger, NULL, listenerExample_logger, activator, activator->pool);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct listenerActivator *activator = userData;
	apr_status_t stat;

	activator->running = false;
	apr_thread_join(&stat, activator->logger);

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
	apr_thread_mutex_lock(activator->logServiceReferencesLock);

	switch (event->type) {
	case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
		arrayList_add(activator->logServiceReferences, event->reference);
		break;
//	case MODIFIED:
//		// only the service metadata has changed, so no need to do anything here
//		break;
	case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
		arrayList_remove(activator->logServiceReferences,
				arrayList_indexOf(activator->logServiceReferences, event->reference));
		break;
	default:
		break;
	}
	apr_thread_mutex_unlock(activator->logServiceReferencesLock);
}

celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;

	apr_thread_mutex_lock(activator->logServiceReferencesLock);
	if (arrayList_size(activator->logServiceReferences) > 0) {
		service_reference_pt reference = arrayList_get(activator->logServiceReferences, 0);
		status = bundleContext_getService(activator->context, reference, (void *) service);
	}
	apr_thread_mutex_unlock(activator->logServiceReferencesLock);

	return status;
}

static void *APR_THREAD_FUNC listenerExample_logger(apr_thread_t *thd, void *data) {
	struct listenerActivator *activator = data;

	while (activator->running) {
		log_service_pt logService = NULL;
		listenerExample_getLogService(activator, &logService);
		if (logService != NULL) {
			(*(logService->log))(logService->logger, OSGI_LOGSERVICE_INFO, "ping");
		} else {
			listenerExample_alternativeLog(activator, "No LogService available. Printing to standard out.");
		}
		apr_sleep(5000000);
	}

	return NULL;
}

celix_status_t listenerExample_alternativeLog(struct listenerActivator *activator, char *message) {
	celix_status_t status = CELIX_SUCCESS;

	printf("%s\n", message);

	return status;
}

