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

#include "zip_ei.h"

extern "C" {

int __real_zip_stat_index(zip_t *, zip_uint64_t, zip_flags_t, zip_stat_t *);
CELIX_EI_DEFINE(zip_stat_index, int)
int __wrap_zip_stat_index(zip_t *za, zip_uint64_t index, zip_flags_t flags, zip_stat_t *st) {
    zip_error_set(zip_get_error(za), ZIP_ER_MEMORY, 0);
    CELIX_EI_IMPL(zip_stat_index);
    zip_error_clear(za);
    return __real_zip_stat_index(za, index, flags, st);
}


zip_file_t * __real_zip_fopen_index(zip_t *, zip_uint64_t, zip_flags_t);
CELIX_EI_DEFINE(zip_fopen_index, zip_file_t *)
zip_file_t * __wrap_zip_fopen_index(zip_t *za, zip_uint64_t index, zip_flags_t flags) {
    zip_error_set(zip_get_error(za), ZIP_ER_MEMORY, 0);
    CELIX_EI_IMPL(zip_fopen_index);
    zip_error_clear(za);
    return __real_zip_fopen_index(za, index, flags);
}


zip_int64_t __real_zip_fread(zip_file_t *, void *, zip_uint64_t);
CELIX_EI_DEFINE(zip_fread, zip_int64_t)
zip_int64_t __wrap_zip_fread(zip_file_t *zf, void *buf, zip_uint64_t len) {
    zip_error_set(zip_file_get_error(zf), ZIP_ER_INTERNAL, 0);
    CELIX_EI_IMPL(zip_fread);
    zip_error_set(zip_file_get_error(zf), 0, 0);
    return __real_zip_fread(zf, buf, len);
}
}