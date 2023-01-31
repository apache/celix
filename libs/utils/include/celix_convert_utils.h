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
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid boolean.
 * @param[out] converted If not NULL, will be set to true if the string is a valid boolean, otherwise false.
 * @return The boolean value.
 */
bool celix_utils_convertStringToBool(const char* val, bool defaultValue, bool* converted);

/**
 * @brief Convert a string to a double.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid double.
 * @param[out] converted If not NULL, will be set to true if the string is a valid double, otherwise false.
 * @return The double value.
 */
double celix_utils_convertStringToDouble(const char* val, double defaultValue, bool* converted);

/**
 * @brief Convert a string to a long.
 *
 * @param[in] val The string to convert.
 * @param[in] defaultValue The default value if the string is not a valid long.
 * @param[out] converted If not NULL, will be set to true if the string is a valid long, otherwise false.
 * @return The long value.
 */
long celix_utils_convertStringToLong(const char* val, long defaultValue, bool* converted);

/**
 * @brief Convert a string to a celix_version_t.
 *
 * @note This convert function will only convert version strings in the format major.minor.micro(.qualifier)?.
 * So the major, minor and micro are required, the qualifier is optional.
 *
 * @param val The string to convert.
 * @return A new celix_version_t* if the string is a valid version, otherwise NULL.
 */
celix_version_t* celix_utils_convertStringToVersion(const char* val);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_CONVERT_UTILS_H
