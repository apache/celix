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

#ifndef CELIX_STRING_HASH_MAP_H_
#define CELIX_STRING_HASH_MAP_H_

#include "celix_hash_map_value.h"

/**
 * @brief Init macro so that the opts are correctly initialized for C++ compilers
 */
#ifdef __cplusplus
#define CELIX_OPTS_INIT {}
#else
#define CELIX_OPTS_INIT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A hash map with uses string (const char*) as key.
 *
 * Entries in the hash map can be a pointer, long, double or bool value.
 * All entries should be of the same type.
 *
 * @note Not thread safe.
 */
typedef struct celix_string_hash_map celix_string_hash_map_t;

/**
 * @Brief long hash map iterator, which contains a hash map entry's keu and value.
 */
typedef struct celix_string_hash_map_iterator {
    size_t index; //iterator index, starting at 0
    const char* key;
    celix_hash_map_value_t value;

    void* _internal[2]; //internal opaque struct
} celix_string_hash_map_iterator_t;

/**
 * @brief Optional create options when creating a string hash map.
 */
typedef struct celix_string_hash_map_create_options {
    /**
     * @brief A simple removed callback, which if provided will be called if a value is removed
     * from the hash map. The removed value is provided as pointer.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both are configured,
     * only the simpledRemoveCallback will be used.
     *
     * Default is NULL.
     */
    void (*simpleRemovedCallback)(void* value) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removedCallback callback.
     *
     * Default is NULL.
     */
    void* removedCallbackData CELIX_OPTS_INIT;

    /**
     * @brief A removed callback, which if provided will be called if a value is removed
     * from the hash map.
     *
     * The callback data pointer will be provided as first argument to the destroy callback.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both are configured,
     * only the simpledRemoveCallback will be used.
     *
     * Default is NULL.
     */
    void (*removedCallback)(void* data, const char* removedKey, celix_hash_map_value_t removedValue) CELIX_OPTS_INIT;

    /**
     * @brief If set to true, the string hash map will not make of copy of the keys and assumes
     * that the keys are in scope/memory for the complete lifecycle of the string hash map.
     *
     * Note that this changes the default behaviour of the celix_stringHashMap_put* functions.
     *
     * Default is false.
     */
    bool storeKeysWeakly CELIX_OPTS_INIT;

    /**
     * @brief The initial hash map capacity.
     *
     * The number of bucket to allocate when creating the hash map.
     *
     * If 0 is provided, the hash map initial capacity will be 16 (default hash map capacity).
     * Default is 0.
     */
     unsigned int initialCapacity CELIX_OPTS_INIT;

     /**
      * @brief The hash map load factor, which controls the max ratio between nr of entries in the hash map and the
      * hash map capacity.
      *
      * The load factor controls how large the hash map capacity (nr of buckets) is compared to the nr of entries
      * in the hash map. The load factor is an important property of the hash map which influences how close the
      * hash map performs to O(1) for its get, has and put operations.
      *
      * If the nr of entries increases above the loadFactor * capacity, the hash capacity will be doubled.
      * For example a hash map with capacity 16 and load factor 0.75 will double its capacity when the 13th entry
      * is added to the hash map.
      *
      * If 0 is provided, the hash map load factor will be 0.75 (default hash map load factor).
      * Default is 0.
      */
     double loadFactor CELIX_OPTS_INIT;
} celix_string_hash_map_create_options_t;

#ifndef __cplusplus
/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#define CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS {    \
    .simpleRemovedCallback = NULL,                      \
    .removedCallbackData = NULL,                        \
    .removedCallback = NULL,                            \
    .storeKeysWeakly = false,                           \
    .initialCapacity = 0,                               \
    .loadFactor = 0                                     \
}
#endif

/**
 * @brief Creates a new empty hash map with a 'const char*' as key.
 */
celix_string_hash_map_t* celix_stringHashMap_create();

/**
 * @brief Creates a new empty string hash map using using the provided hash map create options.
 * @param opts The create options, only used during the creation of the hash map.
 */
celix_string_hash_map_t* celix_stringHashMap_createWithOptions(const celix_string_hash_map_create_options_t* opts);

/**
 * @brief Clears and then deallocated the hash map.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every hash map entry.
 *
 * @param map The hashmap
 */
void celix_stringHashMap_destroy(celix_string_hash_map_t* map);

