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

#ifndef CELIX_LOG_UTILS_H
#define CELIX_LOG_UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "celix_log_level.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts the celix log level enum to a const char* value.
 */
const char* celix_logUtils_logLevelToString(celix_log_level_e level);

/**
 * Converts a const char* value to a celix log level
 * If the provided log level cannot be parsed or is NULL, the fallbackLogLevel is returned.
 */
celix_log_level_e celix_logUtils_logLevelFromString(const char *level, celix_log_level_e fallbackLogLevel);

/**
 * Converts a const char* value to a celix log level
 * If the provided log level cannot be parsed or is NULL, the fallbackLogLevel is returned and
 * if a convertedSuccessfully pointer is provided this will be set to false.
 * If converted succcessfully and the convertedSuccessfully pointer is provided. This will be set to true.
 */
celix_log_level_e celix_logUtils_logLevelFromStringWithCheck(const char *level, celix_log_level_e fallbackLogLevel, bool *convertedSuccessfully);

/**
 * Logs a message to stdout/stderr using the provided logName and log level.
 * If the provided log level is higher than info, stderr will be used.
 *
 */
void celix_logUtils_logToStdout(const char *logName, celix_log_level_e level, const char *format, ...);

/**
 * Logs a detailed message to stdout/stderr using the provided logName and log level.
 * If the provided log level is higher than info, stderr will be used.
 *
 * The file, function and line arguments are expected to be called with the values:
 * __FILE__, __FUNCTION__ and __LINE__.
 *
 * If the argument file or function is NULL, the arguments file, function and line are not used.
 *
 */
void celix_logUtils_logToStdoutDetails(const char *logName, celix_log_level_e level, const char* file, const char* function, int line, const char *format, ...);


/**
 * Logs a message to stdout/stderr using the provided logName and log level.
 * If the provided log level is higher than info, stderr will be used.
 *
 */
void celix_logUtils_vLogToStdout(const char *logName, celix_log_level_e level, const char *format, va_list formatArgs);

/**
 * Logs - a detailed - messages to stdout/stderr using the provided logName and log level.
 * If the provided log level is higher than info, stderr will be used.
 *
 * The file, function and line arguments are expected to be called with the values:
 * __FILE__, __FUNCTION__ and __LINE__.
 *
 * If the argument file or function is NULL, the arguments file, function and line are not used.
 *
 */
void celix_logUtils_vLogToStdoutDetails(const char *logName, celix_log_level_e level, const char* file, const char* function, int line, const char *format, va_list formatArgs);

/**
 * Prints a backtrace to the provided output stream.
 *
 */
void celix_logUtils_printBacktrace(FILE* stream);

#ifdef __cplusplus
}
#endif

#endif //CELIX_LOG_UTILS_H
