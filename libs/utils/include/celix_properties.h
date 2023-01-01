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
 * @file celix_properties.h
 * @brief Header file for the Celix Properties API.
 *
 * The Celix Properties API provides a means for storing and manipulating key-value pairs, called properties,
 * which can be used to store configuration data or metadata for a service, component, or framework configuration.
 * Functions are provided for creating and destroying property sets, loading and storing properties from/to a file
 * or stream, and setting, getting, and unsetting individual properties. There are also functions for converting
 * property values to various types (e.g. long, bool, double) and for iterating over the properties in a set.
 */

#include <stdio.h>
#include <stdbool.h>

#include "celix_errno.h"

#ifndef CELIX_PROPERTIES_H_
#define CELIX_PROPERTIES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief celix_properties_t is a type that represents a set of key-value pairs called properties,
 * which can be used to store configuration data or metadata for a services, components or framework configuration.
 */
typedef struct celix_properties celix_properties_t;

//TODO rethink the iter. Maybe this can be more closely coupled to celix_string_hash_map?
//typedef struct celix_properties_iterator {
//    //private data
//    void* _data1;
//    void* _data2;
//    void* _data3;
//    int _data4;
//    int _data5;
//} celix_properties_iterator_t;

/**
 * @brief Type representing an iterator for iterating over the properties in a property set.
 */
typedef struct celix_properties_iterator {
    //private opaque data
    char _data[56];
} celix_properties_iterator_t;


/**
 * @brief Creates a new empty property set.
 * @return A new empty property set.
 */
celix_properties_t* celix_properties_create(void);

/**
 * @brief Destroys a property set, freeing all associated resources.
 *
 * @param properties The property set to destroy. If properties is NULL, this function will do nothing.
 */
void celix_properties_destroy(celix_properties_t *properties);

/**
 * @brief Loads properties from a file.
 *
 * @param filename The name of the file to load properties from.
 * @return A property set containing the properties from the file.
 * @retval NULL If an error occurred (e.g. file not found).
 */
celix_properties_t* celix_properties_load(const char *filename);


/**
 * @brief Loads properties from a stream.
 *
 * @param stream The stream to load properties from.
 * @return A property set containing the properties from the stream.
 * @retval NULL If an error occurred (e.g. invalid format).
 */
celix_properties_t* celix_properties_loadWithStream(FILE *stream);

/**
 * @brief Loads properties from a string.
 *
 * @param input The string to load properties from.
 * @return A property set containing the properties from the string.
 * @retval NULL If an error occurred (e.g. invalid format).
 */
celix_properties_t* celix_properties_loadFromString(const char *input);

/**
 * @brief Stores properties to a file.
 *
 * @param properties The property set to store.
 * @param file The name of the file to store the properties to.
 * @param header An optional header to write to the file before the properties.
 */
void celix_properties_store(celix_properties_t *properties, const char *file, const char *header); //TODO add return status

/**
 * @brief Gets the value of a property.
 *
 * @param properties The property set to search.
 * @param key The key of the property to get.
 * @param defaultValue The value to return if the property is not set.
 * @return The value of the property, or the default value if the property is not set.
  */
const char* celix_properties_get(const celix_properties_t *properties, const char *key, const char *defaultValue);

/**
 * @brief Sets the value of a property.
 *
 *
 * @param properties The property set to modify.
 * @param key The key of the property to set.
 * @param value The value to set the property to.
 */
void celix_properties_set(celix_properties_t *properties, const char *key, const char *value);

/**
 * @brief Sets the value of a property without copying the key and value strings.
 *
 * @param properties The property set to modify.
 * @param key The key of the property to set. This string will be used directly, so it must not be freed or modified
 *            after calling this function.
 * @param value The value to set the property to. This string will be used directly, so it must not be freed or
 *              modified after calling this function.
 */
void celix_properties_setWithoutCopy(celix_properties_t *properties, char *key, char *value);

/**
 * @brief Unsets a property, removing it from the property set.
 * @param properties The property set to modify.
 * @param key The key of the property to unset.
 */
void celix_properties_unset(celix_properties_t *properties, const char *key);

/**
 * @brief Makes a copy of a property set.
 *
 * @param properties The property set to copy.
 * @return A copy of the given property set.
 */
celix_properties_t* celix_properties_copy(const celix_properties_t *properties);

/**
 * @brief Gets the value of a property as a long integer.
 *
 * @param props The property set to search.
 * @param key The key of the property to get.
 * @param defaultValue The value to return if the property is not set or cannot be converted to a long.
 * @return The value of the property as a long, or the default value if the property is not set or cannot be converted
 *         to a long.
 */
