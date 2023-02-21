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

#include "malloc_ei.h"

extern "C" {
void *__real_malloc(size_t);
CELIX_EI_DEFINE(malloc, void *)
void *__wrap_malloc(size_t size) {
    CELIX_EI_IMPL0(malloc);
    return __real_malloc(size);
}

void *__real_realloc(void *__ptr, size_t __size);
CELIX_EI_DEFINE(realloc, void *)
void *__wrap_realloc(void *__ptr, size_t __size) {
    CELIX_EI_IMPL0(realloc);
    return __real_realloc(__ptr, __size);
}

void *__real_calloc (size_t __nmemb, size_t __size);
CELIX_EI_DEFINE(calloc, void *)
void *__wrap_calloc (size_t __nmemb, size_t __size) {
    CELIX_EI_IMPL0(calloc);
    return __real_calloc(__nmemb, __size);
}
}