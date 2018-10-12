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
 * log_helper.c
 *
 *  \date       Nov 10, 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdarg.h>

#include "bundle_context.h"
#include "service_tracker.h"
#include "celix_threads.h"
#include "array_list.h"

#include "celix_errno.h"
#include "log_service.h"

#include "log_helper.h"

#define LOGHELPER_ENABLE_STDOUT_FALLBACK_NAME 				"LOGHELPER_ENABLE_STDOUT_FALLBACK"
#define LOGHELPER_ENABLE_STDOUT_FALLBACK_DEFAULT 			false

#define LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_NAME 		"LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG"
#define LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_DEFAULT 	false


struct log_helper {
	bundle_context_pt bundleContext;
    service_tracker_pt logServiceTracker;
	celix_thread_mutex_t logListLock;
	array_list_pt logServices;
	bool stdOutFallback;
	bool stdOutFallbackIncludeDebug;
};

celix_status_t logHelper_logServiceAdded(void *handle, service_reference_pt reference, void *service);
celix_status_t logHelper_logServiceRemoved(void *handle, service_reference_pt reference, void *service);


celix_status_t logHelper_create(bundle_context_pt context, log_helper_pt* loghelper)
{
	celix_status_t status = CELIX_SUCCESS;

	(*loghelper) = calloc(1, sizeof(**loghelper));

	if (!(*loghelper))
	{
		status = CELIX_ENOMEM;
	}
	else
	{
		(*loghelper)->bundleContext = context;
		(*loghelper)->logServiceTracker = NULL;
		(*loghelper)->stdOutFallback = false;

		(*loghelper)->stdOutFallback = celix_bundleContext_getPropertyAsBool(context, LOGHELPER_ENABLE_STDOUT_FALLBACK_NAME, LOGHELPER_ENABLE_STDOUT_FALLBACK_DEFAULT);
		(*loghelper)->stdOutFallbackIncludeDebug = celix_bundleContext_getPropertyAsBool(context, LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_NAME, LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_DEFAULT);

		pthread_mutex_init(&(*loghelper)->logListLock, NULL);
        arrayList_create(&(*loghelper)->logServices);
	}

	return status;
}

celix_status_t logHelper_start(log_helper_pt loghelper)
{
	celix_status_t status;
	service_tracker_customizer_pt logTrackerCustomizer = NULL;

	status = serviceTrackerCustomizer_create(loghelper, NULL, logHelper_logServiceAdded, NULL, logHelper_logServiceRemoved, &logTrackerCustomizer);

	if (status == CELIX_SUCCESS) {
        loghelper->logServiceTracker = NULL;
		status = serviceTracker_create(loghelper->bundleContext, (char*) OSGI_LOGSERVICE_NAME, logTrackerCustomizer, &loghelper->logServiceTracker);
	}

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_open(loghelper->logServiceTracker);
	}

	return status;
}



celix_status_t logHelper_logServiceAdded(void *handle, service_reference_pt reference, void *service)
{
	log_helper_pt loghelper = handle;

	pthread_mutex_lock(&loghelper->logListLock);
	arrayList_add(loghelper->logServices, service);
	pthread_mutex_unlock(&loghelper->logListLock);

	return CELIX_SUCCESS;
}

celix_status_t logHelper_logServiceRemoved(void *handle, service_reference_pt reference, void *service)
{
	log_helper_pt loghelper = handle;

	pthread_mutex_lock(&loghelper->logListLock);
	arrayList_removeElement(loghelper->logServices, service);
	pthread_mutex_unlock(&loghelper->logListLock);

	return CELIX_SUCCESS;
}


celix_status_t logHelper_stop(log_helper_pt loghelper) {
	celix_status_t status;

    status = serviceTracker_close(loghelper->logServiceTracker);

    return status;
}

celix_status_t logHelper_destroy(log_helper_pt* loghelper) {
        celix_status_t status = CELIX_SUCCESS;

        if((*loghelper)->logServiceTracker){
      		serviceTracker_destroy((*loghelper)->logServiceTracker);
        }

        pthread_mutex_lock(&(*loghelper)->logListLock);
        arrayList_destroy((*loghelper)->logServices);
    	pthread_mutex_unlock(&(*loghelper)->logListLock);

        pthread_mutex_destroy(&(*loghelper)->logListLock);

        free(*loghelper);
        *loghelper = NULL;
        return status;
}




celix_status_t logHelper_log(log_helper_pt loghelper, log_level_t level, char* message, ... )
{
    celix_status_t status = CELIX_SUCCESS;
	va_list listPointer;
    char msg[1024];
    msg[0] = '\0';
    bool logged = false;

    if(loghelper == NULL){
	return CELIX_ILLEGAL_ARGUMENT;
    }

	va_start(listPointer, message);
	vsnprintf(msg, 1024, message, listPointer);

	pthread_mutex_lock(&loghelper->logListLock);

	int i = 0;
	for (; i < arrayList_size(loghelper->logServices); i++) {
		log_service_pt logService = arrayList_get(loghelper->logServices, i);
		if (logService != NULL) {
			(logService->log)(logService->logger, level, msg);
			logged = true;
		}
	}

	pthread_mutex_unlock(&loghelper->logListLock);

    if (!logged && loghelper->stdOutFallback) {
        char *levelStr = NULL;
        bool print = true;

        switch (level) {
            case OSGI_LOGSERVICE_ERROR:
                levelStr = "ERROR";
                break;
            case OSGI_LOGSERVICE_WARNING:
                levelStr = "WARNING";
                break;
            case OSGI_LOGSERVICE_INFO:
                levelStr = "INFO";
                break;
            case OSGI_LOGSERVICE_DEBUG:
            default:
                levelStr = "DEBUG";
                print = loghelper->stdOutFallbackIncludeDebug;
                break;
        }

        if (print) {
			printf("%s: %s\n", levelStr, msg);
		}
    }

    va_end(listPointer);

	return status;
}
