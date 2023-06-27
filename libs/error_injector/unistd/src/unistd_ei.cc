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

#include "unistd_ei.h"

#include <errno.h>

extern "C" {
char *__real_getcwd(char *buf, size_t size);
CELIX_EI_DEFINE(getcwd, char*)
char *__wrap_getcwd(char *buf, size_t size) {
    errno = ENOMEM;
    CELIX_EI_IMPL(getcwd);
    errno = 0;
    return __real_getcwd(buf, size);
}

int __real_symlink(const char *target, const char *linkpath);
CELIX_EI_DEFINE(symlink, int)
int __wrap_symlink(const char *target, const char *linkpath) {
    errno = EIO;
    CELIX_EI_IMPL(symlink);
    errno = 0;
    return __real_symlink(target, linkpath);
}

int __real_unlink(const char *pathname);
CELIX_EI_DEFINE(unlink, int)
int __wrap_unlink(const char *pathname) {
    errno = EACCES;
    CELIX_EI_IMPL(unlink);
    errno = 0;
    return __real_unlink(pathname);
}

}