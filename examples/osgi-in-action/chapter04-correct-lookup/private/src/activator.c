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
#include "pthread.h"
#include "log_service.h"
#include "bundle.h"

typedef log_service_t LOG_SERVICE;

struct threadData {
	char * service;
	int threadId;
	BUNDLE_CONTEXT m_context;
};

typedef struct threadData *THREAD_DATA;

static pthread_t m_logTestThread;


//*******************************************************************************
// function prototypes
//*******************************************************************************
void startTestThread(THREAD_DATA data);
void stopTestThread();
void pauseTestThread();
void alternativeLog(char *message, THREAD_DATA data);
//*******************************************************************************
// global functions
//*******************************************************************************

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	apr_pool_t *pool;
	celix_status_t status = bundleContext_getMemoryPool(context, &pool);
	if (status == CELIX_SUCCESS) {
		*userData = apr_palloc(pool, sizeof(struct threadData));
		((THREAD_DATA)(*userData))->service = "chapter04-correct-lookup";
		((THREAD_DATA)(*userData))->threadId = 0;
		((THREAD_DATA)(*userData))->m_context = context;
	} else {
		status = CELIX_START_ERROR;
	}
	return status;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	((THREAD_DATA) userData)->m_context = context;
	startTestThread(((THREAD_DATA) userData));
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	stopTestThread();
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

//------------------------------------------------------------------------------------------
//  The rest of this is just support code, not meant to show any particular best practices
//------------------------------------------------------------------------------------------

// Test LogService by periodically sending a message
void *LogServiceTest (void *argument){
	celix_status_t status = CELIX_SUCCESS;
	THREAD_DATA data = (THREAD_DATA) argument;
	BUNDLE_CONTEXT m_context = ((THREAD_DATA) argument)->m_context;
	while (pthread_self() == m_logTestThread) {
		SERVICE_REFERENCE logServiceRef = NULL;
		// lookup the current "best" LogService each time, just before we need to use it
		status = bundleContext_getServiceReference(m_context, (char *) LOG_SERVICE_NAME, &logServiceRef);
		// if the service reference is null then we know there's no log service available
		if (status == CELIX_SUCCESS && logServiceRef != NULL) {
			void *log = NULL;
			LOG_SERVICE logService = NULL;
			bundleContext_getService(m_context, logServiceRef, &log);
			logService = (LOG_SERVICE) log;
			// if the dereferenced instance is null then we know the service has been removed
			if (logService != NULL) {
				(*(logService->log))(logService->logger, LOG_INFO, "ping");
			} else {
				alternativeLog("LogService has gone", data);
			}
		} else {
			alternativeLog("LogService has gone", data);
		}
		pauseTestThread();
	}
	return NULL;
}

void startTestThread(THREAD_DATA data) {
	// start separate worker thread to run the actual tests, managed by the bundle lifecycle
	data->threadId++;
	int  rc = pthread_create(&m_logTestThread, NULL, LogServiceTest, data);
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

void alternativeLog(char *message, THREAD_DATA data) {
	// this provides similar style debug logging output for when the LogService disappears
	celix_status_t status = CELIX_SUCCESS;
	BUNDLE bundle = NULL;
	char tid[20], bid[20];
	long bundleId;
	if (data->m_context != NULL) {
		status = bundleContext_getBundle(data->m_context, &bundle);
		if (status == CELIX_SUCCESS) {
			status = bundle_getBundleId(bundle, &bundleId);
			if (status == CELIX_SUCCESS) {
				sprintf(tid, "thread=%d", data->threadId);
				sprintf(bid, "bundle=%ld", bundleId);
				printf("<--> %s, %s : %s\n", tid, bid, message);
			} else {
				printf("%s:%s:%d:getBundleId failed:  %s\n", __FILE__, __FUNCTION__, __LINE__, message);
			}
		} else {
			printf("%s:%s:%d:getBundle failed: %s\n", __FILE__, __FUNCTION__, __LINE__, message);
		}
	} else {
		printf("%s:%s:%d:bundle context NULL:  %s\n", __FILE__, __FUNCTION__, __LINE__, message);
	}
}
