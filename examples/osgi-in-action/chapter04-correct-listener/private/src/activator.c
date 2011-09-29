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

#include <stdlib.h>
#include <stdio.h>
#include <apr_general.h>
#include <unistd.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "bundle.h"
#include "log_service.h"
#include "array_list.h"

typedef log_service_t LOG_SERVICE;

struct clientActivator {
	BUNDLE_CONTEXT m_context;
	SERVICE_LISTENER m_Loglistener;
	apr_pool_t *pool;
	char * service;
};
typedef struct clientActivator * CLIENT_ACTIVATOR;

ARRAY_LIST m_logServiceRefs = NULL;
static pthread_t m_logTestThread;
pthread_mutex_t logListenerMutex;

//*******************************************************************************
// function prototypes
//*******************************************************************************
void serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event);
void startTestThread(void *userData);
void stopTestThread();
void pauseTestThread();
void alternativeLog(char *message, BUNDLE_CONTEXT m_context);
void logListener(CLIENT_ACTIVATOR activator);
//*******************************************************************************
// global functions
//*******************************************************************************

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	CLIENT_ACTIVATOR activator;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct clientActivator));
		activator = (CLIENT_ACTIVATOR) (*userData);
		activator->service = "chapter04-correct-listener";
		activator->m_Loglistener = NULL;
		activator->pool = pool;
		activator->m_context = context;
		m_logServiceRefs = arrayList_create();
		/* init the logServiceListener mutex */
		pthread_mutex_init(&logListenerMutex, NULL);
	} else {
		status = CELIX_START_ERROR;
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	int i;
	celix_status_t status = CELIX_SUCCESS;
	CLIENT_ACTIVATOR clientActivator = (CLIENT_ACTIVATOR) userData;
	ARRAY_LIST refs = NULL;
	SERVICE_EVENT event = NULL;
	struct clientActivator * activator = (struct clientActivator *) userData;
	logListener(activator);
	// after adding the listener check for any existing services that need pseudo events
	// to register the existing services in the m_logServiceRefs list
	status = bundleContext_getServiceReferences(context, NULL, NULL, &refs);
	if (status == CELIX_SUCCESS){
		if (refs != NULL) {
			for (i = 0; (i < arrayList_size(refs)); i++){
				event = apr_palloc(clientActivator->pool, sizeof(struct serviceEvent));
				event->reference = arrayList_get(m_logServiceRefs, i);
				event->type = REGISTERED;
				serviceChanged(clientActivator->m_Loglistener, event);
			}
		} else {
			alternativeLog("no bundle references\n", context);
		}
		startTestThread(userData);
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct clientActivator * act = (struct clientActivator *) userData;
	stopTestThread();
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	struct clientActivator * act = (struct clientActivator *) userData;
	arrayList_destroy(m_logServiceRefs);
	return CELIX_SUCCESS;
}

void serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	pthread_mutex_lock(&logListenerMutex);
	switch (event->type) {
	case REGISTERED:
		printf("reg\n");
		arrayList_add(m_logServiceRefs, event->reference);
		break;
	case MODIFIED:
		printf("mod\n");
		// only the service metadata has changed, so no need to do anything here
		break;
	case UNREGISTERING:
		printf("unr\n");
		arrayList_remove(m_logServiceRefs,
				arrayList_indexOf(m_logServiceRefs, event->reference));
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&logListenerMutex);
}

// we must lock the listener before querying the internal state
LOG_SERVICE getLogService(BUNDLE_CONTEXT context) {
	LOG_SERVICE logService = NULL;
	celix_status_t status = CELIX_SUCCESS;
	pthread_mutex_lock(&logListenerMutex);
	if (arrayList_size(m_logServiceRefs) > 0) {
		// the last service reference should have the highest ranking
		status = bundleContext_getService(context,
				arrayList_get(m_logServiceRefs, 0), (void **) &logService);
		if (status != CELIX_SUCCESS){
			printf("could not find service\n");
		}
	}
	pthread_mutex_unlock(&logListenerMutex);
	return logService;
}

