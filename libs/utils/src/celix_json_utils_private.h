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

#ifndef CELIX_CELIX_JSON_UTILS_PRIVATE_H
#define CELIX_CELIX_JSON_UTILS_PRIVATE_H

#include "celix_errno.h"
#include "celix_version.h"

#include <jansson.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a celix_version_t to a json object.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param version The version to convert. Must be not NULL.
 * @param out The converted json object. Caller is owner. Must be not NULL.
 * @return CELIX_SUCCESS if conversion was successful.
 *       ENOMEM if an memory allocation failed.
 */
celix_status_t celix_utils_versionToJson(const celix_version_t* version, json_t** out);

/**
 * @brief Convert a json object to a celix_version_t.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param json The json object to convert. Must be not NULL.
 * @param out The converted celix_version_t. Caller is owner. Must be not NULL.
 * @return CELIX_SUCCESS if conversion was successful.
 *       CELIX_ILLEGAL_ARGUMENT if json object is not a version string.
 *       ENOMEM if an memory allocation failed.
 */
celix_status_t celix_utils_jsonToVersion(const json_t* json, celix_version_t** out);

/**
 * @brief Check if the given json object is a version string.
 * @param string The json object to check.
 * @return true if the json object is a version string.
 */
bool celix_utils_isVersionJsonString(const json_t* string);

/**
 * @brief Convert a json error code to a celix_status_t.
 * @param error The json error code.
 * @return The corresponding celix_status_t.
 */
celix_status_t celix_utils_jsonErrorToStatus(enum json_error_code error);

/**
 * @brief Convert binary data to a json object.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] data The binary data to convert. Must be not NULL.
 * @param[in] size The size of the binary data.
 * @param[out] out The converted json object. Caller is owner. Must be not NULL.
 * @return CELIX_SUCCESS if conversion was successful.
 *       ENOMEM if an memory allocation failed.
 */
celix_status_t celix_utils_binaryToJson(const void* data, size_t size, json_t** out);

/**
 * @brief Convert a json object to binary data.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] json The json object to convert. Must be not NULL.
 * @param[out] data The converted binary data. Caller is owner. Must be not NULL.
 * @param[out] size The size of the binary data. Must be not NULL.
 * @return CELIX_SUCCESS if conversion was successful.
 *       CELIX_ILLEGAL_ARGUMENT if json object is not a binary string.
 *       ENOMEM if memory allocation failed.
 */
celix_status_t celix_utils_jsonToBinary(const json_t* json, void** data, size_t* size);

/**
 * @brief Check if the given json object is a binary string.
 * @param[in] string The json object to check.
 * @return true if the json object is a binary string.
 */
bool celix_utils_isBinaryJsonString(const json_t* string);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_JSON_UTILS_PRIVATE_H
