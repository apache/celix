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

#ifndef CELIX_LOG_SERVICE_H
#define CELIX_LOG_SERVICE_H

#include "celix_log_level.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_LOG_SERVICE_NAME              "celix_log_service"
#define CELIX_LOG_SERVICE_VERSION           "1.0.0"
#define CELIX_LOG_SERVICE_USE_RANGE         "[1.0.0,2)"

#define CELIX_LOG_SERVICE_PROPERTY_NAME     "name"

/**
 * Celix log service. The log service can be requested for logging purpose.
 *
 * When requesting this service a name can be used in the service filter. If the name is present,
 * a logging instance for that name will be created.
 *
 * Names can be used to create a hierarchy for loggers, e.g.:
 * - (name=celix_pubsub_PubSubTcpAdmin)
 * - (name=celix_pubsub_PubSubZqmAdmin)
 * - (name=celix_HttpAdmin)
 */
typedef struct celix_log_service {
    void *handle;

    /**
     * Logs a trace message, printf style.
     */
    void (*trace)(void *handle, const char* format, ...);

    /**
     * Logs a debug message, printf style.
     */
    void (*debug)(void *handle, const char* format, ...);

    /**
     * Logs a info message, printf style.
     */
    void (*info)(void *handle, const char* format, ...);

    /**
     * Logs a warning message, printf style.
     */
    void (*warning)(void *handle, const char* format, ...);

    /**
     * Logs a error message, printf style.
     */
    void (*error)(void *handle, const char* format, ...);

    /**
     * Logs a fatal message, printf style.
     */
    void (*fatal)(void *handle, const char* format, ...);

    /**
     * Logs a message using the provided log level, printf style
     * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
     */
    void (*log)(void *handle, celix_log_level_e level, const char* format, ...);

    /**
     * Logs a message using the provided log level, printf style
     * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
     *
     * The file, function and line arguments are expected to be called with the values:
     * __FILE__, __FUNCTION__ and __LINE__.
     *
     * If the argument file or function is NULL, the arguments file, function and line are not used.
     */
    void (*logDetails)(void *handle, celix_log_level_e level, const char* file, const char* function, int line, const char* format, ...);

    /**
     * Log a message using a format string and va_list argument (vprintf style)
     * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
     */
    void (*vlog)(void *handle, celix_log_level_e level, const char* format, va_list formatArgs);

    /**
     * Log a detailed message using a format string and va_list argument (vprintf style)
     * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
     *
     * The file, function and line arguments are expected to be called with the values:
     * __FILE__, __FUNCTION__ and __LINE__.
     *
     * If the argument file or function is NULL, the arguments file, function and line are not used.
     */
    void (*vlogDetails)(void *handle, celix_log_level_e level, const char* file, const char* function, int line, const char* format, va_list formatArgs);
} celix_log_service_t;

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_SERVICE_H
