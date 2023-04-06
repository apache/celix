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

#include <errno.h>
#include "stdio_ei.h"

extern "C" {
FILE *__real_fopen (const char *__filename, const char *__modes);
CELIX_EI_DEFINE(fopen, FILE *)
FILE *__wrap_fopen (const char *__filename, const char *__modes) {
    errno = EMFILE;
    CELIX_EI_IMPL(fopen);
    errno = 0;
    return __real_fopen(__filename, __modes);
}


size_t __real_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s);
CELIX_EI_DEFINE(fwrite, size_t)
size_t __wrap_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s) {
    errno = ENOSPC;
    CELIX_EI_IMPL(fwrite);
    errno = 0;
    return __real_fwrite(__ptr, __size, __n, __s);
}


int __real_remove (const char *__filename);
CELIX_EI_DEFINE(remove, int)
int __wrap_remove (const char *__filename) {
    errno = EACCES;
    CELIX_EI_IMPL(remove);
    errno = 0;
    return __real_remove(__filename);
}
}