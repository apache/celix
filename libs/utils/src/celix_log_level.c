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

#include "celix_log_utils.h"
#include <strings.h>

static const char* const CELIX_STRING_VALUE_DISABLED = "disabled";
static const char* const CELIX_STRING_VALUE_FATAL = "fatal";
static const char* const CELIX_STRING_VALUE_ERROR = "error";
static const char* const CELIX_STRING_VALUE_WARNING = "warning";
static const char* const CELIX_STRING_VALUE_INFO = "info";
static const char* const CELIX_STRING_VALUE_DEBUG = "debug";
static const char* const CELIX_STRING_VALUE_TRACE = "trace";

const char* celix_logLevel_toString(celix_log_level_e level) {
    switch (level) {
    case CELIX_LOG_LEVEL_DISABLED:
        return CELIX_STRING_VALUE_DISABLED;
    case CELIX_LOG_LEVEL_FATAL:
        return CELIX_STRING_VALUE_FATAL;
    case CELIX_LOG_LEVEL_ERROR:
        return CELIX_STRING_VALUE_ERROR;
    case CELIX_LOG_LEVEL_WARNING:
        return CELIX_STRING_VALUE_WARNING;
    case CELIX_LOG_LEVEL_INFO:
        return CELIX_STRING_VALUE_INFO;
    case CELIX_LOG_LEVEL_DEBUG:
        return CELIX_STRING_VALUE_DEBUG;
    default: // only trace left
        return CELIX_STRING_VALUE_TRACE;
    }
}

celix_log_level_e celix_logLevel_fromString(const char* str, celix_log_level_e fallbackLogLevel) {
    return celix_logLevel_fromStringWithCheck(str, fallbackLogLevel, NULL);
}

celix_log_level_e
celix_logLevel_fromStringWithCheck(const char* str, celix_log_level_e fallbackLogLevel, bool* convertedSuccessfully) {
    celix_log_level_e level = fallbackLogLevel;
    if (convertedSuccessfully != NULL) {
        *convertedSuccessfully = true;
    }
    if (str != NULL) {
        if (strncasecmp(CELIX_STRING_VALUE_DISABLED, str, strlen(CELIX_STRING_VALUE_DISABLED)) == 0) {
            level = CELIX_LOG_LEVEL_DISABLED;
        } else if (strncasecmp(CELIX_STRING_VALUE_FATAL, str, strlen(CELIX_STRING_VALUE_FATAL)) == 0) {
            level = CELIX_LOG_LEVEL_FATAL;
        } else if (strncasecmp(CELIX_STRING_VALUE_ERROR, str, strlen(CELIX_STRING_VALUE_ERROR)) == 0) {
            level = CELIX_LOG_LEVEL_ERROR;
        } else if (strncasecmp(CELIX_STRING_VALUE_WARNING, str, strlen(CELIX_STRING_VALUE_WARNING)) == 0) {
            level = CELIX_LOG_LEVEL_WARNING;
        } else if (strncasecmp(CELIX_STRING_VALUE_INFO, str, strlen(CELIX_STRING_VALUE_INFO)) == 0) {
            level = CELIX_LOG_LEVEL_INFO;
        } else if (strncasecmp(CELIX_STRING_VALUE_DEBUG, str, strlen(CELIX_STRING_VALUE_DEBUG)) == 0) {
            level = CELIX_LOG_LEVEL_DEBUG;
        } else if (strncasecmp(CELIX_STRING_VALUE_TRACE, str, strlen(CELIX_STRING_VALUE_TRACE)) == 0) {
            level = CELIX_LOG_LEVEL_TRACE;
        } else {
            celix_logUtils_logToStdout(
                "logUtils",
                CELIX_LOG_LEVEL_ERROR,
                "Cannot match log level str '%s' to an existing log level. Falling back to log level %s",
                str,
                celix_logUtils_logLevelToString(fallbackLogLevel));
            if (convertedSuccessfully != NULL) {
                *convertedSuccessfully = false;
            }
        }
    } else {
        celix_logUtils_logToStdout(
            "logUtils",
            CELIX_LOG_LEVEL_ERROR,
            "Cannot match NULL log level str to an existing log level. Falling back to log level %s",
            celix_logUtils_logLevelToString(fallbackLogLevel));
        if (convertedSuccessfully != NULL) {
            *convertedSuccessfully = false;
        }
    }
    return level;
}