// Test LogService by periodically sending a message
void *logServiceTest (void *argument){
	int tid;
	LOG_SERVICE logService = NULL;
	CLIENT_ACTIVATOR data = (CLIENT_ACTIVATOR) argument;
	while (pthread_self() == m_logTestThread) {
		SERVICE_REFERENCE logServiceRef = NULL;
		// lookup the current "best" LogService each time, just before we need to use it
		//bundleContext_getServiceReference(m_context, (char *) LOG_SERVICE_NAME, &logServiceRef);
		logService = getLogService(data->m_context);
		// if the dereferenced instance is null then we know the service has been removed
		if (logService != NULL) {
			(*(logService->log))(logService->logger, LOG_INFO, "ping");
		} else {
			alternativeLog("LogService has gone", data->m_context);
		}
		pauseTestThread();
	}
	return NULL;
}



//------------------------------------------------------------------------------------------
//  The rest of this is just support code, not meant to show any particular best practices
//------------------------------------------------------------------------------------------


void startTestThread(void *userData) {
	// start separate worker thread to run the actual tests, managed by the bundle lifecycle
	pthread_create(&m_logTestThread, NULL,logServiceTest, userData);
}

void stopTestThread() {
	// thread should cooperatively shutdown on the next iteration, because field is now null
	pthread_t testThread = m_logTestThread;
	pthread_cancel(testThread);
	pthread_join(testThread, NULL);
	m_logTestThread = 0;
}

void pauseTestThread() {
	// sleep for a bit
	sleep(5);
}

void alternativeLog(char *message, BUNDLE_CONTEXT m_context) {
	// this provides similar style debug logging output for when the LogService disappears
	BUNDLE bundle = NULL;
	celix_status_t status = CELIX_SUCCESS;
	char tid[20], bid[20];
	long bundleId;
	if (m_context != NULL) {
		celix_status_t rc = bundleContext_getBundle(m_context, &bundle);
		if (status == CELIX_SUCCESS) {
			celix_status_t rc = bundle_getBundleId(bundle, &bundleId);
			if (status == CELIX_SUCCESS) {
				sprintf(tid, "thread=%s", "Thread");
				sprintf(bid, "bundle=%ld", bundleId);
				printf("<--> %s, %s : %s\n", tid, bid, message);
			} else {
				printf("%s:%s:%d:getBundleId failed:  %s\n", __FILE__, __FUNCTION__, __LINE__, message);
			}
		} else {
			printf("%s:%s:%d:getBundle failed: %s\n", __FILE__, __FUNCTION__, __LINE__, message);
		}
	}	else {
		printf("%s:%d:bundle context NULL:  %s\n", __FILE__, __LINE__, message);
	}
}

void logListener(CLIENT_ACTIVATOR activator)
{
	celix_status_t status;
	char filter[30];
	int i;
	SERVICE_REFERENCE r = NULL;
	SERVICE_LISTENER m_Loglistener = NULL;
	SERVICE_EVENT e = NULL;
	ARRAY_LIST refs = NULL;
	/* listener code */
	m_Loglistener = (SERVICE_LISTENER) apr_palloc(activator->pool, sizeof(*m_Loglistener));
	activator->m_Loglistener = m_Loglistener;
	m_Loglistener->handle = NULL;
	m_Loglistener->serviceChanged = (void *) serviceChanged;
	sprintf(filter, "(objectClass=%s)", LOG_SERVICE_NAME);
	status = bundleContext_addServiceListener(activator->m_context, m_Loglistener, filter);
	if (status == CELIX_SUCCESS) {
		// after adding the listener check for any existing services that need pseudo events
		bundleContext_getServiceReferences(activator->m_context, NULL, filter, &refs);
		if (refs != NULL) {
			for (i = 0; i < arrayList_size(refs); i++) {
				r = (SERVICE_REFERENCE) arrayList_get(refs, i);
				e = apr_palloc(activator->pool, sizeof(struct clientActivator));
				e->type = REGISTERED;
				e->reference = r;
				(*m_Loglistener->serviceChanged)(m_Loglistener, e);
			}
		}
	} else {
		printf("could not add service listener\n");
	}
}