long celix_properties_getAsLong(const celix_properties_t *props, const char *key, long defaultValue);

/**
 * @brief Sets the value of a property to a long integer.
 *
 * @param props The property set to modify.
 * @param key The key of the property to set.
 * @param value The long value to set the property to.
 */
void celix_properties_setLong(celix_properties_t *props, const char *key, long value);

/**
 * @brief Gets the value of a property as a boolean.
 *
 * @param props The property set to search.
 * @param key The key of the property to get.
 * @param defaultValue The value to return if the property is not set or cannot be converted to a boolean.
 * @return The value of the property as a boolean, or the default value if the property is not set or cannot be
 *         converted to a boolean.
 */
bool celix_properties_getAsBool(const celix_properties_t *props, const char *key, bool defaultValue);

/**
 * @brief Sets the value of a property to a boolean.
 *
 * @param props The property set to modify.
 * @param key The key of the property to set.
 * @param val The boolean value to set the property to.
 */
void celix_properties_setBool(celix_properties_t *props, const char *key, bool val);

/**
 * @brief Sets the value of a property to a double.
 *
 * @param props The property set to modify.
 * @param key The key of the property to set.
 * @param val The double value to set the property to.
 */
void celix_properties_setDouble(celix_properties_t *props, const char *key, double val);

/**
 * @brief Gets the value of a property as a double.
 *
 * @param props The property set to search.
 * @param key The key of the property to get.
 * @param defaultValue The value to return if the property is not set or cannot be converted to a double.
 * @return The value of the property as a double, or the default value if the property is not set or cannot be converted to a double.
 */
double celix_properties_getAsDouble(const celix_properties_t *props, const char *key, double defaultValue);

/**
 * @brief Gets the number of properties in a property set.
 *
 * @param properties The property set to get the size of.
 * @return The number of properties in the property set.
 */
int celix_properties_size(const celix_properties_t *properties);

/**
 * @brief Constructs a new iterator for iterating over the properties in a property set.
 *
 * @param properties The property set to iterate over.
 * @return A new iterator for the given property set.
 */
celix_properties_iterator_t celix_propertiesIterator_construct(const celix_properties_t *properties);

/**
 * @brief Determines whether the iterator has a next property.
 *
 * @param iter The iterator to check.
 * @return true if the iterator has a next property, false otherwise.
 */
bool celix_propertiesIterator_hasNext(celix_properties_iterator_t *iter);

/**
 * @brief Gets the next key in the iterator.
 * @param iter The iterator to get the next key from.
 * @return The next key in the iterator.
 */
const char* celix_propertiesIterator_nextKey(celix_properties_iterator_t *iter);

/**
 * @brief Gets the property set being iterated over.
 *
 * @param iter The iterator to get the property set from.
 * @return The property set being iterated over.
 */
celix_properties_t* celix_propertiesIterator_properties(celix_properties_iterator_t *iter);

/**
 * @brief Determines whether two iterators are equal.
 *
 * @param a The first iterator to compare.
 * @param b The second iterator to compare.
 * @return true if the iterators are equal, false otherwise.
 */
bool celix_propertiesIterator_equals(const celix_properties_iterator_t* a, const celix_properties_iterator_t* b);

/**
 * @brief Macro for iterating over the properties in a property set.
 *
 * @param props The property set to iterate over.
 * @param key The variable to use for the current key in the loop.
 *
 * Example usage:
 * @code{.c}
 * celix_properties_t *props = celix_properties_create();
 * celix_properties_set(props, "key1", "value1");
 * celix_properties_set(props, "key2", "value2");
 * celix_properties_set(props, "key3", "value3");
 *
 * const char* key;
 * CELIX_PROPERTIES_FOR_EACH(props, key) {
 *     printf("%s = %s\n", key, celix_properties_get(props, key, ""));
 * }
 * @endcode
 * Output:
 * @code{.c}
 * key1 = value1
 * key2 = value2
 * key3 = value3
 * @endcode
*/
#define CELIX_PROPERTIES_FOR_EACH(props, key) \
    for(celix_properties_iterator_t iter_##key = celix_propertiesIterator_construct(props); \
        celix_propertiesIterator_hasNext(&iter_##key), (key) = celix_propertiesIterator_nextKey(&iter_##key);)

#ifdef __cplusplus
}
#endif

#endif /* CELIX_PROPERTIES_H_ */
