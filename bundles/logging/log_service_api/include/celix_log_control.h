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

#ifndef CELIX_LOG_CONTROL_H
#define CELIX_LOG_CONTROL_H

#include <stdbool.h>
#include <stddef.h>
#include "celix_log_level.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_LOG_CONTROL_NAME      "celix_log_control"
#define CELIX_LOG_CONTROL_VERSION   "1.1.0"
#define CELIX_LOG_CONTROL_USE_RANGE "[1.1.0,2)"

typedef struct celix_log_control {
    void *handle;

    size_t (*nrOfLogServices)(void *handle, const char* select);

    size_t (*nrOfSinks)(void *handle, const char* select);

    size_t (*setActiveLogLevels)(void *handle, const char* select, celix_log_level_e activeLogLevel);

    size_t (*setSinkEnabled)(void *handle, const char* select, bool enabled);

    /**
     * @brief Get a list of names for the log service provided by the log service.
     * @param handle The service handle.
     * @return A string array list.
     *         The array list is owned by the caller and should be destroyed by calling celix_arrayList_destroy.
     */
    celix_array_list_t* (*currentLogServices)(void *handle);

    /**
     * @brief Get a list of sinks names used by the log service.
     * @param handle The service handle.
     * @return A string array list.
     *        The array list is owned by the caller and should be destroyed by calling celix_arrayList_destroy.
     */
    celix_array_list_t* (*currentSinks)(void *handle);

    bool (*logServiceInfo)(void *handle, const char* loggerName, celix_log_level_e* outActiveLogLevel);

    bool (*sinkInfo)(void *handle, const char* sinkName, bool *outEnabled);

    /**
     * @brief Switch between detailed and brief mode for selected loggers.
     * @details In detailed mode, details(if available) are attached to log messages from the select loggers.
     * For example, the file name, function name, and line number.
     * @since 1.1.0
     * @param[in] handle The service handle.
     * @param[in] select The select string that specifies the case-insensitive name prefix of target loggers.
     * @param[in] detailed True to enable detailed mode, false to disable detailed mode(i.e. enable brief mode).
     * @return Number of logger selected.
     */
    size_t (*setDetailed)(void *handle, const char* select, bool detailed);

    /**
     * @brief Get the active log level and the detailed mode for a selected logger.
     * @details If the logger is not found, the outActiveLogLevel and outDetailed are not changed.
     * @since 1.1.0
     * @param [in] handle The service handle.
     * @param [in] loggerName The name of the target logger.
     * @param [out] outActiveLogLevel The active log level of the target logger.
     * @param [out] outDetailed The detailed mode of the target logger.
     * @return True if the target logger is found, false otherwise.
     */
    bool (*logServiceInfoEx)(void *handle, const char* loggerName, celix_log_level_e* outActiveLogLevel, bool* outDetailed);

} celix_log_control_t;

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_CONTROL_H
