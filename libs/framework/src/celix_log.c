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

#include "celix_errno.h"
#include "celix_log.h"

#ifdef __linux__
//includes for the backtrace function
#include <execinfo.h>
#include <stdlib.h>
#endif

#ifdef __linux__
static void framework_logBacktrace(void) {
    void *bbuf[64];
    int nrOfTraces = backtrace(bbuf, 64);
    char **lines = backtrace_symbols(bbuf, nrOfTraces);
    for (int i = 0; i < nrOfTraces; ++i) {
        char *line = lines[i];
        fprintf(stderr, "%s\n", line);
    }
    free(lines);
}
#else
static void framework_logBacktrace(void) {}
#endif

void framework_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, const char *fmsg, ...) {
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);

    //FIXME logger and/or logger->logFunction can be null. But this solution is not thread safe!
    if (logger != NULL && logger->logFunction != NULL) {
        logger->logFunction(level, func, file, line, msg);
    }

    va_end(listPointer);
}

void framework_logCode(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, celix_status_t code, const char *fmsg, ...) {
    char message[256];
    celix_strerror(code, message, 256);
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);

    framework_log(logger, level, func, file, line, "%s [%d]: %s", message, code, msg);

    va_end(listPointer);
}

celix_status_t frameworkLogger_log(framework_log_level_t level, const char *func, const char *file, int line, const char *msg) {
    char *levelStr = NULL;
    switch (level) {
        case OSGI_FRAMEWORK_LOG_ERROR:
            levelStr = "ERROR";
            break;
        case OSGI_FRAMEWORK_LOG_WARNING:
            levelStr = "WARNING";
            break;
        case OSGI_FRAMEWORK_LOG_INFO:
            levelStr = "INFO";
            break;
        case OSGI_FRAMEWORK_LOG_DEBUG:
        default:
            levelStr = "DEBUG";
            break;
    }

    if (level == OSGI_FRAMEWORK_LOG_ERROR) {
        fprintf(stderr, "%s: %s\n\tat %s(%s:%d)\n", levelStr, msg, func, file, line);
        framework_logBacktrace();
    } else {
        printf("%s: %s\n", levelStr, msg);
    }

    return CELIX_SUCCESS;
}
