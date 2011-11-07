/*
 * listener_example.c
 *
 *  Created on: Sep 22, 2011
 *      Author: alexander
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <apr_general.h>

#include "bundle_activator.h"
#include "log_service.h"

struct listenerActivator {
	apr_pool_t *pool;

	BUNDLE_CONTEXT context;
	SERVICE_LISTENER listener;

	apr_thread_mutex_t *logServiceReferencesLock;
	ARRAY_LIST logServiceReferences;

	bool running;
	apr_thread_t *logger;
};

void listenerExample_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event);
celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_t *service);

static void *APR_THREAD_FUNC listenerExample_logger(apr_thread_t *thd, void *activator);

celix_status_t listenerExample_alternativeLog(struct listenerActivator *activator, char *message);

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
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
				arrayList_create(subpool, &activator->logServiceReferences);
				activator->running = false;
				activator->logger = NULL;
			}
		} else {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct listenerActivator *activator = userData;

	SERVICE_LISTENER listener = apr_palloc(activator->pool, sizeof(struct listenerActivator));
	if (!listener) {
		status = CELIX_ENOMEM;
	} else {
		char filter[30];
		sprintf(filter, "(objectClass=%s)", LOG_SERVICE_NAME);

		listener->handle = activator;
		listener->serviceChanged = (void *) listenerExample_serviceChanged;
		status = bundleContext_addServiceListener(context, listener, filter);
		if (status == CELIX_SUCCESS) {
			activator->listener = listener;
		}

		ARRAY_LIST logServices = NULL;
		status = bundleContext_getServiceReferences(context, NULL, filter, &logServices);
		if (status == CELIX_SUCCESS) {
			int i;
			for (i = 0; i < arrayList_size(logServices); i++) {
				SERVICE_REFERENCE logService = (SERVICE_REFERENCE) arrayList_get(logServices, i);
				SERVICE_EVENT event = apr_palloc(activator->pool, sizeof(*event));
				event->reference = logService;
				event->type = REGISTERED;

				listenerExample_serviceChanged(listener, event);
			}
		}

		activator->running = true;
		apr_thread_create(&activator->logger, NULL, listenerExample_logger, activator, activator->pool);
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct listenerActivator *activator = userData;
	apr_status_t stat;

	activator->running = false;
	apr_thread_join(&stat, activator->logger);

	bundleContext_removeServiceListener(context, activator->listener);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;
	struct listenerActivator *activator = userData;
	arrayList_destroy(activator->logServiceReferences);
	return status;
}

void listenerExample_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	struct listenerActivator *activator = listener->handle;
	apr_thread_mutex_lock(activator->logServiceReferencesLock);

	switch (event->type) {
	case REGISTERED:
		arrayList_add(activator->logServiceReferences, event->reference);
		break;
//	case MODIFIED:
//		// only the service metadata has changed, so no need to do anything here
//		break;
	case UNREGISTERING:
		arrayList_remove(activator->logServiceReferences,
				arrayList_indexOf(activator->logServiceReferences, event->reference));
		break;
	default:
		break;
	}
	apr_thread_mutex_unlock(activator->logServiceReferencesLock);
}

celix_status_t listenerExample_getLogService(struct listenerActivator *activator, log_service_t *service) {
	celix_status_t status = CELIX_SUCCESS;

	apr_thread_mutex_lock(activator->logServiceReferencesLock);
	if (arrayList_size(activator->logServiceReferences) > 0) {
		SERVICE_REFERENCE reference = arrayList_get(activator->logServiceReferences, 0);
		status = bundleContext_getService(activator->context, reference, (void *) service);
	}
	apr_thread_mutex_unlock(activator->logServiceReferencesLock);

	return status;
}

static void *APR_THREAD_FUNC listenerExample_logger(apr_thread_t *thd, void *data) {
	struct listenerActivator *activator = data;

	while (activator->running) {
		log_service_t logService = NULL;
		listenerExample_getLogService(activator, &logService);
		if (logService != NULL) {
			(*(logService->log))(logService->logger, LOG_INFO, "ping");
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

