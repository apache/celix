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

#include "celix_cleanup.h"
#include "celix_hash_map_value.h"
#include "celix_utils_export.h"
#include "celix_errno.h"

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
 * @Brief string hash map iterator, which contains a hash map entry's keu and value.
 */
typedef struct celix_string_hash_map_iterator {
    const char* key; /**< The key of the hash map entry. */
    celix_hash_map_value_t value; /**< The value of the hash map entry. */

    void* _internal[2]; /**< internal opaque data, do not use */
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
     *
     * @param[in] removedValue The value that was removed from the hash map. This value is no longer used by the
     *                         hash map and can be freed.
     */
    void (*simpleRemovedCallback)(void* removedValue) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removedCallback and removedKeyCallback callback.
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
     *
     * @param[in] data The void pointer to the data that was provided when the callback was set as removedCallbackData.
     * @param[in] removedKey The key of the value that was removed from the hash map.
     *                       Note that the removedKey can still be in use if the a value entry for the same key is
     *                       replaced, so this callback should not free the removedKey.
     * @param[in] removedValue The value that was removed from the hash map. This value is no longer used by the
     *                         hash map and can be freed.
     */
    void (*removedCallback)(void* data, const char* removedKey, celix_hash_map_value_t removedValue) CELIX_OPTS_INIT;

    /**
     * @brief A removed key callback, which if provided will be called if a key is no longer used in the hash map.
     *
     * @param[in] data The void pointer to the data that was provided when the callback was set as removedCallbackData.
     * @param[in] key The key that is no longer used in the hash map. if `storeKeysWeakly` was configured as true,
     *                the key can be freed.
     */
    void (*removedKeyCallback)(void* data, char* key) CELIX_OPTS_INIT;

    /**
     * @brief If set to true, the string hash map will not make of copy of the keys and assumes
     * that the keys are in scope/memory for the complete lifecycle of the string hash map.
     *
     * When keys are stored weakly it is the caller responsibility to check if a key is already in use
     * (celix_stringHashMap_hasKey).
     *
     * If the key is already in use, the celix_stringHashMap_put* will not store the provided key and will
     * instead keep the already existing key. If needed, the caller may free the provided key.
     * If the key is not already in use, the celix_stringHashMap_put* will store the provided key and the caller
     * should not free the provided key.
     *
     * Uf a celix_stringHashMap_put* returns a error, the caller may free the provided key.
     *
     * @note This changes the default behaviour of the celix_stringHashMap_put* functions.
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
      * @brief The hash map max load factor, which controls the max ratio between nr of entries in the hash map and the
      * hash map capacity.
      *
      * The max load factor controls how large the hash map capacity (nr of buckets) is compared to the nr of entries
      * in the hash map. The load factor is an important property of the hash map which influences how close the
      * hash map performs to O(1) for its get, has and put operations.
      *
      * If the nr of entries increases above the maxLoadFactor * capacity, the hash table capacity will be doubled.
      * For example a hash map with capacity 16 and load factor 0.75 will double its capacity when the 13th entry
      * is added to the hash map.
      *
      * If 0 is provided, the hash map load factor will be 0.75 (default hash map load factor).
      * Default is 0.
      */
     double maxLoadFactor CELIX_OPTS_INIT;
} celix_string_hash_map_create_options_t;

#ifndef __cplusplus
/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#define CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS {    \
    .simpleRemovedCallback = NULL,                      \
    .removedCallbackData = NULL,                        \
    .removedCallback = NULL,                            \
    .removedKeyCallback = NULL,                         \
    .storeKeysWeakly = false,                           \
    .initialCapacity = 0,                               \
    .maxLoadFactor = 0                                  \
}
#endif

/**
 * @brief Creates a new empty hash map with a 'const char*' as key.
 */
CELIX_UTILS_EXPORT celix_string_hash_map_t* celix_stringHashMap_create();

/**
 * @brief Creates a new empty string hash map using using the provided hash map create options.
 * @param opts The create options, only used during the creation of the hash map.
 */
