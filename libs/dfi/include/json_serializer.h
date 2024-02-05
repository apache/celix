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

#ifndef __JSON_SERIALIZER_H_
#define __JSON_SERIALIZER_H_

#include <jansson.h>
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Deserialize a JSON string buffer to a given type.
 * @note The string buffer doesn't need to be null-terminated.
 *
 * Caller is the owner of the out parameter and should release it using dynType_free.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The type to deserialize to.
 * @param[in] input The JSON string buffer to deserialize.
 * @param[in] length The length of the given JSON string buffer.
 * @param[out] result The deserialized result.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonSerializer_deserialize(const dyn_type* type, const char* input, size_t length, void** result);

/**
 * @brief Deserialize a JSON object to a given type.
 *
 * Caller is the owner of the out parameter and should release it using dynType_free.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The type to deserialize to.
 * @param[in] input The JSON object to deserialize.
 * @param[out] out The deserialized result.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonSerializer_deserializeJson(const dyn_type* type, json_t* input, void** result);

/**
 * @brief Serialize a given type to a JSON string.
 *
 * Caller is the owner of the out parameter and should release it using free.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The type to serialize.
 * @param[in] input The input to serialize.
 * @param[out] out The serialized result.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonSerializer_serialize(const dyn_type* type, const void* input, char** output);

/**
 * @brief Serialize a given type to a JSON object.
 *
 * Caller is the owner of the out parameter and should release it using json_decref.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The type to serialize.
 * @param[in] input The input to serialize.
 * @param[out] out The serialized result.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonSerializer_serializeJson(const dyn_type* type, const void* input, json_t** out);

#ifdef __cplusplus
}
#endif

#endif
