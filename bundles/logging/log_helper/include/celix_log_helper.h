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

#ifndef CELIX_CELIX_LOG_HELPER_H
#define CELIX_CELIX_LOG_HELPER_H

#include <stdarg.h>

#include "celix_log_level.h"
#include "celix_bundle_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_log_helper celix_log_helper_t; //opaque

celix_log_helper_t* celix_logHelper_create(celix_bundle_context_t* ctx, const char* logServiceName);

void celix_logHelper_destroy(celix_log_helper_t* logHelper);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_TRACE level, printf style
 */
void celix_logHelper_trace(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_DEBUG level, printf style
 */
void celix_logHelper_debug(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_INFO level, printf style
 */
void celix_logHelper_info(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_WARNING level, printf style
 */
void celix_logHelper_warning(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_ERROR level, printf style
 */
void celix_logHelper_error(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs to celix_logHelper_log using the CELIX_LOG_LEVEL_FATAL level, printf style
 */
void celix_logHelper_fatal(celix_log_helper_t* logHelper, const char *format, ...);

/**
 * @brief Logs a message using the provided celix log level to the log_helper, printf style.
 *
 * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
 */
void celix_logHelper_log(celix_log_helper_t* logHelper, celix_log_level_e level, const char *format, ...);

/**
 * @brief Logs a detailed message using the provided celix log level to the log_helper, printf style.
 *
 * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
 *
 * The file, function and line arguments are expected to be called with the values:
 * __FILE__, __FUNCTION__ and __LINE__.
 *
 * If the argument file or function is NULL, the arguments file, function and line are not used.
 *
 */
void celix_logHelper_logDetails(celix_log_helper_t* logHelper, celix_log_level_e level, const char* file, const char* function, int line, const char *format, ...);

/**
 * @brief Logs a message to the log_helper using a format string and a va_list argument (vprintf style).
 *
 * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
 */
void celix_logHelper_vlog(celix_log_helper_t* logHelper, celix_log_level_e level, const char *format, va_list formatArgs);

/**
 * @brief Logs a detailed message to log_helper using a format string and a va_list argument (vprintf style).
 *
 * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
 *
 * The file, function and line arguments are expected to be called with the values:
 * __FILE__, __FUNCTION__ and __LINE__.
 *
 * If the argument file or function is NULL, the arguments file, function and line are not used.
 *
 */
void celix_logHelper_vlogDetails(celix_log_helper_t* logHelper, celix_log_level_e level, const char* file, const char* function, int line, const char *format, va_list formatArgs);

/**
 * @brief nr of times a helper log function has been called.
 */
size_t celix_logHelper_logCount(celix_log_helper_t* logHelper);


#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_LOG_HELPER_H
