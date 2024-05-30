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

#include "celix_properties_ei.h"

extern "C" {
void *__real_celix_properties_create();
CELIX_EI_DEFINE(celix_properties_create, celix_properties_t*)
void *__wrap_celix_properties_create() {
    CELIX_EI_IMPL(celix_properties_create);
    return __real_celix_properties_create();
}

celix_properties_t *__real_celix_properties_copy(const celix_properties_t *properties);
CELIX_EI_DEFINE(celix_properties_copy, celix_properties_t*)
celix_properties_t *__wrap_celix_properties_copy(const celix_properties_t *properties) {
    CELIX_EI_IMPL(celix_properties_copy);
    return __real_celix_properties_copy(properties);
}

celix_status_t __real_celix_properties_set(celix_properties_t *properties, const char *key, const char *value);
CELIX_EI_DEFINE(celix_properties_set, celix_status_t)
celix_status_t __wrap_celix_properties_set(celix_properties_t *properties, const char *key, const char *value) {
    CELIX_EI_IMPL(celix_properties_set);
    return __real_celix_properties_set(properties, key, value);
}

celix_status_t __real_celix_properties_setLong(celix_properties_t *properties, const char *key, long value);
CELIX_EI_DEFINE(celix_properties_setLong, celix_status_t)
celix_status_t __wrap_celix_properties_setLong(celix_properties_t *properties, const char *key, long value) {
    CELIX_EI_IMPL(celix_properties_setLong);
    return __real_celix_properties_setLong(properties, key, value);
}

celix_status_t __real_celix_properties_setVersion(celix_properties_t *properties, const char *key, const celix_version_t *version);
CELIX_EI_DEFINE(celix_properties_setVersion, celix_status_t)
celix_status_t __wrap_celix_properties_setVersion(celix_properties_t *properties, const char *key, const celix_version_t *version) {
    CELIX_EI_IMPL(celix_properties_setVersion);
    return __real_celix_properties_setVersion(properties, key, version);
}

celix_status_t
__real_celix_properties_save(const celix_properties_t* properties, const char* filename, int encodeFlags);
CELIX_EI_DEFINE(celix_properties_save, celix_status_t)
celix_status_t
__wrap_celix_properties_save(const celix_properties_t* properties, const char* filename, int encodeFlags) {
    CELIX_EI_IMPL(celix_properties_save);
    return __real_celix_properties_save(properties, filename, encodeFlags);
}

celix_status_t
__real_celix_properties_load(const char* filename,
                                    int decodeFlags,
                                    celix_properties_t** out);
CELIX_EI_DEFINE(celix_properties_load, celix_status_t)
celix_status_t
__wrap_celix_properties_load(const char* filename,
                              int decodeFlags,
                              celix_properties_t** out) {
    CELIX_EI_IMPL(celix_properties_load);
    return __real_celix_properties_load(filename, decodeFlags, out);
}

}