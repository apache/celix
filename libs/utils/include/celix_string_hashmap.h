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

#ifndef CELIX_STRING_HASHMAP_H_
#define CELIX_STRING_HASHMAP_H_

#include <stddef.h>

#include "celixbool.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A hashmap with uses string (char*) as key.
 * @note Not thread safe.
 */
typedef struct celix_string_hashmap celix_string_hashmap_t;
//TODO typedef struct celix_long_hashmap celix_long_hashmap_t;

/**
 * @brief Creates a new empty hashmap with a 'const char*' as key.
 */
celix_string_hashmap_t* celix_stringHashmap_create();

/**
 * @brief frees the hashmap.
 * @param map The hashmap
 * @param freeValues If true free will be called on the hashmap ptr values
 */
void celix_stringHashmap_destroy(celix_string_hashmap_t* map, bool freeValues);

/**
 * @brief Returns the size of the hashmap
 */
size_t celix_stringHashmap_size(celix_string_hashmap_t* map);

/**
 * @brief add pointer entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The pointer value to store with the key
 * @return The previous key or NULL of no key was set. Note also returns NULL if the previous value for the key was NULL.
 */
void* celix_stringHashmap_put(celix_string_hashmap_t* map, const char* key, void* value);

/**
 * @brief Returns the value for the provided key.
 *
 * @param map The hashmap.
 * @param key  The key to lookup.
 * @return Return the pointer value for the key or NULL. Note will also return NULL if the pointer value for the provided key is NULL.
 */
void* celix_stringHashmap_get(celix_string_hashmap_t* map, const char* key);

/**
 * @brief Returns true if the map has the provided key.
 */
bool celix_stringHashmap_hasKey(celix_string_hashmap_t* map, const char* key);

/**
 * @brief Remove a entry from the hashmap.
 * @param map The hashmap.
 * @param key The key te remove.
 * @return Return the pointer value for the removed key or NULL. Note will also return NULL if the pointer value for the removed key is NULL.
 */
void* celix_stringHashmap_remove(celix_string_hashmap_t* map, const char* key);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_STRING_HASHMAP_H_ */
