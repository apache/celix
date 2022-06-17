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

#include <stddef.h>
#include <stdbool.h>

/**
 * Init macro so that the opts are correctly initialized for C++ compilers
 */
#ifdef __cplusplus
#define CELIX_OPTS_INIT {}
#else
#define CELIX_OPTS_INIT
#endif


#ifdef __cplusplus
extern "C" {
#endif

//TODO split up in celix_string_hash_map.h and celix_long_hash_map.h

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
 * @brief A hash map with uses long as key.
 *
 * Entries in the hash map can be a pointer, long, double or bool value.
 * All entries should be of the same type.
 *
 * @note Not thread safe.
 */
typedef struct celix_long_hash_map celix_long_hash_map_t;

/**
 * Represents a value in the hash map.
 *
 * Because the value of hash map entry can be a pointer, long, double or boolean, the value is represented as an union.
 */
typedef union celix_hash_map_value {
    void* ptrValue;
    long longValue;
    double doubleValue;
    bool boolValue;
} celix_hash_map_value_t;

/**
 * TODO
 */
typedef struct celix_long_hash_map_iterator {
    //public fields
    size_t index; //iterator index, starting at 0
    long key;
    celix_hash_map_value_t value;

    void* _internal[2]; //internal opaque struct
} celix_long_hash_map_iterator_t;

typedef struct celix_string_hash_map_iterator {
    //public fields
    size_t index; //iterator index, starting at 0
    const char* key;
    celix_hash_map_value_t value;

    void* _internal[2]; //internal opaque struct
} celix_string_hash_map_iterator_t;

/**
 * Additional create options when creating a string hash map.
 */
typedef struct celix_string_hash_map_create_options {
    /**
     * @brief A simple removed callback, which if provided will be called if a value is removed
     * from the hash map. The removed value is provided as pointer.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*simpleRemovedCallback)(void* value) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removedCallback callback.
     */
    void* removedCallbackData CELIX_OPTS_INIT;

    /**
     * @brief A removed callback, which if provided will be called if a value is removed
     * from the hash map.
     *
     * The callback data pointer will be provided as first argument to the destroy callback.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*removedCallback)(void* data, const char* removedKey, celix_hash_map_value_t removedValue) CELIX_OPTS_INIT;

    /**
     * @brief If set to true, the string hash map will not make of copy of the keys and assumes
     * that the keys are in scope/memory for the complete lifecycle of the string hash map.
     *
     * Note that this changes the default behaviour of the celix_stringHashMap_put* functions.
     */
    bool storeKeysWeakly CELIX_OPTS_INIT;

    /**
     * @brief The initial hash map capacity.
     *
     * The number of bucket to allocate when creating the hash map.
     * If 0 is provided, the hash map initial capacity will be 16 (default value).
     */
     unsigned int initialCapacity;

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
      * If 0 is provided, the hash map load factor will be 0.75 (default value).
      */
     double loadFactor;
} celix_string_hash_map_create_options_t;

/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS {    \
    .simpleRemovedCallback = NULL,                      \
    .removedCallbackData = NULL,                        \
    .removedCallback = NULL,                            \
    .storeKeysWeakly = false                            \
}
#endif

/**
 * Additional create options when creating a long hash map.
 */
typedef struct celix_long_hash_map_create_options {
    /**
     * @brief A simple removed callback, which if provided will be called if a value is removed
     * from the hash map. The removed value is provided as pointer.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*simpledRemoveCallback)(void* value) CELIX_OPTS_INIT;

    /**
     * Optional callback data, which will be provided to the removeCallback callback.
     */
    void* removedCallbackData CELIX_OPTS_INIT;

    /**
     * @brief A destroy callback, which if provided will be called if a value is removed
     * from the hash map.
     *
     * The callback data pointer will be provided as first argument to the destroy callback.
     *
     * @note only simpleRemovedCallback or removedCallback should be configured, if both as configured,
     * only the simpledRemoveCallback will be used.
     */
    void (*removedCallback)(void* data, long removedKey, celix_hash_map_value_t removedValue) CELIX_OPTS_INIT;

    /**
     * @brief The initial hash map capacity.
     *
     * The number of bucket to allocate when creating the hash map.
     * If 0 is provided, the hash map initial capacity will be 16 (default value).
     */
    unsigned int initialCapacity;

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
     * If 0 is provided, the hash map load factor will be 0.75 (default value).
     */
    double loadFactor;
} celix_long_hash_map_create_options_t;

/**
 * @brief C Macro to create a empty string_hash_map_create_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS {      \
    .simpledRemoveCallback = NULL,                      \
    .removedCallbackData = NULL,                        \
    .removedCallback = NULL                             \
}
#endif

/**
 * @brief Creates a new empty hashmap with a 'const char*' as key.
 */
celix_string_hash_map_t* celix_stringHashMap_create();

/**
 * @brief Creates a new empty hashmap with a long as key.
 */
celix_long_hash_map_t* celix_longHashMap_create();

/**
 * TODO
 * @param opts
 * @return
 */
celix_string_hash_map_t* celix_stringHashMap_createWithOptions(const celix_string_hash_map_create_options_t* opts);

/**
 * TODO
 * @param opts
 * @return
 */
celix_long_hash_map_t* celix_longHashMap_createWithOptions(const celix_long_hash_map_create_options_t* opts);

/**
 * @brief frees the hashmap.
 *
 * @param map The hashmap
 */
void celix_stringHashMap_destroy(celix_string_hash_map_t* map);

/**
 * @brief frees the hashmap.
 *
 * @param map The hashmap
 */
void celix_longHashMap_destroy(celix_long_hash_map_t* map);

/**
 * @brief Returns the size of the hashmap
 */
size_t celix_stringHashMap_size(const celix_string_hash_map_t* map);

/**
 * @brief Returns the size of the hashmap
 */
size_t celix_longHashMap_size(const celix_long_hash_map_t* map);

/**
 * @brief add pointer entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key
 * @return The previous key or NULL of no key was set. Note also returns NULL if the previous value for the key was NULL.
 */
void* celix_stringHashMap_put(celix_string_hash_map_t* map, const char* key, void* value);

/**
 * @brief add long entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putLong(celix_string_hash_map_t* map, const char* key, long value);

/**
 * @brief add double entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putDouble(celix_string_hash_map_t* map, const char* key, double value);

/**
 * @brief add bool entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use. The hashmap will create a copy if needed.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_stringHashMap_putBool(celix_string_hash_map_t* map, const char* key, bool value);

/**
 * @brief add pointer entry the string hash map.
 *
 * @param map The hashmap
 * @param key  The key to use.
 * @param value The value to store with the key
 * @return The previous key or NULL of no key was set. Note also returns NULL if the previous value for the key was NULL.
 */
void* celix_longHashMap_put(celix_long_hash_map_t* map, long key, void* value);

/**
 * @brief add long entry the long hash map.
 *
 * @param map The hashmap
 * @param key  The key to use.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_longHashMap_putLong(celix_long_hash_map_t* map, long key, long value);

/**
 * @brief add double entry the long hash map.
 *
 * @param map The hashmap
 * @param key  The key to use.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_longHashMap_putDouble(celix_long_hash_map_t* map, long key, double value);

/**
 * @brief add bool entry the long hash map.
 *
 * @param map The hashmap
 * @param key  The key to use.
 * @param value The value to store with the key.
 * @return True if a previous value with the provided has been replaced.
 */
bool celix_longHashMap_putBool(celix_long_hash_map_t* map, long key, bool value);

/**
 * @brief Returns the value for the provided key.
 *
 * @param map The hashmap.
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
 * @brief Returns the value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @return Return the pointer value for the key or NULL. Note will also return NULL if the pointer value for the provided key is NULL.
 */
void* celix_longHashMap_get(const celix_long_hash_map_t* map, long key);

/**
 * @brief Returns the long value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
long celix_longHashMap_getLong(const celix_long_hash_map_t* map, long key, long fallbackValue);

/**
 * @brief Returns the long value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
double celix_longHashMap_getDouble(const celix_long_hash_map_t* map, long key, double fallbackValue);

/**
 * @brief Returns the long value for the provided key.
 *
 * @param map The hashmap.
 * @param key The key to lookup.
 * @param fallbackValue The fallback value to use if a value for the provided key was not found.
 * @return Return the value for the key or fallbackValue if the key is not present.
 */
bool celix_longHashMap_getBool(const celix_long_hash_map_t* map, long key, bool fallbackValue);

/**
 * @brief Returns true if the map has the provided key.
 */
bool celix_stringHashMap_hasKey(const celix_string_hash_map_t* map, const char* key);

/**
 * @brief Returns true if the map has the provided key.
 */
bool celix_longHashMap_hasKey(const celix_long_hash_map_t* map, long key);

/**
 * @brief Remove a entry from the hashmap and return the removed value.
 *
 * @note if the hashmap is created with a destroy value callback, the returned ptr value will already be freed.
 *
 * @param map The hashmap.
 * @param key The key te remove.
 * @return Return the pointer value for the removed key or NULL. Note will also return NULL if the pointer value for the removed key is NULL.
 */
void* celix_stringHashMap_removeAndReturn(celix_string_hash_map_t* map, const char* key);

/**
 * @brief Remove a entry from the hashmap and return the removed value.
 *
 * @note if the hashmap is created with a destroy value callback, the returned ptr value will already be freed.
 *
 * @param map The hashmap.
 * @param key The key te remove.
 * @return Return the pointer value for the removed key or NULL. Note will also return NULL if the pointer value for the removed key is NULL.
 */
void* celix_longHashMap_removeAndReturn(celix_long_hash_map_t* map, long key);

/**
 * @brief Remove a entry from the hashmap and silently ignore if the hash map does not have a entry with the provided
 * key.
 * @return True is the value is found and removed.
 */
bool celix_stringHashMap_remove(celix_string_hash_map_t* map, const char* key);

/**
 * @brief Remove a entry from the hashmap and silently ignore if the hash map does not have a entry with the provided
 * key.
 * @return True is the value is found and removed.
 */
bool celix_longHashMap_remove(celix_long_hash_map_t* map, long key);

/**
 * TODO
 * @param map
 */
void celix_stringHashMap_clear(celix_string_hash_map_t* map);

/**
 * TODO
 * @param map
 */
void celix_longHashMap_clear(celix_long_hash_map_t* map);


/**
 * TODO
 * @param map
 * @return
 */
celix_string_hash_map_iterator_t celix_stringHashMap_iterate(const celix_string_hash_map_t* map);

/**
 * TODO
 * @param iter
 * @return
 */
bool celix_stringHashMapIterator_isEnd(const celix_string_hash_map_iterator_t* iter);

/**
 * TODO
 * @param iter
 * @return
 */
void celix_stringHashMapIterator_next(celix_string_hash_map_iterator_t* iter);

/**
 * TODO
 */
#define CELIX_STRING_HASH_MAP_ITERATE(map, iterName) \
    for (celix_string_hash_map_iterator_t iterName = celix_stringHashMap_iterate(map); !celix_stringHashMapIterator_isEnd(&iterName); celix_stringHashMapIterator_next(&iterName))

/**
 * TODO move iter to next entry and removes hash map entry of provided entry key.
 * @param iter
 * @return
 */
void celix_stringHashMapIterator_remove(celix_string_hash_map_iterator_t* iter);

/**
 * TODO
 * @param map
 * @return
 */
celix_long_hash_map_iterator_t celix_longHashMap_iterate(const celix_long_hash_map_t* map);

/**
 * TODO
 * @param iter
 * @return
 */
bool celix_longHashMapIterator_isEnd(const celix_long_hash_map_iterator_t* iter);

/**
 * TODO
 * @param iter
 * @return
 */
void celix_longHashMapIterator_next(celix_long_hash_map_iterator_t* iter);

/**
 * TODO
 */
#define CELIX_LONG_HASH_MAP_ITERATE(map, iterName) \
    for (celix_long_hash_map_iterator_t iterName = celix_longHashMap_iterate(map); !celix_longHashMapIterator_isEnd(&iterName); celix_longHashMapIterator_next(&iterName))

/**
 * TODO move iter to next entry and removes hash map entry of provided entry key.
 * @param iter
 */
void celix_longHashMapIterator_remove(celix_long_hash_map_iterator_t* iter);

#ifdef __cplusplus
}
#endif

#undef CELIX_OPTS_INIT

#endif /* CELIX_STRING_HASH_MAP_H_ */