CELIX_UTILS_EXPORT celix_string_hash_map_t* celix_stringHashMap_createWithOptions(const celix_string_hash_map_create_options_t* opts);

/**
 * @brief Clears and then deallocated the hash map.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every hash map entry.
 *
 * @param map The hashmap
 */
CELIX_UTILS_EXPORT void celix_stringHashMap_destroy(celix_string_hash_map_t* map);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_string_hash_map_t, celix_stringHashMap_destroy)

/**
 * @brief Returns the size of the hashmap.
 */
CELIX_UTILS_EXPORT size_t celix_stringHashMap_size(const celix_string_hash_map_t* map);

/**
 * @brief Add pointer entry the string hash map.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] map The hashmap.
 * @param[in] key  The key to use. The hashmap will create a copy if needed.
 * @param[in] value The value to store with the key.
 * @return celix_status_t CELIX_SUCCESS if the entry was added, CELIX_ENOMEM if no memory could be allocated for the
 *         entry.
 */
CELIX_UTILS_EXPORT celix_status_t celix_stringHashMap_put(celix_string_hash_map_t* map, const char* key, void* value);

/**
 * @brief add long entry the string hash map.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return celix_status_t CELIX_SUCCESS if the entry was added, CELIX_ENOMEM if no memory could be allocated for the
 *         entry.
 */
CELIX_UTILS_EXPORT celix_status_t celix_stringHashMap_putLong(celix_string_hash_map_t* map, const char* key, long value);

/**
 * @brief add double entry the string hash map.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @return celix_status_t CELIX_SUCCESS if the entry was added, CELIX_ENOMEM if no memory could be allocated for the
 *         entry.
 */
CELIX_UTILS_EXPORT celix_status_t celix_stringHashMap_putDouble(celix_string_hash_map_t* map, const char* key, double value);

/**
 * @brief add bool entry the string hash map.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param map The hashmap.
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return celix_status_t CELIX_SUCCESS if the entry was added, CELIX_ENOMEM if no memory could be allocated for the
 *         entry.
 */
CELIX_UTILS_EXPORT celix_status_t celix_stringHashMap_putBool(celix_string_hash_map_t* map, const char* key, bool value);

/**
 * @brief Returns the value for the provided key.
 *
 * @param map The hashmap..
 * @param key The key to lookup.
 * @return Return the pointer value for the key or NULL. Note will also return NULL if the pointer value for the provided key is NULL.
 */
CELIX_UTILS_EXPORT void* celix_stringHashMap_get(const celix_string_hash_map_t* map, const char* key);

/**
 * @brief Returns the long value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
CELIX_UTILS_EXPORT long celix_stringHashMap_getLong(const celix_string_hash_map_t* map, const char* key, long fallbackValue);

/**
 * @brief Returns the double value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
CELIX_UTILS_EXPORT double celix_stringHashMap_getDouble(const celix_string_hash_map_t* map, const char* key, double fallbackValue);

/**
 * @brief Returns the bool value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
CELIX_UTILS_EXPORT bool celix_stringHashMap_getBool(const celix_string_hash_map_t* map, const char* key, bool fallbackValue);

/**
 * @brief Returns true if the map has the provided key.
 */
CELIX_UTILS_EXPORT bool celix_stringHashMap_hasKey(const celix_string_hash_map_t* map, const char* key);

/**
 * @brief Remove a entry from the hashmap and silently ignore if the hash map does not have a entry with the provided
 * key.
 *
 * @note If the hash map was created with a (simple) remove callback, the callback will have been called if a entry
 * for the provided key was present when this function returns.
 *
 * @return True is the value is found and removed.
 */
CELIX_UTILS_EXPORT bool celix_stringHashMap_remove(celix_string_hash_map_t* map, const char* key);

/**
 * @brief Clear all entries in the hash map.
 *
 * @note If a (simple) removed callback is configured, the callback will be called for every hash map entry.
 */
