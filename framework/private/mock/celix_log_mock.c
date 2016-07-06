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
 * celix_log_mock.c
 *
 *  \date       Oct, 5 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include <stdarg.h>

#include "celix_errno.h"
#include "celix_log.h"

static void test_logger_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, const char *fmsg, ...);
static void test_logger_print(framework_log_level_t level, const char *func, const char *file, int line, const char *msg);

void framework_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, const char *fmsg, ...) {
	mock_c()->actualCall("framework_log");

    test_logger_log(logger, level, func, file, line, "%s", fmsg);
}

void framework_logCode(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, celix_status_t code, const char *fmsg, ...) {
	mock_c()->actualCall("framework_logCode")->withIntParameters("code", code);
    char message[256];
    celix_strerror(code, message, 256);
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);

    test_logger_log(logger, level, func, file, line, "%s [%d]: %s", message, code, msg);

    va_end(listPointer);
}

celix_status_t frameworkLogger_log(framework_log_level_t level, const char *func, const char *file, int line, const char *msg) {
	mock_c()->actualCall("frameworkLogger_log");

	test_logger_print(level, func, file, line, msg);

	return mock_c()->returnValue().value.intValue;
}

//test logger functions, let you read the logged errors
static void test_logger_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, const char *fmsg, ...) {
    char msg[512];
    va_list listPointer;
    va_start(listPointer, fmsg);
    vsprintf(msg, fmsg, listPointer);

    test_logger_print(level, func, file, line, msg);

    va_end(listPointer);
}

static void test_logger_print(framework_log_level_t level, const char *func, const char *file, int line, const char *msg) {
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
        printf("Code says: %s: %s\n\tat %s(%s:%d)\n", levelStr, msg, func, file, line);
    } else {
        printf("Code says: %s: %s\n", levelStr, msg);
    }
}
