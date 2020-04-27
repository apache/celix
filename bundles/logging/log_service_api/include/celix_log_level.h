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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum celix_log_level {
    CELIX_LOG_LEVEL_TRACE =     0,
    CELIX_LOG_LEVEL_DEBUG =     1,
    CELIX_LOG_LEVEL_INFO =      2,
    CELIX_LOG_LEVEL_WARNING =   3,
    CELIX_LOG_LEVEL_ERROR =     4,
    CELIX_LOG_LEVEL_FATAL =     5,
    CELIX_LOG_LEVEL_DISABLED =  6
} celix_log_level_e;

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_LEVEL_H
