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

#ifdef __cplusplus
extern "C" {
#endif

//logging
DFI_SETUP_LOG_HEADER(avrobinSerializer);

int avrobinSerializer_deserialize(dyn_type *type, const uint8_t *input, size_t inlen, void **result);

int avrobinSerializer_serialize(dyn_type *type, const void *input, uint8_t **output, size_t *outlen);

int avrobinSerializer_generateSchema(dyn_type *type, char **output);

int avrobinSerializer_saveFile(const char *filename, const char *schema, const uint8_t *serdata, size_t serdatalen);

#ifdef __cplusplus
}
#endif

#endif
