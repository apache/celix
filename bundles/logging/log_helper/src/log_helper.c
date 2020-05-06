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


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <celix_api.h>

#include "bundle_context.h"
#include "service_tracker.h"
#include "celix_threads.h"
#include "array_list.h"

#include "celix_errno.h"
#include "log_service.h"

#include "log_helper.h"

#define LOGHELPER_ENABLE_STDOUT_FALLBACK_NAME 				"LOGHELPER_ENABLE_STDOUT_FALLBACK"
#define LOGHELPER_ENABLE_STDOUT_FALLBACK_DEFAULT 			true

#define LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_NAME 		"LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG"
#define LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_DEFAULT 	false


#ifdef __linux__
//includes for the backtrace function
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#ifdef __linux__
static char* logHelper_backtrace(void) {
	void *bbuf[64];
	int nrOfTraces = backtrace(bbuf, 64);
	char **lines = backtrace_symbols(bbuf, nrOfTraces);

	char *result = NULL;
	size_t size = 0;
	FILE *os = open_memstream(&result, &size);
	fprintf(os, "Backtrace:\n");
	for (int i = 0; i < nrOfTraces; ++i) {
		char *line = lines[i];
		fprintf(os, "%s\n", line);
	}
	free(lines);
	fclose(os);
	return result;
}
#else
static char* logHelper_backtrace(void) {
	return NULL;
}
#endif

struct log_helper {
	celix_bundle_context_t *bundleContext;
	char *name;
	long logServiceTrackerId;

    celix_thread_mutex_t logListLock;
	array_list_pt logServices;
	bool stdOutFallback;
	bool stdOutFallbackIncludeDebug;
};

static void logHelper_logServiceAdded(void *handle, void *service);
static void logHelper_logServiceRemoved(void *handle, void *service);

celix_status_t logHelper_start(log_helper_t *loghelper  __attribute__((unused)))
{
    //NOP, done in create
    return CELIX_SUCCESS;
}

celix_status_t logHelper_stop(log_helper_t *loghelper __attribute__((unused)))
{
    //NOP, done in stop
    return CELIX_SUCCESS;
}

log_helper_t* logHelper_createWithName(celix_bundle_context_t* context, const char *name) {
	log_helper_t* logHelper = calloc(1, sizeof(*logHelper));

    logHelper->bundleContext = context;
    logHelper->name = celix_utils_strdup(name);
    logHelper->logServiceTrackerId = -1L;
    logHelper->stdOutFallback = false;

    logHelper->stdOutFallback = celix_bundleContext_getPropertyAsBool(context, LOGHELPER_ENABLE_STDOUT_FALLBACK_NAME, LOGHELPER_ENABLE_STDOUT_FALLBACK_DEFAULT);
    logHelper->stdOutFallbackIncludeDebug = celix_bundleContext_getPropertyAsBool(context, LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_NAME, LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG_DEFAULT);

    pthread_mutex_init(&logHelper->logListLock, NULL);
    arrayList_create(&logHelper->logServices);

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = OSGI_LOGSERVICE_NAME;
    opts.callbackHandle = logHelper;
    opts.add = logHelper_logServiceAdded;
    opts.remove = logHelper_logServiceRemoved;
    logHelper->logServiceTrackerId = celix_bundleContext_trackServicesWithOptions(logHelper->bundleContext, &opts);

	return logHelper;
}

celix_status_t logHelper_create(bundle_context_pt context, log_helper_t **loghelper) {
	*loghelper = logHelper_createWithName(context, NULL);
	return *loghelper == NULL ? CELIX_ILLEGAL_STATE : CELIX_SUCCESS;
}

static void logHelper_logServiceAdded(void *handle, void *service)
{
	log_helper_t *loghelper = handle;

	pthread_mutex_lock(&loghelper->logListLock);
	arrayList_add(loghelper->logServices, service);
	pthread_mutex_unlock(&loghelper->logListLock);
}

static void logHelper_logServiceRemoved(void *handle, void *service)
{
	log_helper_t *loghelper = handle;

	pthread_mutex_lock(&loghelper->logListLock);
	arrayList_removeElement(loghelper->logServices, service);
	pthread_mutex_unlock(&loghelper->logListLock);
}

celix_status_t logHelper_destroy(log_helper_t **loghelper) {
        celix_status_t status = CELIX_SUCCESS;

        celix_bundleContext_stopTracker((*loghelper)->bundleContext, (*loghelper)->logServiceTrackerId);

        pthread_mutex_lock(&(*loghelper)->logListLock);
        arrayList_destroy((*loghelper)->logServices);
    	pthread_mutex_unlock(&(*loghelper)->logListLock);

        pthread_mutex_destroy(&(*loghelper)->logListLock);

        free((*loghelper)->name);

        free(*loghelper);
        *loghelper = NULL;
        return status;
}

celix_status_t logHelper_log(log_helper_t *loghelper, log_level_t level, const char* message, ... )
{
    celix_status_t status = CELIX_SUCCESS;
	va_list listPointer;
    char msg[1024];
    msg[0] = '\0';
    size_t prefixLen = 0;
    if (loghelper->name != NULL) {
        prefixLen = strlen(loghelper->name) + 3; //3 for '[] '
    }
    bool logged = false;

    if(loghelper == NULL){
	return CELIX_ILLEGAL_ARGUMENT;
    }

	va_start(listPointer, message);
    if (loghelper->name != NULL) {
        snprintf(msg, 1024, "[%s] ", loghelper->name);
    }
	vsnprintf(msg + prefixLen, 1024 - prefixLen, message, listPointer);

	pthread_mutex_lock(&loghelper->logListLock);

	int i = 0;
	for (; i < arrayList_size(loghelper->logServices); i++) {
		log_service_t *logService = arrayList_get(loghelper->logServices, i);
		if (logService != NULL) {
			(logService->log)(logService->logger, level, msg); //TODO add backtrace to msg if the level is ERROR
			if (level == OSGI_LOGSERVICE_ERROR) {
				char *backtrace = logHelper_backtrace();
				logService->log(logService->logger, level, backtrace);
				free(backtrace);
			}
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
			if (level == OSGI_LOGSERVICE_ERROR) {
				fprintf(stderr, "%s: %s\n", levelStr, msg);

                char *backtrace = logHelper_backtrace();
                if (backtrace != NULL) {
                    fprintf(stderr, "%s", backtrace);
                    free(backtrace);
				}
			} else {
				printf("%s: %s\n", levelStr, msg);
			}

		}
    }

    va_end(listPointer);

	return status;
}
