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

#include "celix_utils.h"
#include "celix_log_constants.h"
#include "celix_log_utils.h"
#include "celix_log_helper.h"
#include "celix_log_service.h"
#include "celix_threads.h"
#include "celix_err.h"

struct celix_log_helper {
    celix_bundle_context_t *ctx;
    long logServiceTrackerId;
    celix_log_level_e activeLogLevel;
    char *logServiceName;

    celix_thread_mutex_t mutex; //protects below
    celix_log_service_t* logService;
    size_t logCount;
};

static void celix_logHelper_setLogSvc(void *handle, void *svc) {
    celix_log_helper_t* logHelper = handle;
    celix_log_service_t* logSvc = svc;
    celixThreadMutex_lock(&logHelper->mutex);
    logHelper->logService = logSvc;
    celixThreadMutex_unlock(&logHelper->mutex);
}

celix_log_helper_t* celix_logHelper_create(celix_bundle_context_t* ctx, const char* logServiceName) {
    celix_log_helper_t* logHelper = calloc(1, sizeof(*logHelper));
    if (logHelper == NULL) {
        return NULL;
    }
    logHelper->ctx = ctx;
    logHelper->logServiceName = celix_utils_strdup(logServiceName);
    if (logHelper->logServiceName == NULL) {
        free(logHelper);
        return NULL;
    }
    celixThreadMutex_create(&logHelper->mutex, NULL);

    const char *actLogLevelStr = celix_bundleContext_getProperty(ctx, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_DEFAULT_VALUE);
    logHelper->activeLogLevel = celix_logUtils_logLevelFromString(actLogLevelStr, CELIX_LOG_LEVEL_INFO);


    char *filter = NULL;
    if (logServiceName != NULL) {
        if (asprintf(&filter, "(%s=%s)", CELIX_LOG_SERVICE_PROPERTY_NAME, logServiceName) < 0) {
            free(logHelper->logServiceName);
            free(logHelper);
            return NULL;
        }
    }

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.filter.versionRange = CELIX_LOG_SERVICE_USE_RANGE;
    opts.filter.filter = filter;
    opts.callbackHandle = logHelper;
    opts.set = celix_logHelper_setLogSvc;
    logHelper->logServiceTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

    free(filter);

    return logHelper;
}

void celix_logHelper_destroy(celix_log_helper_t *logHelper) {
    if (logHelper != NULL) {
        celix_bundleContext_stopTracker(logHelper->ctx, logHelper->logServiceTrackerId);
        celixThreadMutex_destroy(&logHelper->mutex);
        free(logHelper->logServiceName);
        free(logHelper);
    }
}

void celix_logHelper_trace(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_TRACE, format, args);
    va_end(args);
}

void celix_logHelper_debug(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void celix_logHelper_info(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void celix_logHelper_warning(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_WARNING, format, args);
    va_end(args);
}

void celix_logHelper_error(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void celix_logHelper_fatal(celix_log_helper_t* logHelper, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, CELIX_LOG_LEVEL_FATAL, format, args);
    va_end(args);
}

void celix_logHelper_log(celix_log_helper_t* logHelper, celix_log_level_e level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlog(logHelper, level, format, args);
    va_end(args);
}

void celix_logHelper_logDetails(celix_log_helper_t* logHelper, celix_log_level_e level, const char* file, const char* function, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logHelper_vlogDetails(logHelper, level, file, function, line, format, args);
    va_end(args);
}

void celix_logHelper_vlog(celix_log_helper_t* logHelper, celix_log_level_e level, const char *format, va_list formatArgs) {
    celix_logHelper_vlogDetails(logHelper, level, NULL, NULL, 0, format, formatArgs);
}

void celix_logHelper_vlogDetails(celix_log_helper_t* logHelper, celix_log_level_e level, const char* file, const char* function, int line, const char *format, va_list formatArgs) {
    if (level == CELIX_LOG_LEVEL_DISABLED) {
        //silently ignore
        return;
    }
    if (level >= logHelper->activeLogLevel) {
        celixThreadMutex_lock(&logHelper->mutex);
        celix_log_service_t* ls = logHelper->logService;
        if (ls != NULL) {
            ls->vlogDetails(ls->handle, level, file, function, line, format, formatArgs);
        } else {
            //falling back on stdout/stderr
            celix_logUtils_vLogToStdoutDetails(logHelper->logServiceName, level, file, function, line, format, formatArgs);
        }
        logHelper->logCount += 1;
        celixThreadMutex_unlock(&logHelper->mutex);
    }
}

void celix_logHelper_logTssErrors(celix_log_helper_t* logHelper, celix_log_level_e level) {
    char buf[CELIX_ERR_BUFFER_SIZE] = {0};
    if (celix_err_dump(buf, sizeof(buf), "[TssErr] ", NULL) == 0) {
        // nothing to output
        return;
    }
    celix_err_resetErrors();
    celix_logHelper_logDetails(logHelper, level, NULL, NULL, 0, "Detected tss errors:\n%s", buf);
}

size_t celix_logHelper_logCount(celix_log_helper_t* logHelper) {
    size_t count;
    celixThreadMutex_lock(&logHelper->mutex);
    count = logHelper->logCount;
    celixThreadMutex_unlock(&logHelper->mutex);
    return count;
}