CELIX_UTILS_EXPORT void celix_stringHashMap_clear(celix_string_hash_map_t* map);

/**
 * @brief Check if the provided string hash maps are equal.
 *
 * Equals means that both maps have the same number of entries and that all entries in the first map
 * are also present in the second map and have the same value.
 *
 * The value are compared using memcpy, so for pointer values the pointer value is compared and not the value itself.
 *
 * @param[in] map1 The first map to compare.
 * @param[in] map2 The second map to compare.
 * @return true if the maps are equal, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_stringHashMap_equals(const celix_string_hash_map_t* map1, const celix_string_hash_map_t* map2);

/**
 * @brief Get an iterator pointing to the first element in the map.
 *
 * @param[in] map The map to get the iterator for.
 * @return An iterator pointing to the first element in the map.
 */
CELIX_UTILS_EXPORT celix_string_hash_map_iterator_t celix_stringHashMap_begin(const celix_string_hash_map_t* map);

/**
 * @brief Get an iterator pointing to the element following the last element in the map.
 *
 * @param[in] map The map to get the iterator for.
 * @return An iterator pointing to the element following the last element in the map.
 */
celix_string_hash_map_iterator_t celix_stringHashMap_end(const celix_string_hash_map_t* map);

/**
 *
 * @brief Determine if the iterator points to the element following the last element in the map.
 *
 * @param[in] iter The iterator to check.
 * @return true if the iterator points to the element following the last element in the map, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_stringHashMapIterator_isEnd(const celix_string_hash_map_iterator_t* iter);

/**
 * @brief Advance the iterator to the next element in the map.
 * @param[in] iter The iterator to advance.
 */
CELIX_UTILS_EXPORT void celix_stringHashMapIterator_next(celix_string_hash_map_iterator_t* iter);

/**
 * @brief Compares two celix_string_hash_map_iterator_t objects for equality.
 *
 * The value are compared using memcpy, so for pointer values the pointer value is compared and not the value itself.
 *
 * @param[in] iterator The first iterator to compare.
 * @param[in] other The second iterator to compare.
 * @return true if the iterators point to the same entry in the same hash map, false otherwise.
 */
bool celix_stringHashMapIterator_equals(
        const celix_string_hash_map_iterator_t* iterator,
        const celix_string_hash_map_iterator_t* other);

/**
 * @brief Remove the hash map entry for the provided iterator and updates the iterator to the next hash map entry
 *
 * Small example of how to use the celix_stringHashMapIterator_remove function:
 * @code{.c}
 * //remove all entries except the first 4 entries from hash map
 * celix_string_hash_map_t* map = ...
 * celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(map);
 * while (!celix_stringHashMapIterator_isEnd(&iter)) {
 *     if (iter.index >= 4) {
 *         celix_stringHashMapIterator_remove(&ter);
 *     } else {
 *         celix_stringHashMapIterator_next(&iter);
 *     }
 * }
 * @endcode
 */
CELIX_UTILS_EXPORT void celix_stringHashMapIterator_remove(celix_string_hash_map_iterator_t* iter);

/**
 * @brief Marco to loop over all the entries of a string hash map.
 *
 * Small example of how to use the iterate macro:
 * @code{.c}
 * celix_string_hash_map_t* map = ...
 * CELIX_STRING_HASH_MAP_ITERATE(map, iter) {
 *     printf("Visiting hash map entry with key %s\n", inter.key);
 * }
 * @endcode
 *
 * @param map The (const celix_string_hash_map_t*) map to iterate over.
 * @param iterName A iterName which will be of type celix_string_hash_map_iterator_t to hold the iterator.
 */
#define CELIX_STRING_HASH_MAP_ITERATE(map, iterName) \
    for (celix_string_hash_map_iterator_t iterName = celix_stringHashMap_begin(map); !celix_stringHashMapIterator_isEnd(&(iterName)); celix_stringHashMapIterator_next(&(iterName)))


#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_STRING_HASH_MAP_H_ */
