/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
#ifndef __AVROBIN_SERIALIZER_H_
#define __AVROBIN_SERIALIZER_H_

#include "dfi_log_util.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

//logging
DFI_SETUP_LOG_HEADER(avrobinSerializer);

/**
 * @brief Deserialize data in AVRO format.
 *
 * The caller is the owner of the deserialized data and use `dynType_free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dynamic type for the input data.
 * @param[in] input The input data in AVRO format.
 * @param[in] inlen The length of the input data.
 * @param[out] result The deserialized data.
 * @return 0 if successful, otherwise 1.
 * @deprecated AVRO is deprecated and will be removed in the future.
 */
CELIX_DFI_DEPRECATED_EXPORT int avrobinSerializer_deserialize(dyn_type *type, const uint8_t *input, size_t inlen, void **result);

/**
 * @brief Serialize data to AVRO format.
 *
 * The caller is the owner of the serialized data and use `free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dynamic type for the input data.
 * @param[in] input The input data.
 * @param[out] output The serialized data in AVRO format.
 * @param[out] outlen The length of the serialized data.
 * @return 0 if successful, otherwise 1.
 * @deprecated AVRO is deprecated and will be removed in the future.
 */
CELIX_DFI_DEPRECATED_EXPORT int avrobinSerializer_serialize(dyn_type *type, const void *input, uint8_t **output, size_t *outlen);


/**
 * @brief Generate a AVRO schema for a given Dynamic Type.
 *
 * The caller is the owner of the generated schema and use `free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dynamic type for which a schema should be generated.
 * @param[out] output The generated schema.
 * @return 0 if successful, otherwise 1.
 * @deprecated AVRO is deprecated and will be removed in the future.
 */
CELIX_DFI_DEPRECATED_EXPORT int avrobinSerializer_generateSchema(dyn_type *type, char **output);

/**
 * @brief Save a serialized data to a file.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] filename The name of the file.
 * @param[in] schema The schema of the serialized data.
 * @param[in] serdata The serialized data.
 * @param[in] serdatalen The length of the serialized data.
 * @return 0 if successful, otherwise 1.
 * @deprecated AVRO is deprecated and will be removed in the future.
 */
CELIX_DFI_DEPRECATED_EXPORT int avrobinSerializer_saveFile(const char *filename, const char *schema, const uint8_t *serdata, size_t serdatalen);

#ifdef __cplusplus
}
#endif

#endif