/**
 * @brief Returns the size of the hashmap.
 */
size_t celix_stringHashMap_size(const celix_string_hash_map_t* map);

/**
 * @brief add pointer entry the string hash map.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return The previous key or NULL of no key was set. Note also returns NULL if the previous value for the key was NULL.
 */
void* celix_stringHashMap_put(celix_string_hash_map_t* map, const char* key, void* value);

/**
 * @brief add long entry the string hash map.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putLong(celix_string_hash_map_t* map, const char* key, long value);

/**
 * @brief add double entry the string hash map.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putDouble(celix_string_hash_map_t* map, const char* key, double value);

/**
 * @brief add bool entry the string hash map.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putBool(celix_string_hash_map_t* map, const char* key, bool value);

/**
 * @brief Returns the value for the provided key.
 *
 * @param map The hashmap..
 * @param key The key to lookup.
 * @return Return the pointer value for the key or NULL. Note will also return NULL if the pointer value for the provided key is NULL.
 */
void* celix_stringHashMap_get(const celix_string_hash_map_t* map, const char* key);

/**
 * @brief Returns the long value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
long celix_stringHashMap_getLong(const celix_string_hash_map_t* map, const char* key, long fallbackValue);

/**
 * @brief Returns the double value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
double celix_stringHashMap_getDouble(const celix_string_hash_map_t* map, const char* key, double fallbackValue);

/**
 * @brief Returns the bool value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
bool celix_stringHashMap_getBool(const celix_string_hash_map_t* map, const char* key, bool fallbackValue);

/**
 * @brief Returns true if the map has the provided key.
 */
bool celix_stringHashMap_hasKey(const celix_string_hash_map_t* map, const char* key);

/**
 * @brief Remove a entry from the hashmap and silently ignore if the hash map does not have a entry with the provided
 * key.
 *
 * @note If the hash map was created with a (simple) remove callback, the callback will have been called if a entry
 * for the provided key was present when this function returns.
 *
 * @return True is the value is found and removed.
 */
bool celix_stringHashMap_remove(celix_string_hash_map_t* map, const char* key);

/**
 * @brief Clear all entries in the hash map.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every hash map entry.
 */
void celix_stringHashMap_clear(celix_string_hash_map_t* map);

/**
 * @brief Create and return a hash map iterator for the beginning of the hash map.
 */
celix_string_hash_map_iterator_t celix_stringHashMap_begin(const celix_string_hash_map_t* map);

/**
 * @brief Check if the iterator is the end of the hash map.
 *
 * @note the end iterator should not be used to retrieve a key of value.
 *
 * @return true if the iterator is the end.
 */
bool celix_stringHashMapIterator_isEnd(const celix_string_hash_map_iterator_t* iter);

/**
 * @brief Moves the provided iterator to the next entry in the hash map.
 */
void celix_stringHashMapIterator_next(celix_string_hash_map_iterator_t* iter);

/**
 * @brief Marco to loop over all the entries of a string hash map.
 *
 * Small example of how to use the iterate macro:
 * @code
 * celix_string_hash_map_t* map = ...
 * CELIX_STRING_HASH_MAP_ITERATE(map, iter) {
 *     printf("Visiting hash map entry with key %s\n", inter.key);
 * }
 * @endcode
 */
#define CELIX_STRING_HASH_MAP_ITERATE(map, iterName) \
    for (celix_string_hash_map_iterator_t iterName = celix_stringHashMap_begin(map); !celix_stringHashMapIterator_isEnd(&(iterName)); celix_stringHashMapIterator_next(&(iterName)))

/**
 * @brief Remove the hash map entry for the provided iterator and updates the iterator to the next hash map entry
 *
 * Small example of how to use the celix_stringHashMapIterator_remove function:
 * @code
 * //remove all even entries from hash map
 * celix_string_hash_map_t* map = ...
 * celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(map);
 * while (!celix_stringHashMapIterator_isEnd(&iter)) {
 *     if (iter.index % 2 == 0) {
 *         celix_stringHashMapIterator_remove(&ter);
 *     } else {
 *         celix_stringHashMapIterator_next(&iter);
 *     }
 * }
 * @endcode
 */
void celix_stringHashMapIterator_remove(celix_string_hash_map_iterator_t* iter);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_STRING_HASH_MAP_H_ */
