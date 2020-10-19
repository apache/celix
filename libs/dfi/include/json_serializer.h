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
#include "dfi_log_util.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

//logging
DFI_SETUP_LOG_HEADER(jsonSerializer);

int jsonSerializer_deserialize(dyn_type *type, const char *input, size_t length, void **result);
int jsonSerializer_deserializeJson(dyn_type *type, json_t *input, void **result);

int jsonSerializer_serialize(dyn_type *type, const void* input, char **output);
int jsonSerializer_serializeJson(dyn_type *type, const void* input, json_t **out);

#ifdef __cplusplus
}
#endif

#endif
