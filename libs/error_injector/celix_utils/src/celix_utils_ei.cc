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

#include <cstdarg>
#include "celix_utils_ei.h"

extern "C" {

char* __real_celix_utils_strdup(const char *);
CELIX_EI_DEFINE(celix_utils_strdup, char*)
char* __wrap_celix_utils_strdup(const char* str) {
    CELIX_EI_IMPL0(celix_utils_strdup);
    return __real_celix_utils_strdup(str);
}

celix_status_t __real_celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut);
CELIX_EI_DEFINE(celix_utils_createDirectory, celix_status_t)
celix_status_t __wrap_celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut) {
    if (errorOut) {
        *errorOut = nullptr;
    }
    CELIX_EI_IMPL(celix_utils_createDirectory);
    return __real_celix_utils_createDirectory(path, failIfPresent, errorOut);
}

celix_status_t __real_celix_utils_getLastModified(const char* path, struct timespec* lastModified);
CELIX_EI_DEFINE(celix_utils_getLastModified, celix_status_t)
celix_status_t __wrap_celix_utils_getLastModified(const char* path, struct timespec* lastModified) {
    CELIX_EI_IMPL(celix_utils_getLastModified);
    return __real_celix_utils_getLastModified(path, lastModified);
}

celix_status_t __real_celix_utils_deleteDirectory(const char* path, const char** errorOut);
CELIX_EI_DEFINE(celix_utils_deleteDirectory, celix_status_t)
celix_status_t __wrap_celix_utils_deleteDirectory(const char* path, const char** errorOut) {
    if (errorOut) {
        *errorOut = nullptr;
    }
    CELIX_EI_IMPL(celix_utils_deleteDirectory);
    return __real_celix_utils_deleteDirectory(path, errorOut);
}

}
