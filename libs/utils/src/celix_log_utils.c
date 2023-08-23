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

#include "celix_compiler.h"
#include "celix_log_utils.h"

#include <stdbool.h>
#include <stdarg.h>
#ifndef __UCLIBC__
#include <execinfo.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

const char* celix_logUtils_logLevelToString(celix_log_level_e level) { return celix_logLevel_toString(level); }

celix_log_level_e celix_logUtils_logLevelFromString(const char* str, celix_log_level_e fallbackLogLevel) {
    return celix_logLevel_fromString(str, fallbackLogLevel);
}

celix_log_level_e celix_logUtils_logLevelFromStringWithCheck(const char* str,
                                                             celix_log_level_e fallbackLogLevel,
                                                             bool* convertedSuccessfully) {
    return celix_logLevel_fromStringWithCheck(str, fallbackLogLevel, convertedSuccessfully);
}

#ifndef __UCLIBC__
static inline void celix_logUtils_inlinePrintBacktrace(FILE *stream) {
    void *bbuf[64];
    int nrOfTraces = backtrace(bbuf, 64);
    char **lines = backtrace_symbols(bbuf, nrOfTraces);
    for (int i = 0; i < nrOfTraces; ++i) {
        char *line = lines[i];
        fprintf(stream, "%s\n", line);
    }
    free(lines);
    fflush(stream);
}
#else /* __UCLIBC__ */
static inline void celix_logUtils_inlinePrintBacktrace(FILE *stream CELIX_UNUSED) {
    //nop
}
#endif /* __UCLIBC__ */

void celix_logUtils_printBacktrace(FILE* stream) {
    celix_logUtils_inlinePrintBacktrace(stream);
}

void celix_logUtils_logToStdout(const char *logName, celix_log_level_e level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logUtils_vLogToStdout(logName, level, format, args);
    va_end(args);
}

void celix_logUtils_logToStdoutDetails(const char *logName, celix_log_level_e level, const char* file, const char* function, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logUtils_vLogToStdoutDetails(logName, level, file, function, line, format, args);
    va_end(args);
}

void celix_logUtils_vLogToStdout(const char *logName, celix_log_level_e level, const char *format, va_list formatArgs) {
    celix_logUtils_vLogToStdoutDetails(logName, level, NULL, NULL, 0, format, formatArgs);
}

static pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;

void celix_logUtils_vLogToStdoutDetails(const char *logName, celix_log_level_e level, const char* file, const char* function, int line, const char *format, va_list formatArgs) {
    if (level == CELIX_LOG_LEVEL_DISABLED) {
        //silently ignore
        return;
    }

    //get printable time
    time_t t = time(NULL);
    struct tm local;
    localtime_r(&t, &local);

    FILE *out = stdout;
    if (level == CELIX_LOG_LEVEL_WARNING || level == CELIX_LOG_LEVEL_ERROR || level == CELIX_LOG_LEVEL_FATAL) {
        out = stderr;
    }

    pthread_mutex_lock(&globalMutex);
    fprintf(out, "[%i-%02i-%02iT%02i:%02i:%02i] ", local.tm_year + 1900, local.tm_mon+1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    if (file != NULL && function != NULL) {
        (void)file; //note not using file
        fprintf(out, "[%7s] [%s] [%s:%i] ", celix_logUtils_logLevelToString(level), logName, function, line);
    } else {
        fprintf(out, "[%7s] [%s] ", celix_logUtils_logLevelToString(level), logName);
    }
    vfprintf(out, format, formatArgs);
    fprintf(out, "\n");

    if (level >= CELIX_LOG_LEVEL_FATAL) {
        fprintf(out, "Backtrace:\n");
        celix_logUtils_inlinePrintBacktrace(out);
    }
    fflush(out);
    pthread_mutex_unlock(&globalMutex);
}