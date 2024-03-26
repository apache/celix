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
 * @file celix_filter.h
 * @brief Header file for the Celix Filter API.
 *
 * #Introduction
 * An Apache Celix filter is a based on LDAP filters, for more information about LDAP filters see:
 *  - https://tools.ietf.org/html/rfc4515
 *  - https://tools.ietf.org/html/rfc1960
 *
 * In short an Apache Celix filter is a tree of filter nodes, parsed from a filter string.
 * Each node has an operand, an optional attribute string and an optional value string.
 * The operand can be one of the following:
 * - `=`: equal
 * - `=*`: present
 * - `~=`: approx
 * - `>=`: greater
 * - `<=`: less
 * - `>=`: greater equal
 * - `<=`: less equal
 * - `=`: substring if string contains a `*`.
 * - `!`: not
 * - `&`: and
 * - `|`: or
 *
 The filter string is a list of filter node strings wrapped in parenthesis to group filter nodes.
 * The following are valid filter strings:
 * - `(key=value)` # Matches if properties contains a entry with key "key" and value "value".
 * - `(key>=2)`# Matches if properties contains a entry with key "key" and value greater or equal to 2.
 * - `(key=*)` # Matches if properties contains a entry with key "key".
 * - `(!(key=value))` # Matches if properties does not contain a entry with key "key" and value "value".
 * - `(&(key1=value1)(key2=value2))` # Matches if properties contains a entry with key "key1" and value "value1" and
 *                                   # a entry with key "key2" and value "value2".
 * - `(|(key1=value1)(key2=value2))` # Matches if properties contains a entry with key "key1" and value "value1" or
 *                                   # a entry with key "key2" and value "value2".
 *
 * Apache Celix filters can be used to match a set of Apache Celix properties and such Apache Celix filters should be
 * used together with a set of properties.
 *
 * #Filter matching and property value types
 * When matching a filter the attribute type of a filter node is used to determine the type of the property value and
 * this type is used to compare the property value with the filter attribute value.
 * If the property value is of a type that the filter attribute value can be parsed to, the filter attribute value the
 * comparison is done with the matching type. If the property value is not of a type that the filter attribute value
 * can be parsed to, the comparison is done with the string representation of the property value.
 *
 * Internally attribute values will be parsed to a long, double, boolean, Apache Celix version and array list of
 * longs, doubles, booleans, Apache Celix versions and string if possible during creation.
 * And these typed attribute values will be used in the to-be-matched property value.
 *
 * Example: The filter "(key>20)" and a property set with a long value 3 for key "key", will not match and the same
 * filter but with a property set which has a string value "3" for key "key", will match.
 *
 * #Filter matching and property value arrays
 * If a filter matches a property set entry which has an array value (either a long, boolean, double, string or version
 * array) the filter match will check if the array contains a single entry which matches the filter attribute value using
 * the aforementioned "Filter matching and property value types" rules.
 *
 * Filter matching with array is supported for the following operands: "=" (including substring), ">=", "<=", ">", "<"
 * and "~=".
 *
 * Example: The filter "(key=3)" will match the property set entry with key "key" and a long array list value of
 *  [1,2,3].
 *
 * #Substring filter operand
 * The substring filter operand is used to match a string value with a filter attribute value. The filter attribute
 * value can contain a `*` to match any character sequence. The filter attribute value can also contain a `*` at the
 * start or end of the string to match any character sequence at the start or end of the string.
 *
 * A substring filter operand uses the string representation of the property value to match with the filter attribute
 * or if the property value is an string array the substring filter operand will match if any of the string array values.
 *
 * Example: The filter "(key=*Smith)" will match the property set entry with key "key" and a string value "John Smith".
 *
 * #Approx filter operand
 * The approx filter operand is used to check if the filter attribute value is a substring of the property value.
 * A approx filter operand uses the string representation of the property value to match with the filter attribute or
 * if the property value is an string array the approx filter operand will match if any of the string array values.
 *
 * Example: The filter "(key~=Smith)" will match the property set entry with key "key" and a string value "John Smith".
 */

#ifndef CELIX_FILTER_H_
#define CELIX_FILTER_H_

#include "celix_filter_type.h"
#include "celix_array_list.h"
#include "celix_cleanup.h"
#include "celix_properties.h"
#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration of the filter operands.
 */
typedef enum celix_filter_operand_enum {
    CELIX_FILTER_OPERAND_EQUAL,
    CELIX_FILTER_OPERAND_APPROX,
    CELIX_FILTER_OPERAND_GREATER,
    CELIX_FILTER_OPERAND_GREATEREQUAL,
    CELIX_FILTER_OPERAND_LESS,
    CELIX_FILTER_OPERAND_LESSEQUAL,
    CELIX_FILTER_OPERAND_PRESENT,
    CELIX_FILTER_OPERAND_SUBSTRING,
    CELIX_FILTER_OPERAND_AND,
    CELIX_FILTER_OPERAND_OR,
    CELIX_FILTER_OPERAND_NOT,
} celix_filter_operand_t;

/**
 * @brief Internal opaque struct for internal use only.
 */
typedef struct celix_filter_internal celix_filter_internal_t; // opaque struct for internal use only

/**
 * @brief The Apache Celix filter struct.
 */
