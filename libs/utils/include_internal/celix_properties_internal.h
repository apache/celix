/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
*  KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

/**
 * @file celix_properties_internal.h
 * @brief Header file the Apache Celix internal properties API.
 * The internal API is only meant to be used inside the Apache Celix project, so this is not part of the public API.
 */

#ifndef CELIX_CELIX_PROPERTIES_INTERNAL_H
#define CELIX_CELIX_PROPERTIES_INTERNAL_H

#include "celix_hash_map_internal.h"
#include "celix_properties.h"
#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the statistics for the underlining string hash map of the provided properties set.
 */
CELIX_UTILS_EXPORT celix_hash_map_statistics_t celix_properties_getStatistics(const celix_properties_t* properties);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_PROPERTIES_INTERNAL_H
