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

#include "jansson_ei.h"
#include "celix_cleanup.h"
#include <stddef.h>

extern "C" {

size_t __real_json_array_size(const json_t *array);
CELIX_EI_DEFINE(json_array_size, size_t)
size_t __wrap_json_array_size(const json_t *array) {
    CELIX_EI_IMPL(json_array_size);
    return __real_json_array_size(array);
}

char* __real_json_dumps(const json_t *json, size_t flags);
CELIX_EI_DEFINE(json_dumps, char*)
char* __wrap_json_dumps(const json_t *json, size_t flags) {
    CELIX_EI_IMPL(json_dumps);
    return __real_json_dumps(json, flags);
}

json_t* __real_json_object(void);
CELIX_EI_DEFINE(json_object, json_t*)
json_t* __wrap_json_object(void) {
    CELIX_EI_IMPL(json_object);
    return __real_json_object();
}

int __real_json_object_set_new(json_t *object, const char *key, json_t *value);
CELIX_EI_DEFINE(json_object_set_new, int)
int __wrap_json_object_set_new(json_t *object, const char *key, json_t *value) {
    json_auto_t* val = value;
    CELIX_EI_IMPL(json_object_set_new);
    return __real_json_object_set_new(object, key, celix_steal_ptr(val));
}

json_t* __real_json_array(void);
CELIX_EI_DEFINE(json_array, json_t*)
json_t* __wrap_json_array(void) {
    CELIX_EI_IMPL(json_array);
    return __real_json_array();
}

int __real_json_array_append_new(json_t *array, json_t *value);
CELIX_EI_DEFINE(json_array_append_new, int)
int __wrap_json_array_append_new(json_t *array, json_t *value) {
    json_auto_t *val = value;
    CELIX_EI_IMPL(json_array_append_new);
    return __real_json_array_append_new(array, celix_steal_ptr(val));
}

json_t* __real_json_integer(json_int_t value);
CELIX_EI_DEFINE(json_integer, json_t*)
json_t* __wrap_json_integer(json_int_t value) {
    CELIX_EI_IMPL(json_integer);
    return __real_json_integer(value);
}

}