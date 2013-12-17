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
 * celix_log.c
 *
 *  \date       6 Oct 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdarg.h>

#include "celix_errno.h"
#include "celix_log.h"

void framework_log(framework_log_level_t level, const char *func, const char *file, int line, char *fmsg, ...) {
    char *levelStr = NULL;
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);
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
        printf("%s: %s\n\tat %s(%s:%d)\n", levelStr, msg, func, file, line);
    } else {
        printf("%s: %s\n", levelStr, msg);
    }

}

void framework_logCode(framework_log_level_t level, const char *func, const char *file, int line, celix_status_t code, char *fmsg, ...) {
    char message[256];
    celix_strerror(code, message, 256);
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);

    framework_log(level, func, file, line, "%s [%d]: %s", message, code, msg);
}

