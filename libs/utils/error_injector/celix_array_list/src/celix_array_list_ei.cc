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

#include "celix_array_list_ei.h"

extern "C" {

void *__real_celix_arrayList_create(void);
CELIX_EI_DEFINE(celix_arrayList_create, celix_array_list_t*)
void *__wrap_celix_arrayList_create(void) {
    CELIX_EI_IMPL(celix_arrayList_create);
    return __real_celix_arrayList_create();
}

void *__real_celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts);
CELIX_EI_DEFINE(celix_arrayList_createWithOptions, celix_array_list_t*)
void *__wrap_celix_arrayList_createWithOptions(const celix_array_list_create_options_t* opts) {
    CELIX_EI_IMPL(celix_arrayList_createWithOptions);
    return __real_celix_arrayList_createWithOptions(opts);
}

celix_status_t __real_celix_arrayList_add(celix_array_list_t* list, void* value);
CELIX_EI_DEFINE(celix_arrayList_add, celix_status_t)
celix_status_t __wrap_celix_arrayList_add(celix_array_list_t* list, void* value) {
    CELIX_EI_IMPL(celix_arrayList_add);
    return __real_celix_arrayList_add(list, value);
}

celix_status_t __real_celix_arrayList_addString(celix_array_list_t* list, const char* value);
CELIX_EI_DEFINE(celix_arrayList_addString, celix_status_t)
celix_status_t __wrap_celix_arrayList_addString(celix_array_list_t* list, const char* value) {
    CELIX_EI_IMPL(celix_arrayList_addString);
    return __real_celix_arrayList_addString(list, value);
}

celix_status_t __real_celix_arrayList_addInt(celix_array_list_t* list, int value);
CELIX_EI_DEFINE(celix_arrayList_addInt, celix_status_t)
celix_status_t __wrap_celix_arrayList_addInt(celix_array_list_t* list, int value) {
    CELIX_EI_IMPL(celix_arrayList_addInt);
    return __real_celix_arrayList_addInt(list, value);
}

celix_status_t __real_celix_arrayList_addLong(celix_array_list_t* list, long value);
CELIX_EI_DEFINE(celix_arrayList_addLong, celix_status_t)
celix_status_t __wrap_celix_arrayList_addLong(celix_array_list_t* list, long value) {
    CELIX_EI_IMPL(celix_arrayList_addLong);
    return __real_celix_arrayList_addLong(list, value);
}

celix_status_t __real_celix_arrayList_addUInt(celix_array_list_t* list, unsigned int value);
CELIX_EI_DEFINE(celix_arrayList_addUInt, celix_status_t)
celix_status_t __wrap_celix_arrayList_addUInt(celix_array_list_t* list, unsigned int value) {
    CELIX_EI_IMPL(celix_arrayList_addUInt);
    return __real_celix_arrayList_addUInt(list, value);
}

celix_status_t __real_celix_arrayList_addULong(celix_array_list_t* list, unsigned long value);
CELIX_EI_DEFINE(celix_arrayList_addULong, celix_status_t)
celix_status_t __wrap_celix_arrayList_addULong(celix_array_list_t* list, unsigned long value) {
    CELIX_EI_IMPL(celix_arrayList_addULong);
    return __real_celix_arrayList_addULong(list, value);
}

celix_status_t __real_celix_arrayList_addFloat(celix_array_list_t* list, float value);
CELIX_EI_DEFINE(celix_arrayList_addFloat, celix_status_t)
celix_status_t __wrap_celix_arrayList_addFloat(celix_array_list_t* list, float value) {
    CELIX_EI_IMPL(celix_arrayList_addFloat);
    return __real_celix_arrayList_addFloat(list, value);
}

celix_status_t __real_celix_arrayList_addDouble(celix_array_list_t* list, double value);
CELIX_EI_DEFINE(celix_arrayList_addDouble, celix_status_t)
celix_status_t __wrap_celix_arrayList_addDouble(celix_array_list_t* list, double value) {
    CELIX_EI_IMPL(celix_arrayList_addDouble);
    return __real_celix_arrayList_addDouble(list, value);
}

celix_status_t __real_celix_arrayList_addBool(celix_array_list_t* list, bool value);
CELIX_EI_DEFINE(celix_arrayList_addBool, celix_status_t)
celix_status_t __wrap_celix_arrayList_addBool(celix_array_list_t* list, bool value) {
    CELIX_EI_IMPL(celix_arrayList_addBool);
    return __real_celix_arrayList_addBool(list, value);
}

celix_status_t __real_celix_arrayList_addSize(celix_array_list_t* list, size_t value);
CELIX_EI_DEFINE(celix_arrayList_addSize, celix_status_t)
celix_status_t __wrap_celix_arrayList_addSize(celix_array_list_t* list, size_t value) {
    CELIX_EI_IMPL(celix_arrayList_addSize);
    return __real_celix_arrayList_addSize(list, value);
}

celix_array_list_t* __real_celix_arrayList_copy(const celix_array_list_t* list);
CELIX_EI_DEFINE(celix_arrayList_copy, celix_array_list_t*)
celix_array_list_t* __wrap_celix_arrayList_copy(const celix_array_list_t* list) {
    CELIX_EI_IMPL(celix_arrayList_copy);
    return __real_celix_arrayList_copy(list);
}

}
