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

#include "celix_log_level.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_LOG_CONTROL_NAME      "celix_log_control"
#define CELIX_LOG_CONTROL_VERSION   "1.0.0"
#define CELIX_LOG_CONTROL_USE_RANGE "[1.0.0,2)"

typedef struct celix_log_control {
    void *handle;

    size_t (*nrOfLogServices)(void *handle, const char* select);

    size_t (*nrOfSinks)(void *handle, const char* select);

    size_t (*setActiveLogLevels)(void *handle, const char* select, celix_log_level_e activeLogLevel);

    size_t (*setSinkEnabled)(void *handle, const char* select, bool enabled);

    celix_array_list_t* (*currentLogServices)(void *handle);

    celix_array_list_t* (*currentSinks)(void *handle);

    bool (*logServiceInfo)(void *handle, const char* loggerName, celix_log_level_e* outActiveLogLevel);

    bool (*sinkInfo)(void *handle, const char* sinkName, bool *outEnabled);

} celix_log_control_t;

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_CONTROL_H