struct celix_filter_struct {
    celix_filter_operand_t operand; /**< The filter operand. */
    const char* attribute;          /**< The filter attribute; NULL for operands `AND`, `OR` or `NOT`. */
    const char* value;              /**< The filter value; NULL for operands `AND`, `OR` or `NOT`. */
    const char* filterStr;          /**< The filter string representation. */

    celix_array_list_t* children; /**< The filter children; only valid if the operand is not `AND`, `OR` or `NOT` else
                                     the value is NULL. */

    celix_filter_internal_t* internal; /**< Internal use only. */
};

/**
 * @brief Create a filter based on the provided filter string.
 *
 * If the return status is NULL, an error message is logged to celix_err.
 *
 * @param filterStr The filter string. If NULL or "" a match-all "(|)" filter is created.
 * @return The created filter or NULL if the filter string is invalid.
 */
CELIX_UTILS_EXPORT celix_filter_t* celix_filter_create(const char* filterStr);

/**
 * @brief Destroy the provided filter. Ignores NULL values.
 */
CELIX_UTILS_EXPORT void celix_filter_destroy(celix_filter_t* filter);

/**
 * @brief Add support for `celix_autoptr`.
 */
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_filter_t, celix_filter_destroy)

/**
 * @brief Check whether the provided filter matches the provided properties.
 * @param[in] filter The filter.
 * @param[in] props The properties.
 * @return True if the filter matches the properties, false otherwise. If filter is NULL always returns true and
 *         if props is NULL, the result is the same as if an empty properties set was provided.
 */
CELIX_UTILS_EXPORT bool celix_filter_match(const celix_filter_t* filter, const celix_properties_t* props);

/**
 * @brief Check whether the 2 filters are equal.
 *
 * Note that the equals is done based on the parsed filter string and not on the provided filter strings.
 * So the filters parsed from the strings "(key=value)" and " (key=value)" are equal.
 *
 * @param[in] filter1 The first filter.
 * @param[in] filter2 The second filter.
 * @return True if the filters are equal, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_filter_equals(const celix_filter_t* filter1, const celix_filter_t* filter2);

/**
 * @brief Get the filter string representation.
 *
 * @param[in] filter The filter.
 * @return The filter string representation. The returned string is owned by the filter, must not be freed and is only
 *         valid as long as the filter is valid.
 */
CELIX_UTILS_EXPORT const char* celix_filter_getFilterString(const celix_filter_t* filter);

/**
 * @brief Find the first filter attribute value matching the provided attribute name.
 *
 * Examples:
 *  - filter arg: "(key=value)", attribute arg: "key" -> "value"
 *  - filter arg: "(key1=value1)(key2=value2)", attribute arg: "key2" -> "value2"
 *  - filter arg: "(|(key1=value1)(key1=value2))", attribute arg: "key1" -> "value1"
 *
 * @param[in] filter The filter.
 * @param[in] attribute The attribute to find.
 * @return The first found attribute or NULL if the attribute is not found. The returned string is owned by the filter,
 *         must not be freed and is only valid as long as the filter is valid.
 */
CELIX_UTILS_EXPORT const char* celix_filter_findAttribute(const celix_filter_t* filter, const char* attribute);

/**
 * @brief Determines if a filter has a mandatory 'equals' attribute with the provided attribute name.
 *
 * This function recursively examines a filter object to determine if it contains  an 'equals' attribute that matches
 * the specified attribute name. The function takes into account the logical operators AND, OR, and NOT in the
 * filter structure, and appropriately handles them to assess the presence and name of the attribute.
 *
 * Example:
 *   using this function for attribute key "key1" on filter "(key1=value1)" yields true.
 *   using this function for attribute key "key1" on filter "(!(key1=value1))" yields false.
 *   using this function for attribute key "key1" on filter "(key1>=value1)" yields false.
 *   using this function for attribute key "key1" on filter "(|(key1=value1)(key2=value2))" yields false.
 *
 * @param[in] filter The filter.
 * @param[in] attribute The attribute to check.
 * @return True if the filter indicates the mandatory presence of an attribute with a specific value for the provided
 *         attribute key, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_filter_hasMandatoryEqualsValueAttribute(const celix_filter_t* filter,
                                                                      const char* attribute);
/**
 * @brief Determines if a filter mandates the absence of a specific attribute, irrespective of its value.
 *
 * This function recursively examines a filter object to determine if it contains a specification indicating the
 * mandatory absence of the specified attribute. It takes into account logical operators AND, OR, and NOT in the filter
 * structure, and appropriately handles them to assess the absence of the attribute.
 *
 * Examples:
 *   using this function for attribute key "key1" on filter "(!(key1=*))" yields true.
 *   using this function for attribute key "key1" on filter "(key1=*)" yields false.
 *   using this function for attribute key "key1" on filter "(key1=value)" yields false.
 *   using this function for attribute key "key1" on filter "(|(!(key1=*))(key2=value2))" yields false.
 *
 * @param[in] filter The filter to examine.
 * @param[in] attribute The attribute to check for mandatory absence.
 * @return True if the filter indicates the mandatory absence of the specified attribute, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_filter_hasMandatoryNegatedPresenceAttribute(const celix_filter_t* filter,
                                                                          const char* attribute);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_FILTER_H_ */
