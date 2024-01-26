/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */
#ifndef CELIX_JANSSON_EI_H
#define CELIX_JANSSON_EI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <jansson.h>
#include "celix_error_injector.h"

CELIX_EI_DECLARE(json_array_size, size_t);
CELIX_EI_DECLARE(json_dumps, char*);
CELIX_EI_DECLARE(json_object, json_t*);
CELIX_EI_DECLARE(json_object_set_new, int);
CELIX_EI_DECLARE(json_array, json_t*);
CELIX_EI_DECLARE(json_array_append_new, int);
CELIX_EI_DECLARE(json_integer, json_t*);
CELIX_EI_DECLARE(json_string, json_t*);

#ifdef __cplusplus
}
#endif
#endif //CELIX_JANSSON_EI_H
