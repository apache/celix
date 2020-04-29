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

#ifndef CELIX_LOG_WRITER_H
#define CELIX_LOG_WRITER_H

#include <stdarg.h>

#include "celix_log_level.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_LOG_SINK_NAME             "celix_log_sink"
#define CELIX_LOG_SINK_VERSION          "1.0.0"
#define CELIX_LOG_SINK_USE_RANGE        "[1.0.0,2)"

#define CELIX_LOG_SINK_PROPERTY_NAME    "name"

typedef struct celix_log_sink {
    void *handle;

    /**
     * Sink a log message, vprintf style.
     *
     * @param handle            The service handle.
     * @param level             The log level.
     * @param logServiceId      The service id of the log service used to log a message.
     * @param logServiceName    The name of the log service used to log a message.
     * @param file              An optional file line argument (only valid if function is also not NULL)
     * @param function          An optional function line argument (only valid if file is also not NULL)
     * @param line              An optional file line argument (only valid if file and function are not NULL)
     * @param format            The log message in a format string (i.e. a printf format)
     * @param formatArgs        va_list of the variable format arg
     */
    void (*sinkLog)(void *handle, celix_log_level_e level, long logServiceId, const char* logServiceName, const char* file, const char* function, int line, const char *format, va_list formatArgs);
} celix_log_sink_t;

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_WRITER_H
