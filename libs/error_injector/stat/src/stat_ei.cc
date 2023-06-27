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

#include "stat_ei.h"
#include <errno.h>

extern "C" {

int __real_mkdir(const char *__path, __mode_t __mode);
CELIX_EI_DEFINE(mkdir, int)
int __wrap_mkdir(const char *__path, __mode_t __mode) {
    errno = EACCES;
    CELIX_EI_IMPL(mkdir);
    errno = 0;
    return __real_mkdir(__path, __mode);
}


int __real_stat(const char *__restrict __file, struct stat *__restrict __buf);
CELIX_EI_DEFINE(stat, int)
int __wrap_stat(const char *__restrict __file, struct stat *__restrict __buf) {
    errno = EOVERFLOW;
    CELIX_EI_IMPL(stat);
    errno = 0;
    return __real_stat(__file, __buf);
}

int __real_lstat(const char *__restrict __file, struct stat *__restrict __buf);
CELIX_EI_DEFINE(lstat, int)
int __wrap_lstat(const char *__restrict __file, struct stat *__restrict __buf) {
    errno = EACCES;
    CELIX_EI_IMPL(lstat);
    errno = 0;
    return __real_lstat(__file, __buf);
}
}