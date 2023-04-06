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

#include "dlfcn_ei.h"
#include <errno.h>

extern "C" {
void *__real_dlopen(const char *__file, int __mode);
CELIX_EI_DEFINE(dlopen, void *)
void *__wrap_dlopen(const char *__file, int __mode) {
    CELIX_EI_IMPL(dlopen);
    return __real_dlopen(__file, __mode);
}

char *__real_dlerror(void);
CELIX_EI_DEFINE(dlerror, char *)
char *__wrap_dlerror(void) {
    CELIX_EI_IMPL(dlerror);
    return __real_dlerror();
}

}