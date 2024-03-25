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

#include "celix_utils_ei.h"
#include <cstdarg>
#include <errno.h>

extern "C" {

char* __real_celix_utils_strdup(const char *);
CELIX_EI_DEFINE(celix_utils_strdup, char*)
char* __wrap_celix_utils_strdup(const char* str) {
    errno = ENOMEM;
    CELIX_EI_IMPL(celix_utils_strdup);
    errno = 0;
    return __real_celix_utils_strdup(str);
}

celix_status_t __real_celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut);
CELIX_EI_DEFINE(celix_utils_createDirectory, celix_status_t)
celix_status_t __wrap_celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut) {
    if (errorOut) {
        *errorOut = "Error Injected";
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
        *errorOut = "Error Injected";
    }
    CELIX_EI_IMPL(celix_utils_deleteDirectory);
    return __real_celix_utils_deleteDirectory(path, errorOut);
}

CELIX_EI_DEFINE(celix_utils_writeOrCreateString, char *)
char* __wrap_celix_utils_writeOrCreateString(char* buffer, size_t bufferSize, const char* format, ...) {
    errno = ENOMEM;
    CELIX_EI_IMPL(celix_utils_writeOrCreateString);
    errno = 0;
    va_list args;
    va_start(args, format);
    char *ret = celix_utils_writeOrCreateVString(buffer, bufferSize, format, args);
    va_end(args);
    return ret;
}


celix_status_t __real_celix_utils_extractZipData(const void *zipData, size_t zipDataSize, const char* extractToDir, const char** errorOut);
CELIX_EI_DEFINE(celix_utils_extractZipData, celix_status_t)
celix_status_t __wrap_celix_utils_extractZipData(const void *zipData, size_t zipDataSize, const char* extractToDir, const char** errorOut) {
    if (errorOut) {
        *errorOut = "Error Injected";
    }
    CELIX_EI_IMPL(celix_utils_extractZipData);
    return __real_celix_utils_extractZipData(zipData, zipDataSize, extractToDir, errorOut);
}

celix_status_t __real_celix_utils_extractZipFile(const char *zipFilePath, const char* extractToDir, const char** errorOut);
CELIX_EI_DEFINE(celix_utils_extractZipFile, celix_status_t)
celix_status_t __wrap_celix_utils_extractZipFile(const char *zipFilePath, const char* extractToDir, const char** errorOut) {
    if (errorOut) {
        *errorOut = "Error Injected";
    }
    CELIX_EI_IMPL(celix_utils_extractZipFile);
    return __real_celix_utils_extractZipFile(zipFilePath, extractToDir, errorOut);
}

char* __real_celix_utils_trim(const char* string);
CELIX_EI_DEFINE(celix_utils_trim, char*)
char* __wrap_celix_utils_trim(const char* string) {
    CELIX_EI_IMPL(celix_utils_trim);
    return __real_celix_utils_trim(string);
}

struct timespec __real_celix_gettime(clockid_t clockId);
CELIX_EI_DEFINE(celix_gettime, struct timespec)
struct timespec __wrap_celix_gettime(clockid_t clockId) {
    CELIX_EI_IMPL(celix_gettime);
    return __real_celix_gettime(clockId);
}

double __real_celix_elapsedtime(clockid_t clockId, struct timespec startTime);
CELIX_EI_DEFINE(celix_elapsedtime, double)
double __wrap_celix_elapsedtime(clockid_t clockId, struct timespec startTime) {
    CELIX_EI_IMPL(celix_elapsedtime);
    return __real_celix_elapsedtime(clockId, startTime);
}

}