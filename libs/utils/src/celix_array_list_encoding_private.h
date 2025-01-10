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

#ifndef CELIX_CELIX_ARRAY_LIST_ENCODING_PRIVATE_H
#define CELIX_CELIX_ARRAY_LIST_ENCODING_PRIVATE_H

#include "celix_array_list.h"
#include "celix_array_list_encoding.h"//import encodeFlags and decodeFlags
#include "celix_errno.h"

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Encode the provided array list to a json array.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param list The list to encode. Must be not NULL.
 * @param encodeFlags  The flags to use for encoding. See celix_array_list_encoding.h
 * @param out The resulting json array. Caller is owner. Must be not NULL.
 * @return CELIX_SUCCESS if encoding was successful.
 *      CELIX_ILLEGAL_ARGUMENT if list does not support the encoding flags.
 *      ENOMEM if an memory allocation failed.
 */
celix_status_t celix_arrayList_encodeToJson(const celix_array_list_t* list, int encodeFlags, json_t** out);

/**
 * @brief Decode the provided json array to a array list.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param jsonArray The json array to decode. Must be not NULL.
 * @param decodeFlags The flags to use for decoding. See celix_array_list_encoding.h
 * @param out The resulting array list. Caller is owner. Must be not NULL.
 * @return CELIX_SUCCESS if decoding was successful.
 *      CELIX_ILLEGAL_ARGUMENT if jsonArray style is not supported.
 *      ENOMEM if an memory allocation failed.
 */
celix_status_t celix_arrayList_decodeFromJson(const json_t* jsonArray, int decodeFlags, celix_array_list_t** out);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_ARRAY_LIST_ENCODING_PRIVATE_H
