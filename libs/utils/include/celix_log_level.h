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

#ifndef CELIX_LOG_LEVEL_H
#define CELIX_LOG_LEVEL_H

#include <stdbool.h>

#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_log_level {
    CELIX_LOG_LEVEL_TRACE = 0,
    CELIX_LOG_LEVEL_DEBUG = 1,
    CELIX_LOG_LEVEL_INFO = 2,
    CELIX_LOG_LEVEL_WARNING = 3,
    CELIX_LOG_LEVEL_ERROR = 4,
    CELIX_LOG_LEVEL_FATAL = 5,
    CELIX_LOG_LEVEL_DISABLED = 6
} celix_log_level_e;

/**
 * @brief Converts a log level to a string.
 * @param level The log level to convert.
 * @return The string representation of the log level.
 */
CELIX_UTILS_EXPORT const char* celix_logLevel_toString(celix_log_level_e level);

/**
 * @brief Converts a string to a log level.
 * @param[in] str The string to convert.
 * @param[in] fallbackLogLevel The log level to return if the string is not a valid log level.
 * @return The log level.
 */
CELIX_UTILS_EXPORT celix_log_level_e celix_logLevel_fromString(const char* str, celix_log_level_e fallbackLogLevel);

/**
 * @brief Converts a string to a log level.
 * @param[in] str The string to convert.
 * @param[in] fallbackLogLevel The log level to return if the string is not a valid log level.
 * @param[out] convertedSuccessfully A pointer to a boolean which will be set to true if the string is a valid log
 * level, false otherwise. Can be NULL if the caller is not interested in this information.
 * @return The log level.
 */
CELIX_UTILS_EXPORT celix_log_level_e
celix_logLevel_fromStringWithCheck(const char* str, celix_log_level_e fallbackLogLevel, bool* convertedSuccessfully);

#ifdef __cplusplus
};
#endif

#endif // CELIX_LOG_LEVEL_H
