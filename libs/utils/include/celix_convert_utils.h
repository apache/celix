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

#ifndef CELIX_CELIX_CONVERT_UTILS_H
#define CELIX_CELIX_CONVERT_UTILS_H

#include <stdbool.h>

#include "celix_array_list.h"
#include "celix_errno.h"
#include "celix_utils_export.h"
#include "celix_version.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file celix_convert_utils.h
 * @brief The celix_convert_utils.h file contains utility functions to convert strings to other types.
 */

/**
 * @brief Convert a string to a boolean.
 *
 * Converts a string to a boolean. White space is ignored and the following values are considered booleans (case
 * insensitive): "true', "false".
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid boolean.
 * @param[out] converted If not NULL, will be set to true if the string is a valid boolean, otherwise false.
 * @return The boolean value.
 */
CELIX_UTILS_EXPORT bool celix_utils_convertStringToBool(const char* val, bool defaultValue, bool* converted);

/**
 * @brief Convert a string to a double.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid double.
 * @param[out] converted If not NULL, will be set to true if the string is a valid double, otherwise false.
 * @return The double value.
 */
CELIX_UTILS_EXPORT double celix_utils_convertStringToDouble(const char* val, double defaultValue, bool* converted);

/**
 * @brief Convert a string to a long.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid long.
 * @param[out] converted If not NULL, will be set to true if the string is a valid long, otherwise false.
 * @return The long value.
 */
CELIX_UTILS_EXPORT long celix_utils_convertStringToLong(const char* val, long defaultValue, bool* converted);

/**
 * @brief Convert a string to a celix_version_t.
 *
 * @note This convert function will only convert version strings in the format major.minor.micro(.qualifier)?.
 * So the major, minor and micro are required, the qualifier is optional.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid celix_version_t.
 * @param[out] version The converted version. If the string is not a valid version, the version will be set to a copy of
 *                     the defaultValue.
 * @return CELIX_SUCCESS if the string is a valid version, CELIX_ILLEGAL_ARGUMENT if the string is not a valid version
 * and CELIX_ENOMEM if memory could not be allocated. Note that on a CELIX_ILLEGAL_ARGUMENT the version will be set to a
 * copy of the defaultValue.
 */
CELIX_UTILS_EXPORT celix_status_t celix_utils_convertStringToVersion(const char* val,
                                                                     const celix_version_t* defaultValue,
                                                                     celix_version_t** version);

/**
 * @brief Convert a string to a celix_array_list_t* with long entries.
 *
 * The expected format of the string is a "," separated list of longs. Whitespace is ignored.
 * Long entries are created using celix_utils_convertStringToLong.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid "," separated list of longs.
 * @param[out] list The converted list. If the string is not a valid list, the list will be set to a copy of the
 * defaultValue.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_utils_convertStringToLongArrayList(const char* val,
                                                        const celix_array_list_t* defaultValue,
                                                        celix_array_list_t** list);

/**
 * @brief Convert a celix_array_list_t* with long entries to a string.
 *
 * @param[in] list The list to convert.
 * @return The string representation of the list. The returned string is allocated and should be freed.
 */
CELIX_UTILS_EXPORT
char* celix_utils_longArrayListToString(const celix_array_list_t* list);

/**
 * @brief Convert a string to a celix_array_list_t* with double entries.
 *
 * The expected format of the string is a "," separated list of doubles. Whitespace is ignored.
 * Double entries are created using celix_utils_convertStringToDouble.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid "," separated list of doubles.
 * @param[out] list The converted list. If the string is not a valid list, the list will be set to a copy of the
 * defaultValue.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_utils_convertStringToDoubleArrayList(const char* val,
                                                          const celix_array_list_t* defaultValue,
                                                          celix_array_list_t** list);

/**
 * @brief Convert a celix_array_list_t* with double entries to a string.
 *
 * @param[in] list The list to convert.
 * @return The string representation of the list. The returned string is allocated and should be freed.
 */
CELIX_UTILS_EXPORT
char* celix_utils_doubleArrayListToString(const celix_array_list_t* list);

/**
 * @brief Convert a string to a celix_array_list_t* with boolean entries.
 *
 * The expected format of the string is a "," separated list of booleans. Whitespace is ignored.
 * Boolean entries are converted using celix_utils_convertStringToBool.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid "," separated list of booleans.
 * @param[out] list The converted list. If the string is not a valid list, the list will be set to a copy of the
 * defaultValue.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_utils_convertStringToBoolArrayList(const char* val,
                                                        const celix_array_list_t* defaultValue,
                                                        celix_array_list_t** list);

/**
 * @brief Convert a celix_array_list_t* with boolean entries to a string.
 *
 * @param[in] list The list to convert.
 * @return The string representation of the list. The returned string is allocated and should be freed.
 */
CELIX_UTILS_EXPORT
char* celix_utils_boolArrayListToString(const celix_array_list_t* list);

/**
 * @brief Convert a string to a celix_array_list_t* with string entries.
 *
 * The expected format of the string is a "," separated list of strings. Whitespace is preserved.
 * String entries are copied and the returned list will be configured to call free when entries are removed.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid "," separated list of strings.
 *                         Note that the defaultValue is copied if the string is not a valid list of string entries
 *                         and the defaultValue is expected to be configured with a removed entry callback so the
 * strings are freed.
 * @param[out] list The converted list. If the string is not a valid list, the list will be set to a copy of the
 * defaultValue.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_utils_convertStringToStringArrayList(const char* val,
                                                          const celix_array_list_t* defaultValue,
                                                          celix_array_list_t** list);

/**
 * @brief Convert a celix_array_list_t* with string entries to a string.
 *
 * @param[in] list The list to convert.
 * @return The string representation of the list. The returned string is allocated and should be freed.
 */
CELIX_UTILS_EXPORT
char* celix_utils_stringArrayListToString(const celix_array_list_t* list);

/**
 * @brief Convert a string to a celix_array_list_t* with celix_version_t* entries.
 *
 * The expected format of the string is a "," separated list of celix_version_t* entries. Whitespace is ignored.
 * Version entries are created using celix_utils_convertStringToVersion and the returned list will be configured to call
 * celix_version_destroy when entries are removed.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid "," separated list of string parseable to
 *                         celix_version_t entries. Note that the defaultValue is copied if the string is not a valid
 *                         list of version entries and the defaultValue
 *                         is expected to be configured with a removed entry callback so the versions are freed.
 * @param[out] list The converted list. If the string is not a valid list, the list will be set to a copy of the
 * defaultValue.
 */
CELIX_UTILS_EXPORT
celix_status_t celix_utils_convertStringToVersionArrayList(const char* val,
                                                           const celix_array_list_t* defaultValue,
                                                           celix_array_list_t** list);

/**
 * @brief Convert a celix_array_list_t* with version entries to a string.
 *
 * @param[in] list The list to convert.
 * @return The string representation of the list. The returned string is allocated and should be freed.
 */
CELIX_UTILS_EXPORT
char* celix_utils_versionArrayListToString(const celix_array_list_t* list);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_CONVERT_UTILS_H
