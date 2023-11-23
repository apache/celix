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

/**
 * @file celix_hash_map_private.h
 * @brief Private header for celix_hash_map, with function used for whitebox testing.
 * The symbols of private headers are not exported and as such cannot be used by other libraries, except for testing.
 */

#ifndef CELIX_CELIX_HASH_MAP_PRIVATE_H
#define CELIX_CELIX_HASH_MAP_PRIVATE_H

#include "celix_errno.h"
#include "celix_hash_map_value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_hash_map celix_hash_map_t;             // opaque
typedef struct celix_hash_map_entry celix_hash_map_entry_t; // opaque
typedef union celix_hash_map_key celix_hash_map_key_t;      // opaque

typedef enum celix_hash_map_key_type { CELIX_HASH_MAP_STRING_KEY, CELIX_HASH_MAP_LONG_KEY } celix_hash_map_key_type_e;

/**
 * @brief Resizes the hash map to the next allowed size.
 */
celix_status_t celix_hashMap_resize(celix_hash_map_t* map);

/**
 * @brief Add a new entry to the hash map.
 */
celix_status_t
celix_hashMap_addEntry(celix_hash_map_t* map, const celix_hash_map_key_t* key, const celix_hash_map_value_t* value);

/**
 * @brief Initialize the hash map.
 */
celix_status_t celix_hashMap_init(celix_hash_map_t* map,
                                  celix_hash_map_key_type_e keyType,
                                  unsigned int initialCapacity,
                                  double maxLoadFactor);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_HASH_MAP_PRIVATE_H
