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
 * @file celix_hash_map_internal.h
 * @brief Header file the Apache Celix internal long and string hashmap API.
 * The internal API is only meant to be used inside the Apache Celix project, so this is not part of the public API.
 */

#ifndef CELIX_CELIX_HASH_MAP_INTERNAL_H
#define CELIX_CELIX_HASH_MAP_INTERNAL_H

#include "celix_utils_export.h"
#include "celix_string_hash_map.h"
#include "celix_long_hash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Statistics for a hash map.
 */
typedef struct celix_hash_map_statistics {
    size_t nrOfEntries;
    size_t nrOfBuckets;
    size_t resizeCount;
    double averageNrOfEntriesPerBucket;
    double stdDeviationNrOfEntriesPerBucket;
} celix_hash_map_statistics_t;

/**
 * @brief Return the statistics for the given hash map.
 */
CELIX_UTILS_EXPORT celix_hash_map_statistics_t celix_longHashMap_getStatistics(const celix_long_hash_map_t* map);

/**
 * @brief Return the statistics for the given hash map.
 */
CELIX_UTILS_EXPORT celix_hash_map_statistics_t celix_stringHashMap_getStatistics(const celix_string_hash_map_t* map);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_HASH_MAP_INTERNAL_H
