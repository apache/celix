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

#include <stdarg.h>
#include <celix_log_utils.h>
#include <assert.h>
#include <stdlib.h>

#include "celix_errno.h"
#include "celix_log.h"
#include "celix_threads.h"
#include "celix_array_list.h"
#include "memstream/open_memstream.h"

#define LOG_NAME        "celix_framework"

struct celix_framework_logger {
    celix_thread_mutex_t mutex; //protect below
    celix_log_level_e activeLogLevel;
    char *buf;
    size_t bufSize;
    FILE* stream;
    void *logHandle;
    void (*logFunction)(void* handle, celix_log_level_e level, const char* file, const char *function, int line, const char *format, va_list formatArgs);
};

static pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;
celix_array_list_t* globalLoggers = NULL;

celix_framework_logger_t* celix_frameworkLogger_globalLogger() {
    celix_framework_logger_t* logger = NULL;
    pthread_mutex_lock(&globalMutex);
    if (globalLoggers != NULL) {
        logger = celix_arrayList_get(globalLoggers, 0); //Not NULL, always 1 entry
    }
    pthread_mutex_unlock(&globalMutex);
    return logger;
}

celix_framework_logger_t* celix_frameworkLogger_create(celix_log_level_e activeLogLevel) {
    celix_framework_logger_t* logger = calloc(1, sizeof(*logger));
    celixThreadMutex_create(&logger->mutex, NULL);
    logger->activeLogLevel = activeLogLevel;
    logger->stream = open_memstream(&logger->buf, &logger->bufSize);

    pthread_mutex_lock(&globalMutex);
    if (globalLoggers == NULL) {
        globalLoggers = celix_arrayList_create();
    }
    celix_arrayList_add(globalLoggers, logger);
    pthread_mutex_unlock(&globalMutex);

    return logger;
}

void celix_frameworkLogger_destroy(celix_framework_logger_t* logger) {
    if (logger != NULL) {
        pthread_mutex_lock(&globalMutex);
        assert(globalLoggers != NULL);
        for (int i = 0; i < celix_arrayList_size(globalLoggers); ++i) {
            celix_framework_logger_t *visit = celix_arrayList_get(globalLoggers, i);
            if (visit == logger) {
                celix_arrayList_removeAt(globalLoggers, i);
                break;
            }
        }
        if (celix_arrayList_size(globalLoggers) == 0) {
            celix_arrayList_destroy(globalLoggers);
            globalLoggers = NULL;
        }
        pthread_mutex_unlock(&globalMutex);

        celixThreadMutex_destroy(&logger->mutex);
        fclose(logger->stream);
        free(logger->buf);
        free(logger);
    }
}
void celix_frameworkLogger_setLogCallback(celix_framework_logger_t* logger, void* logHandle, void (*logFunction)(void* handle, celix_log_level_e level, const char* file, const char *func, int line, const char *format, va_list formatArgs)) {
    celixThreadMutex_lock(&logger->mutex);
    logger->logHandle = logHandle;
    logger->logFunction = logFunction;
    celixThreadMutex_unlock(&logger->mutex);
}


static void vlog(celix_framework_logger_t* logger, celix_log_level_e level, celix_status_t *optionalStatus, const char* file, const char* function, int line, const char* format, va_list args) {
    if (level == CELIX_LOG_LEVEL_DISABLED) {
        return;
    }
    celixThreadMutex_lock(&logger->mutex);
    if (logger->logFunction != NULL) {
        //note let log function handle active log levels
        logger->logFunction(logger->logHandle, level, file, function, line, format, args);
    } else if (level >= logger->activeLogLevel) {
        if (optionalStatus != NULL) {
            fseek(logger->stream, 0L, SEEK_SET);
            fprintf(logger->stream, "%s: ", celix_strerror(*optionalStatus));
            vfprintf(logger->stream, format, args);
            fputc('\0', logger->stream); //note not sure if this is needed
            fflush(logger->stream);
            celix_logUtils_logToStdoutDetails(LOG_NAME, level, file, function, line, logger->buf);
        } else {
            celix_logUtils_vLogToStdoutDetails(LOG_NAME, level, file, function, line, format, args);
        }
    }
    celixThreadMutex_unlock(&logger->mutex);
}


void framework_log(celix_framework_logger_t*  logger, celix_log_level_e level, const char *func, const char *file, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vlog(logger, level, NULL, file, func, line, format, args);
    va_end(args);
}

void framework_logCode(celix_framework_logger_t*  logger, celix_log_level_e level, const char *func, const char *file, int line, celix_status_t code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vlog(logger, level, &code, file, func, line, format, args);
    va_end(args);
}